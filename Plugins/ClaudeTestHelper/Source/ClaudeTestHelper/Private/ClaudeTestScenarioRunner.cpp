#include "ClaudeTestScenarioRunner.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"
#include "Misc/OutputDeviceRedirector.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogScenario, Log, All);
/** Remote step replies: in their own category, because expects don't look at the runner's own lines. */
DEFINE_LOG_CATEGORY_STATIC(LogScenarioRemote, Log, All);

namespace ClaudeTestScenarioRunnerPrivate
{
	constexpr uint64 ScreenKey = 0xC1A0DE10ull;
	/** How long the result stays on screen: until the next scenario, in practice. */
	constexpr float ResultSeconds = 3600.f;
	const FName LogScenarioName(TEXT("LogScenario"));
	/** How long a screenshot may take to be written. */
	constexpr double ShotSeconds = 15.0;
	/** How long a remote step waits for its reply. */
	constexpr double RemoteSeconds = 15.0;

	/** The port Remote Control's web server listens on (its settings, which a -ini: override changes). Read by name, so the
	 *  plugin doesn't need Remote Control; 30010, its default, when it isn't loaded. */
	int32 RemoteControlPort()
	{
		const UClass* Settings = FindObject<UClass>(nullptr, TEXT("/Script/RemoteControlCommon.RemoteControlSettings"));
		const FNumericProperty* Port = Settings ? FindFProperty<FNumericProperty>(Settings, TEXT("RemoteControlHttpServerPort")) : nullptr;
		return Port ? int32(Port->GetUnsignedIntPropertyValue(Port->ContainerPtrToValuePtr<void>(Settings->GetDefaultObject()))) : 30010;
	}
}

// Log capture

int32 UClaudeTestScenarioRunner::FLogCapture::FindNumber(const FString& Text, int32 From, double Min, double Max, TOptional<double>& OutLastSeen) const
{
	FScopeLock ScopeLock(&Lock);
	for (int32 Index = FMath::Max(0, From); Index < Lines.Num(); ++Index)
	{
		double Number = 0.0;
		if (ClaudeTestScenario::NumberAfter(Lines[Index], Text, Number))
		{
			if (Number >= Min && Number <= Max)
			{
				return Index;
			}
			OutLastSeen = Number;
		}
	}
	return INDEX_NONE;
}

void UClaudeTestScenarioRunner::FLogCapture::Serialize(const TCHAR* Message, ELogVerbosity::Type Verbosity, const FName& Category)
{
	// The runner's own lines would match what they describe ("expect Started X"), and so would echoed commands.
	if (Category == ClaudeTestScenarioRunnerPrivate::LogScenarioName || FCString::Strncmp(Message, TEXT("Cmd: "), 5) == 0)
	{
		return;
	}
	FString Line = Category.IsNone() ? FString(Message) : FString::Printf(TEXT("%s: %s"), *Category.ToString(), Message);
	FScopeLock ScopeLock(&Lock);
	Lines.Add(MoveTemp(Line));
}

int32 UClaudeTestScenarioRunner::FLogCapture::Num() const
{
	FScopeLock ScopeLock(&Lock);
	return Lines.Num();
}

int32 UClaudeTestScenarioRunner::FLogCapture::Find(const FString& Text, int32 From) const
{
	FScopeLock ScopeLock(&Lock);
	for (int32 Index = FMath::Max(0, From); Index < Lines.Num(); ++Index)
	{
		if (Lines[Index].Contains(Text))
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

// Runner

bool UClaudeTestScenarioRunner::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
	return false;
#else
	return Super::ShouldCreateSubsystem(Outer);
#endif
}

void UClaudeTestScenarioRunner::Deinitialize()
{
	if (bRunning)
	{
		Finish(false, TEXT("the game closed before the scenario finished"));
	}
	Super::Deinitialize();
}

void UClaudeTestScenarioRunner::Start(const FString& NameOrPath, bool bInExitWhenDone)
{
	if (bRunning)
	{
		UE_LOG(LogScenario, Warning, TEXT("Scenario %s is still running; wait for it to finish."), *Name);
		return;
	}

	Name = FPaths::GetBaseFilename(NameOrPath);
	bExitWhenDone = bInExitWhenDone;
	bRunning = true;
	LastResult.Reset();
	LastReason.Reset();
	Steps.Reset();
	StepIndex = 0;
	bStepStarted = false;
	Cursor = 0;
	Rejects.Reset();

	FString Path = NameOrPath;
	if (FPaths::IsRelative(Path) || !FPaths::FileExists(Path))
	{
		Path = FPaths::Combine(FPaths::ProjectDir(), TEXT("Tests"), TEXT("Scenarios"), Name + TEXT(".scenario"));
	}
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *Path))
	{
		Finish(false, FString::Printf(TEXT("couldn't read %s"), *Path));
		return;
	}
	FString Error;
	if (!ClaudeTestScenario::Parse(Text, Steps, Error))
	{
		Finish(false, Error);
		return;
	}

	Capture = MakeUnique<FLogCapture>();
	GLog->AddOutputDevice(Capture.Get());
	UE_LOG(LogScenario, Display, TEXT("Scenario %s: %d steps (%s)"), *Name, Steps.Num(), *Path);
	// The core ticker keeps going through level changes and while the game is paused.
	TickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UClaudeTestScenarioRunner::Tick), 0.05f);
}

