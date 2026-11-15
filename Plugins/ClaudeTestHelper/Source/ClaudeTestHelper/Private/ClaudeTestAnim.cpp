#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "ClaudeTestFreeze.h"
#include "Components/SkeletalMeshComponent.h"
#include "ClaudeTestOutput.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "UObject/UObjectIterator.h"

// Anim.Freeze: holds every character still, for screenshot tests. Characters in a lit level (one in front of the camera)
// otherwise catch their idle animation at a different moment each run, and the picture changes.
namespace ClaudeTestFreeze
{
	/** Game code reads this by name (no link to this plugin): IConsoleManager::Get().FindConsoleVariable(TEXT("Test.LoopsFrozen")). */
	static TAutoConsoleVariable<int32> CVarLoopsFrozen(
		TEXT("Test.LoopsFrozen"),
		0,
		TEXT("Test.LoopsFrozen [0|1]: 1 while Anim.Freeze holds things still; looping motion (menu pulses, drifting backgrounds) should hold at its start when it's 1."));

	bool AreLoopsFrozen()
	{
		return CVarLoopsFrozen.GetValueOnGameThread() != 0;
	}

	void SetLoopsFrozen(bool bFrozen)
	{
		CVarLoopsFrozen->Set(bFrozen ? 1 : 0, ECVF_SetByConsole);
	}
}

namespace ClaudeTestAnim
{
	FAutoConsoleCommandWithWorldArgsAndOutputDevice FreezeCommand(
		TEXT("Anim.Freeze"),
		TEXT("Anim.Freeze [off]: holds every character's animation at its very start (the same pose every run), for screenshot tests. 'off' lets them move again."),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
		{
			const bool bFreeze = Args.IsEmpty() || !Args[0].Equals(TEXT("off"), ESearchCase::IgnoreCase);
			int32 Count = 0;
			for (TObjectIterator<USkeletalMeshComponent> It; It; ++It)
			{
				USkeletalMeshComponent* Mesh = *It;
				if (Mesh->GetWorld() != World || Mesh->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject) || !Mesh->IsRegistered())
				{
					continue;
				}
				if (bFreeze)
				{
					// Back to the start of its animation, one update to take that pose, then held there.
					Mesh->InitAnim(/*bForceReinit*/ true);
					Mesh->TickAnimation(0.f, false);
					Mesh->RefreshBoneTransforms();
				}
				Mesh->bPauseAnims = bFreeze;
				++Count;
			}
			// Looping UI motion (menu selection pulses, a still background's drift) holds at its start too.
			ClaudeTestFreeze::SetLoopsFrozen(bFreeze);
			ClaudeTestOutput::Line(Ar, FString::Printf(TEXT("Anim: %s %d character mesh%s."), bFreeze ? TEXT("froze") : TEXT("unfroze"), Count, Count == 1 ? TEXT("") : TEXT("es")));
		}));
}

#endif
