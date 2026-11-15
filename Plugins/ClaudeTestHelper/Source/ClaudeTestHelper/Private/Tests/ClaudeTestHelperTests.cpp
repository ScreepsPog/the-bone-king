#include "ClaudeTestFreeze.h"
#include "ClaudeTestHelperReader.h"
#include "ClaudeTestReaderCall.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/WorldSettings.h"
#include "HAL/IConsoleManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/OutputDeviceNull.h"
#include "Misc/StringOutputDevice.h"

namespace ClaudeTestHelperTests
{
	constexpr EAutomationTestFlags HelperTestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

	/** A game world for one test, with no player in it: begin play is started by hand. */
	struct FHelperTestWorld
	{
		UWorld* World = nullptr;

		FHelperTestWorld()
		{
			World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld*/ false, TEXT("ClaudeTestHelperWorld"));
			FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
			Context.SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());
			World->BeginPlay();
			if (!World->HasBegunPlay())
			{
				World->GetWorldSettings()->NotifyBeginPlay();
			}
		}

		~FHelperTestWorld()
		{
			if (World->HasBegunPlay())
			{
				World->EndPlay(EEndPlayReason::Quit);
			}
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld*/ false);
		}

		FHelperTestWorld(const FHelperTestWorld&) = delete;
		FHelperTestWorld& operator=(const FHelperTestWorld&) = delete;
	};

	/** What a console command printed, run the way the console runs it in World. */
	FString Run(UWorld* World, const TCHAR* Command)
	{
		FStringOutputDevice Out;
		Out.SetAutoEmitLineTerminator(true);
		IConsoleManager::Get().ProcessUserConsoleInput(Command, Out, World);
		return Out;
	}
}