bool UClaudeTestScenarioRunner::Tick(float DeltaTime)
{
	if (!bRunning)
	{
		return false;
	}
	if (!CheckRejects())
	{
		return false;
	}
	return Advance();
}

bool UClaudeTestScenarioRunner::Advance()
{
	using ClaudeTestScenario::EStepKind;
	while (bRunning && StepIndex < Steps.Num())
	{
		const ClaudeTestScenario::FStep& Step = Steps[StepIndex];
		const double Now = FPlatformTime::Seconds();
		if (!bStepStarted)
		{
			bStepStarted = true;
			StepStartTime = Now;
		}

		switch (Step.Kind)
		{
		case EStepKind::Command:
		{
			UE_LOG(LogScenario, Display, TEXT("line %d: > %s"), Step.Line, *Step.Text);
			// The expects after a command look only at what was logged from the command on, never at older output.
			Cursor = FMath::Max(Cursor, Capture->Num());
			if (GEngine)
			{
				GEngine->Exec(GetGameWorld(), *Step.Text, *GLog);
			}
			break;
		}
		case EStepKind::Expect:
		{
			const int32 Found = Capture->Find(Step.Text, Cursor);
			if (Found == INDEX_NONE)
			{
				if (Now - StepStartTime > Step.Seconds)
				{
					Finish(false, FString::Printf(TEXT("line %d: didn't see \"%s\" within %.0f s"), Step.Line, *Step.Text, Step.Seconds));
					return false;
				}
				return true;
			}
			Cursor = Found + 1;
			UE_LOG(LogScenario, Display, TEXT("line %d: saw \"%s\""), Step.Line, *Step.Text);
			break;
		}
		case EStepKind::ExpectNumber:
		{
			TOptional<double> LastSeen;
			const int32 Found = Capture->FindNumber(Step.Text, Cursor, Step.Min, Step.Max, LastSeen);
			if (Found == INDEX_NONE)
			{
				if (Now - StepStartTime > Step.Seconds)
				{
					const FString Seen = LastSeen.IsSet() ? FString::Printf(TEXT("the last one was %g"), LastSeen.GetValue()) : FString(TEXT("it never showed with a number"));
					Finish(false, FString::Printf(TEXT("line %d: didn't see \"%s\" with a number from %g to %g within %.0f s (%s)"), Step.Line, *Step.Text, Step.Min, Step.Max, Step.Seconds, *Seen));
					return false;
				}
				return true;
			}
			Cursor = Found + 1;
			UE_LOG(LogScenario, Display, TEXT("line %d: saw \"%s\" with a number from %g to %g"), Step.Line, *Step.Text, Step.Min, Step.Max);
			break;
		}
		case EStepKind::Reject:
		{
			FReject& Reject = Rejects.AddDefaulted_GetRef();
			Reject.Text = Step.Text;
			Reject.CheckedUpTo = Capture->Num();
			Reject.Line = Step.Line;
			UE_LOG(LogScenario, Display, TEXT("line %d: from now on, \"%s\" must not appear"), Step.Line, *Step.Text);
			break;
		}
		case EStepKind::Allow:
		{
			// Anything that already showed up still counts.
			if (!CheckRejects())
			{
				return false;
			}
			Rejects.RemoveAll([&Step](const FReject& Reject) { return Reject.Text == Step.Text; });
			UE_LOG(LogScenario, Display, TEXT("line %d: \"%s\" is allowed again"), Step.Line, *Step.Text);
			break;
		}
		case EStepKind::Wait:
		{
			if (Now - StepStartTime < Step.Seconds)
			{
				return true;
			}
			break;
		}
		case EStepKind::Note:
		{
			UE_LOG(LogScenario, Display, TEXT("line %d: note: %s"), Step.Line, *Step.Text);
			break;
		}
		case EStepKind::Shot:
		{
			if (!TakeShot(Step, Now))
			{
				return bRunning;
			}
			break;
		}
		case EStepKind::Remote:
		{
			if (!SendRemote(Step, Now))
			{
				return bRunning;
			}
			break;
		}
		}

		++StepIndex;
		bStepStarted = false;
		// A command may have changed level or started something that logs next frame: carry on next tick.
		if (Step.Kind == EStepKind::Command)
		{
			return true;
		}
	}

	if (bRunning)
	{
		if (!CheckRejects())
		{
			return false;
		}
		Finish(true, FString());
	}
	return false;
}

