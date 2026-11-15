#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ClaudeTestHelperReader.generated.h"

/** Where the game-test runner is: the scenario running (or the last one) and how it ended. */
USTRUCT(BlueprintType)
struct FClaudeTestScenarioStatus
{
	GENERATED_BODY()

	/** True while a scenario runs. */
	UPROPERTY(BlueprintReadOnly, Category = "Claude Test Helper")
	bool bRunning = false;

	/** The scenario running, or the last one that ran; empty if none has. */
	UPROPERTY(BlueprintReadOnly, Category = "Claude Test Helper")
	FString Name;

	/** Passed or Failed for the last scenario; empty while one runs or if none has. */
	UPROPERTY(BlueprintReadOnly, Category = "Claude Test Helper")
	FString LastResult;

	/** Why the last scenario failed ("line 3: didn't see ..."); empty when it passed. */
	UPROPERTY(BlueprintReadOnly, Category = "Claude Test Helper")
	FString LastReason;
};

/**
 * Reads the test helper's state without changing anything (every function is const), for Reader.Read in game tests and for tools
 * such as UE5-Tools' Live Control over Remote Control. One per game world, at "<world>:ClaudeTestHelperReader_0".
 * Changes go through the console commands: Test.Scenario, Anim.Freeze, Input.*.
 */
UCLASS(meta = (DisplayName = "Claude Test Helper Reader", LiveControl))
class CLAUDETESTHELPER_API UClaudeTestHelperReader : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** The game-test runner's state: running, the scenario's name, and how the last one ended. Empty with no game instance. */
	UFUNCTION(BlueprintPure, Category = "Claude Test Helper")
	FClaudeTestScenarioStatus GetScenarioStatus() const;

	/** True while Anim.Freeze holds animation and looping motion still (the Test.LoopsFrozen console variable is 1). */
	UFUNCTION(BlueprintPure, Category = "Claude Test Helper")
	bool AreAnimationsFrozen() const;
};
