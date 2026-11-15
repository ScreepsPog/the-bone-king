#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "ClaudeTestScenario.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ClaudeTestScenarioRunner.generated.h"

/**
 * Runs a game test script (see ClaudeTestScenario.h) while playing: "Test.Scenario <name>" in the console, or
 * "ue.py game <name>" (the unreal-toolkit Claude plugin) to run it with no window. It survives level changes, so a script can travel.
 * Ends with "SCENARIO <name> PASSED" or "SCENARIO <name> FAILED: line N: why" in the log and on screen.
 * Not in shipping builds.
 */
UCLASS()
class UClaudeTestScenarioRunner : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Deinitialize() override;

	/** Starts a scenario by name (from <project>/Tests/Scenarios) or by full path. With Exit When Done, the game closes afterwards (not in the editor). */
	void Start(const FString& NameOrPath, bool bExitWhenDone);

	bool IsRunning() const { return bRunning; }

	/** The scenario running, or the last one that ran; empty if none has. */
	const FString& GetScenarioName() const { return Name; }

	/** How the last scenario ended: Passed or Failed (empty while one runs, or if none has), and why it failed. */
	const FString& GetLastResult() const { return LastResult; }
	const FString& GetLastReason() const { return LastReason; }

private:
	/** Collects log lines while a scenario runs. Log lines can come from any thread. */
	class FLogCapture : public FOutputDevice
	{
	public:
		virtual void Serialize(const TCHAR* Message, ELogVerbosity::Type Verbosity, const FName& Category) override;
		virtual bool CanBeUsedOnAnyThread() const override { return true; }
		virtual bool CanBeUsedOnMultipleThreads() const override { return true; }

		int32 Num() const;
		/** Index of the first line at or after From containing Text, or INDEX_NONE. */
		int32 Find(const FString& Text, int32 From) const;
		/** Index of the first line at or after From with Text and a number from Min to Max right after it, or INDEX_NONE.
		 *  OutLastSeen gets the last number seen after Text that wasn't in range. */
		int32 FindNumber(const FString& Text, int32 From, double Min, double Max, TOptional<double>& OutLastSeen) const;

	private:
		mutable FCriticalSection Lock;
		TArray<FString> Lines;
	};

	struct FReject
	{
		FString Text;
		int32 CheckedUpTo = 0;
		int32 Line = 0;
	};

	bool Tick(float DeltaTime);
	/** Runs steps until one has to wait. False once the scenario has finished. */
	bool Advance();
	bool CheckRejects();
	/** A shot step: asks for the screenshot, then waits for the file. True once it's done (or skipped outside uishots runs). */
	bool TakeShot(const ClaudeTestScenario::FStep& Step, double Now);
	/** A remote step: sends the request, then waits for the reply and logs it. True once it's logged. */
	bool SendRemote(const ClaudeTestScenario::FStep& Step, double Now);
	void Finish(bool bPassed, const FString& Reason);
	UWorld* GetGameWorld() const;

	FString Name;
	TArray<ClaudeTestScenario::FStep> Steps;
	int32 StepIndex = 0;
	double StepStartTime = 0.0;
	bool bStepStarted = false;
	/** Where the next expect starts looking: past the last match, and never before the last command. */
	int32 Cursor = 0;
	TArray<FReject> Rejects;
	TUniquePtr<FLogCapture> Capture;
	FTSTicker::FDelegateHandle TickerHandle;
	/** The screenshot file a shot step is waiting for, and its size last tick. */
	FString PendingShot;
	int64 PendingShotSize = -1;
	/** The remote step's request while it waits, and its reply once in (code 0: no answer). */
	TSharedPtr<class IHttpRequest, ESPMode::ThreadSafe> RemoteRequest;
	bool bRemoteReplied = false;
	int32 RemoteCode = 0;
	FString RemoteBody;
	FString LastResult;
	FString LastReason;
	bool bRunning = false;
	bool bExitWhenDone = false;
};