bool UClaudeTestScenarioRunner::TakeShot(const ClaudeTestScenario::FStep& Step, double Now)
{
	FString Folder;
	if (!FParse::Value(FCommandLine::Get(), TEXT("UIShotsDir="), Folder))
	{
		UE_LOG(LogScenario, Display, TEXT("line %d: shot %s skipped (screenshots are taken in ue.py uishots runs)"), Step.Line, *Step.Text);
		return true;
	}
	if (PendingShot.IsEmpty())
	{
		// The whole viewport with the UI, at the game's resolution, saved under the shot's own name.
		PendingShot = FPaths::Combine(Folder, Step.Text + TEXT(".png"));
		PendingShotSize = -1;
		IFileManager::Get().Delete(*PendingShot, /*RequireExists*/ false, /*EvenReadOnly*/ true, /*Quiet*/ true);
		FScreenshotRequest::RequestScreenshot(PendingShot, /*bShowUI*/ true, /*bAddFilenameSuffix*/ false);
		return false;
	}
	// Done once the file is there and has stopped growing.
	const int64 Size = IFileManager::Get().FileSize(*PendingShot);
	if (Size > 0 && Size == PendingShotSize)
	{
		UE_LOG(LogScenario, Display, TEXT("line %d: SHOT %s saved (%s)"), Step.Line, *Step.Text, *PendingShot);
		PendingShot.Reset();
		return true;
	}
	PendingShotSize = Size;
	if (Now - StepStartTime > ClaudeTestScenarioRunnerPrivate::ShotSeconds)
	{
		const FString File = PendingShot;
		PendingShot.Reset();
		Finish(false, FString::Printf(TEXT("line %d: screenshot %s wasn't written within %.0f s (%s)"), Step.Line, *Step.Text, ClaudeTestScenarioRunnerPrivate::ShotSeconds, *File));
	}
	return false;
}

bool UClaudeTestScenarioRunner::SendRemote(const ClaudeTestScenario::FStep& Step, double Now)
{
	if (!RemoteRequest.IsValid())
	{
		// Like a command: the expects after it look only at what's logged from here on.
		Cursor = FMath::Max(Cursor, Capture->Num());
		const UWorld* World = GetGameWorld();
		const FString Url = FString::Printf(TEXT("http://127.0.0.1:%d%s"), ClaudeTestScenarioRunnerPrivate::RemoteControlPort(), *Step.Text);
		bRemoteReplied = false;
		RemoteCode = 0;
		RemoteBody.Reset();
		RemoteRequest = FHttpModule::Get().CreateRequest();
		RemoteRequest->SetURL(Url);
		RemoteRequest->SetVerb(Step.Verb);
		RemoteRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
		if (!Step.Body.IsEmpty())
		{
			RemoteRequest->SetContentAsString(ClaudeTestScenario::FillWorld(Step.Body, World ? World->GetPathName() : FString()));
		}
		TWeakObjectPtr<UClaudeTestScenarioRunner> WeakThis(this);
		RemoteRequest->OnProcessRequestComplete().BindLambda([WeakThis](FHttpRequestPtr, FHttpResponsePtr Response, bool bConnected)
		{
			if (UClaudeTestScenarioRunner* Runner = WeakThis.Get())
			{
				Runner->bRemoteReplied = true;
				Runner->RemoteCode = bConnected && Response.IsValid() ? Response->GetResponseCode() : 0;
				Runner->RemoteBody = Response.IsValid() ? Response->GetContentAsString() : FString();
			}
		});
		UE_LOG(LogScenario, Display, TEXT("line %d: > remote %s %s"), Step.Line, *Step.Verb, *Url);
		RemoteRequest->ProcessRequest();
		return false;
	}
	if (!bRemoteReplied)
	{
		if (Now - StepStartTime > ClaudeTestScenarioRunnerPrivate::RemoteSeconds)
		{
			RemoteRequest->CancelRequest();
			RemoteRequest.Reset();
			Finish(false, FString::Printf(TEXT("line %d: no reply from Remote Control within %.0f s (its web server needs -RCWebControlEnable and rendering; ue.py game adds both for tests with remote lines)"),
				Step.Line, ClaudeTestScenarioRunnerPrivate::RemoteSeconds));
		}
		return false;
	}
	RemoteRequest.Reset();
	for (const FString& Line : ClaudeTestScenario::DescribeRemoteReply(RemoteCode, RemoteBody))
	{
		UE_LOG(LogScenarioRemote, Display, TEXT("%s"), *Line);
	}
	return true;
}