using namespace ClaudeTestHelperTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClaudeTestHelperReaderFound, "ClaudeTestHelper.Reader.FoundAndReadOnly", HelperTestFlags)
bool FClaudeTestHelperReaderFound::RunTest(const FString& Parameters)
{
	FHelperTestWorld Level;
	UObject* Reader = Level.World->GetSubsystemBase(UClaudeTestHelperReader::StaticClass());
	if (!TestNotNull(TEXT("the Reader is in every game world"), Reader))
	{
		return false;
	}
	TestEqual(TEXT("at Live Control's address"), Reader->GetPathName(), Level.World->GetPathName() + TEXT(":ClaudeTestHelperReader_0"));
	TestTrue(TEXT("found by name, with or without the U"), ClaudeTestReaderCall::FindReader(Level.World, TEXT("ClaudeTestHelperReader")) == Reader
		&& ClaudeTestReaderCall::FindReader(Level.World, TEXT("UClaudeTestHelperReader")) == Reader);

	int32 Count = 0;
	for (TFieldIterator<UFunction> It(UClaudeTestHelperReader::StaticClass(), EFieldIteratorFlags::ExcludeSuper); It; ++It)
	{
		++Count;
		TestTrue(*FString::Printf(TEXT("%s is const (read-only)"), *It->GetName()), It->HasAnyFunctionFlags(FUNC_Const));
	}
	TestEqual(TEXT("two reads"), Count, 2);

	FString Json, Problem;
	TestTrue(TEXT("GetScenarioStatus reads"), ClaudeTestReaderCall::Call(Reader, TEXT("GetScenarioStatus"), {}, Json, Problem));
	TestEqual(TEXT("no game instance: nothing running"), Json, FString(TEXT("{\"ReturnValue\":{\"bRunning\":false,\"Name\":\"\",\"LastResult\":\"\",\"LastReason\":\"\"}}")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClaudeTestHelperCallOnlyReads, "ClaudeTestHelper.Reader.CallOnlyReads", HelperTestFlags)
bool FClaudeTestHelperCallOnlyReads::RunTest(const FString& Parameters)
{
	FHelperTestWorld Level;
	FString Json, Problem;
	UObject* Reader = ClaudeTestReaderCall::FindReader(Level.World, TEXT("ClaudeTestHelperReader"));
	TestFalse(TEXT("an unknown function is refused"), ClaudeTestReaderCall::Call(Reader, TEXT("NoSuchRead"), {}, Json, Problem));
	// Refused before it runs: the default actor is never destroyed.
	TestFalse(TEXT("a function that isn't const is refused"), ClaudeTestReaderCall::Call(GetMutableDefault<AActor>(), TEXT("K2_DestroyActor"), {}, Json, Problem));
	TestTrue(*FString::Printf(TEXT("and says why (%s)"), *Problem), Problem.Contains(TEXT("isn't const")));
	TestNull(TEXT("an unknown Reader isn't found"), ClaudeTestReaderCall::FindReader(Level.World, TEXT("NoSuchReader")));
	TestNull(TEXT("a class that isn't a world subsystem isn't found"), ClaudeTestReaderCall::FindReader(Level.World, TEXT("Actor")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClaudeTestHelperFreeze, "ClaudeTestHelper.Commands.AnimFreezeSetsTheFlagAndReaderSeesIt", HelperTestFlags)
bool FClaudeTestHelperFreeze::RunTest(const FString& Parameters)
{
	FHelperTestWorld Level;
	ClaudeTestFreeze::SetLoopsFrozen(false);
	const FString Froze = Run(Level.World, TEXT("Anim.Freeze"));
	TestTrue(*FString::Printf(TEXT("replies (%s)"), *Froze), Froze.Contains(TEXT("Anim: froze 0 character meshes.")));
	TestTrue(TEXT("the flag is on"), ClaudeTestFreeze::AreLoopsFrozen());
	const IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(TEXT("Test.LoopsFrozen"));
	TestTrue(TEXT("game code can read it by name"), Variable && Variable->GetInt() == 1);
	const FString Read = Run(Level.World, TEXT("Reader.Read ClaudeTestHelperReader AreAnimationsFrozen"));
	TestTrue(*FString::Printf(TEXT("Reader.Read prints JSON (%s)"), *Read), Read.Contains(TEXT("Reader ClaudeTestHelperReader.AreAnimationsFrozen: {\"ReturnValue\":true}")));

	const FString Unfroze = Run(Level.World, TEXT("Anim.Freeze off"));
	TestTrue(*FString::Printf(TEXT("off replies (%s)"), *Unfroze), Unfroze.Contains(TEXT("Anim: unfroze 0 character meshes.")));
	TestFalse(TEXT("the flag is off"), ClaudeTestFreeze::AreLoopsFrozen());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClaudeTestHelperBadArguments, "ClaudeTestHelper.Commands.SayWhatsWrongWithTheirArguments", HelperTestFlags)
bool FClaudeTestHelperBadArguments::RunTest(const FString& Parameters)
{
	FHelperTestWorld Level;
	struct FCase
	{
		const TCHAR* Command;
		const TCHAR* Reply;
	};
	const FCase Cases[] =
	{
		{ TEXT("Input.Press NotAKey"), TEXT("'NotAKey' isn't a key Unreal knows") },
		{ TEXT("Input.Wheel"), TEXT("give a number of notches") },
		{ TEXT("Input.Wheel up"), TEXT("give a number of notches") },
		{ TEXT("Input.Stick Gamepad_LeftY"), TEXT("give a stick axis and how far") },
		{ TEXT("Input.Stick SpaceBar 1"), TEXT("give a stick axis and how far") },
		{ TEXT("Input.Keys"), TEXT("Input.Keys:") },
		{ TEXT("Reader.Read"), TEXT("give a Reader") },
		{ TEXT("Reader.Read NoSuchReader GetX"), TEXT("no Reader called NoSuchReader") },
		{ TEXT("Reader.Read ClaudeTestHelperReader NoSuchRead"), TEXT("has no function NoSuchRead") },
		{ TEXT("Test.Scenario"), TEXT("give a scenario name") },
	};
	for (const FCase& Case : Cases)
	{
		const FString Reply = Run(Level.World, Case.Command);
		TestTrue(FString::Printf(TEXT("%s -> \"%s\" (got \"%s\")"), Case.Command, Case.Reply, *Reply.TrimEnd()), Reply.Contains(Case.Reply));
	}
	const FString List = Run(Level.World, TEXT("Reader.Read ClaudeTestHelperReader"));
	TestTrue(*FString::Printf(TEXT("a Reader alone lists its reads (%s)"), *List), List.Contains(TEXT("GetScenarioStatus")) && List.Contains(TEXT("AreAnimationsFrozen")));
	return true;
}

// Live Control and Claude read a command's arguments from its help: `Name <required> [optional=default]: what it does`.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClaudeTestHelperHelpForm, "ClaudeTestHelper.Commands.HelpTextStartsWithNameAndArguments", HelperTestFlags)
bool FClaudeTestHelperHelpForm::RunTest(const FString& Parameters)
{
	const TCHAR* Names[] = { TEXT("Test.Scenario"), TEXT("Input.Press"), TEXT("Input.Wheel"), TEXT("Input.Stick"), TEXT("Input.Keys"),
		TEXT("Input.Held"), TEXT("Input.Rebind"), TEXT("Anim.Freeze"), TEXT("Reader.Read"), TEXT("Test.LoopsFrozen") };
	for (const TCHAR* Name : Names)
	{
		const IConsoleObject* Object = IConsoleManager::Get().FindConsoleObject(Name);
		if (!TestNotNull(*FString::Printf(TEXT("%s is registered"), Name), Object))
		{
			continue;
		}
		const FString Help = Object->GetHelp();
		FString Head, Rest;
		const bool bHasColon = Help.Split(TEXT(": "), &Head, &Rest);
		TestTrue(*FString::Printf(TEXT("%s help starts with its name (%s)"), Name, *Help.Left(60)), Help.StartsWith(FString(Name)));
		TestTrue(*FString::Printf(TEXT("%s help has ': ' after the arguments"), Name), bHasColon && !Rest.IsEmpty());
		// After the name only arguments: " <required>" or " [optional]" (spaces allowed inside, like <mapping name>).
		const FString Arguments = Head.RightChop(FCString::Strlen(Name));
		int32 Index = 0;
		bool bForm = true;
		while (Index < Arguments.Len() && bForm)
		{
			const TCHAR Open = Index + 1 < Arguments.Len() ? Arguments[Index + 1] : 0;
			const TCHAR Close = Open == TEXT('<') ? TEXT('>') : Open == TEXT('[') ? TEXT(']') : 0;
			const int32 End = Close ? Arguments.Find(FString::Chr(Close), ESearchCase::CaseSensitive, ESearchDir::FromStart, Index + 2) : INDEX_NONE;
			bForm = Arguments[Index] == TEXT(' ') && End != INDEX_NONE;
			Index = End + 1;
		}
		TestTrue(*FString::Printf(TEXT("%s: arguments are each in <> or [] ('%s')"), Name, *Arguments), bForm);
	}
	return true;
}

#endif
