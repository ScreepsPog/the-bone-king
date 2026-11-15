#include "ClaudeTestHelperReader.h"

#include "ClaudeTestFreeze.h"
#include "ClaudeTestScenarioRunner.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

FClaudeTestScenarioStatus UClaudeTestHelperReader::GetScenarioStatus() const
{
	FClaudeTestScenarioStatus Status;
	const UWorld* World = GetWorld();
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	if (const UClaudeTestScenarioRunner* Runner = GameInstance ? GameInstance->GetSubsystem<UClaudeTestScenarioRunner>() : nullptr)
	{
		Status.bRunning = Runner->IsRunning();
		Status.Name = Runner->GetScenarioName();
		Status.LastResult = Runner->GetLastResult();
		Status.LastReason = Runner->GetLastReason();
	}
	return Status;
}

bool UClaudeTestHelperReader::AreAnimationsFrozen() const
{
	return ClaudeTestFreeze::AreLoopsFrozen();
}