bool UClaudeTestScenarioRunner::CheckRejects()
{
	if (!Capture)
	{
		return true;
	}
	for (FReject& Reject : Rejects)
	{
		const int32 Found = Capture->Find(Reject.Text, Reject.CheckedUpTo);
		if (Found != INDEX_NONE)
		{
			Finish(false, FString::Printf(TEXT("line %d: saw \"%s\", which must not appear"), Reject.Line, *Reject.Text));
			return false;
		}
		Reject.CheckedUpTo = Capture->Num();
	}
	return true;
}

void UClaudeTestScenarioRunner::Finish(bool bPassed, const FString& Reason)
{
	bRunning = false;
	if (TickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
		TickerHandle.Reset();
	}
	if (Capture)
	{
		GLog->RemoveOutputDevice(Capture.Get());
		Capture.Reset();
	}

	LastResult = bPassed ? TEXT("Passed") : TEXT("Failed");
	LastReason = bPassed ? FString() : Reason;
	const FString Result = bPassed
		? FString::Printf(TEXT("SCENARIO %s PASSED"), *Name)
		: FString::Printf(TEXT("SCENARIO %s FAILED: %s"), *Name, *Reason);
	if (bPassed)
	{
		UE_LOG(LogScenario, Display, TEXT("%s"), *Result);
	}
	else
	{
		UE_LOG(LogScenario, Error, TEXT("%s"), *Result);
	}
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(ClaudeTestScenarioRunnerPrivate::ScreenKey, ClaudeTestScenarioRunnerPrivate::ResultSeconds,
			bPassed ? FColor(120, 230, 120) : FColor(255, 110, 90), Result);
	}

	if (bExitWhenDone && !GIsEditor)
	{
		// The exit code tells whoever started the game how it went.
		FPlatformMisc::RequestExitWithStatus(false, bPassed ? 0 : 1);
	}
}

UWorld* UClaudeTestScenarioRunner::GetGameWorld() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetWorld() : nullptr;
}

#if !UE_BUILD_SHIPPING
namespace ClaudeTestScenarioConsole
{
	FAutoConsoleCommandWithWorldArgsAndOutputDevice ScenarioCommand(
		TEXT("Test.Scenario"),
		TEXT("Test.Scenario <name> [exit]: runs the game test <project>/Tests/Scenarios/<name>.scenario. With exit, the game closes when it's done (not in the editor)."),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
		{
			if (Args.IsEmpty())
			{
				Ar.Log(ELogVerbosity::Warning, TEXT("Test.Scenario: give a scenario name, e.g. Test.Scenario my_test."));
				return;
			}
			UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
			UClaudeTestScenarioRunner* Runner = GameInstance ? GameInstance->GetSubsystem<UClaudeTestScenarioRunner>() : nullptr;
			if (!Runner)
			{
				Ar.Log(ELogVerbosity::Warning, TEXT("Test.Scenario: run this while playing."));
				return;
			}
			const bool bExit = Args.Num() > 1 && Args[1].Equals(TEXT("exit"), ESearchCase::IgnoreCase);
			Runner->Start(Args[0], bExit);
		}));
}
#endif
