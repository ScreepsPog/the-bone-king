#include "CoreMinimal.h"
#include "Engine/World.h"
#include "Game/CrowdSubsystem.h"
#include "Game/WizardAudioSubsystem.h"
#include "Game/WizardHUD.h"
#include "Engine/GameInstance.h"
#include "Game/WizardPawn.h"
#include "Game/WizardSettingsSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "WebTesting.h"

// Every change a tester would make has a command here. Each replies with one log line that starts "Wiz:".
namespace
{
	UCrowdSubsystem* GetCrowd(UWorld* World)
	{
		UCrowdSubsystem* Crowd = World ? World->GetSubsystem<UCrowdSubsystem>() : nullptr;
		if (!Crowd)
		{
			UE_LOG(LogWizard, Warning, TEXT("Wiz: no game is running in this world"));
		}
		return Crowd;
	}

	AWizardHUD* GetWizardHUD(UWorld* World)
	{
		const APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
		return PlayerController ? Cast<AWizardHUD>(PlayerController->GetHUD()) : nullptr;
	}

	/** "0" is off, "1" is on, no argument flips the current value. */
	bool ReadSwitch(const TArray<FString>& Args, bool bCurrent)
	{
		return Args.Num() > 0 ? FCString::Atoi(*Args[0]) != 0 : !bCurrent;
	}

	const TCHAR* OnOff(bool bValue)
	{
		return bValue ? TEXT("on") : TEXT("off");
	}

	FAutoConsoleCommandWithWorldAndArgs CameraCommand(
		TEXT("Wiz.Camera"),
		TEXT("Wiz.Camera <Angled|TopDown|ThirdPerson>: switches the camera mode. No argument prints the current mode."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UCrowdSubsystem* Crowd = GetCrowd(World);
			AWizardPawn* Wizard = Crowd ? Crowd->GetWizard() : nullptr;
			if (!Wizard)
			{
				return;
			}
			if (Args.Num() > 0)
			{
				EWizardCameraMode Mode;
				if (!AWizardPawn::ParseCameraMode(Args[0], Mode))
				{
					UE_LOG(LogWizard, Warning, TEXT("Wiz: unknown camera '%s'. Use Angled, TopDown or ThirdPerson."), *Args[0]);
					return;
				}
				Wizard->SetCameraMode(Mode);
			}
			UE_LOG(LogWizard, Display, TEXT("Wiz: camera is %s"), *AWizardPawn::GetCameraModeName(Wizard->GetCameraMode()));
		}));

	FAutoConsoleCommandWithWorldAndArgs DebugCommand(
		TEXT("Wiz.Debug"),
		TEXT("Wiz.Debug [0|1=toggle]: shows or hides the debug overlay."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (AWizardHUD* Hud = GetWizardHUD(World))
			{
				Hud->SetDebugVisible(ReadSwitch(Args, Hud->IsDebugVisible()));
				UE_LOG(LogWizard, Display, TEXT("Wiz: debug overlay is %s"), OnOff(Hud->IsDebugVisible()));
			}
			else
			{
				UE_LOG(LogWizard, Warning, TEXT("Wiz: no HUD in this world"));
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs SpawnEnemiesCommand(
		TEXT("Wiz.SpawnEnemies"),
		TEXT("Wiz.SpawnEnemies <Count> [Kind=Grunt] [X Y]: spawns enemies around the wizard, or at X Y when given (Count 1-120; Kind: Grunt, Brute, Archer, Assassin)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UCrowdSubsystem* Crowd = GetCrowd(World);
			if (!Crowd)
			{
				return;
			}
			if (Args.Num() < 1 || FCString::Atoi(*Args[0]) < 1)
			{
				UE_LOG(LogWizard, Warning, TEXT("Wiz: usage Wiz.SpawnEnemies <Count> [Kind=Grunt], Count 1-120"));
				return;
			}
			ECrowdKind Kind = ECrowdKind::Grunt;
			const TCHAR* Plural = TEXT("grunts");
			if (Args.Num() > 1 && Args[1].Equals(TEXT("Brute"), ESearchCase::IgnoreCase))
			{
				Kind = ECrowdKind::Brute;
				Plural = TEXT("brutes");
			}
			else if (Args.Num() > 1 && Args[1].Equals(TEXT("Archer"), ESearchCase::IgnoreCase))
			{
				Kind = ECrowdKind::Archer;
				Plural = TEXT("archers");
			}
			else if (Args.Num() > 1 && Args[1].Equals(TEXT("Assassin"), ESearchCase::IgnoreCase))
			{
				Kind = ECrowdKind::Assassin;
				Plural = TEXT("assassins");
			}
			else if (Args.Num() > 1 && Args[1].Equals(TEXT("Commander"), ESearchCase::IgnoreCase))
			{
				Kind = ECrowdKind::Commander;
				Plural = TEXT("commanders");
			}
			else if (Args.Num() > 1 && !Args[1].Equals(TEXT("Grunt"), ESearchCase::IgnoreCase))
			{
				UE_LOG(LogWizard, Warning, TEXT("Wiz: unknown kind '%s'. Use Grunt, Brute, Archer, Assassin or Commander."), *Args[1]);
				return;
			}
			const int32 Asked = FMath::Clamp(FCString::Atoi(*Args[0]), 1, 120);
			const int32 Spawned = Args.Num() >= 4
				? Crowd->SpawnEnemiesAt(Asked, Kind, FVector2D(FCString::Atod(*Args[2]), FCString::Atod(*Args[3])))
				: Crowd->SpawnEnemies(Asked, Kind);
			UE_LOG(LogWizard, Display, TEXT("Wiz: spawned %d of %d %s, %d enemies now"), Spawned, Asked, Plural, Crowd->GetEnemyCount());
		}));

	FAutoConsoleCommandWithWorldAndArgs AddSkeletonsCommand(
		TEXT("Wiz.AddSkeletons"),
		TEXT("Wiz.AddSkeletons <Count> [Kind=Skeleton]: adds units to the army (Count 1-40; Kind: Skeleton, Guard)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UCrowdSubsystem* Crowd = GetCrowd(World);
			if (!Crowd)
			{
				return;
			}
			if (Args.Num() < 1 || FCString::Atoi(*Args[0]) < 1)
			{
				UE_LOG(LogWizard, Warning, TEXT("Wiz: usage Wiz.AddSkeletons <Count>, Count 1-40"));
				return;
			}
			const int32 Asked = FMath::Clamp(FCString::Atoi(*Args[0]), 1, 40);
			const bool bGuard = Args.Num() > 1 && Args[1].Equals(TEXT("Guard"), ESearchCase::IgnoreCase);
			const bool bMarksman = Args.Num() > 1 && Args[1].Equals(TEXT("Marksman"), ESearchCase::IgnoreCase);
			if (Args.Num() > 1 && !bGuard && !bMarksman && !Args[1].Equals(TEXT("Skeleton"), ESearchCase::IgnoreCase))
			{
				UE_LOG(LogWizard, Warning, TEXT("Wiz: unknown kind '%s'. Use Skeleton, Guard or Marksman."), *Args[1]);
				return;
			}
			const int32 Added = Crowd->AddSkeletons(Asked, bGuard ? ECrowdKind::Guard : bMarksman ? ECrowdKind::Marksman : ECrowdKind::Skeleton);
			UE_LOG(LogWizard, Display, TEXT("Wiz: added %d of %d %s, army is %d"), Added, Asked,
				bGuard ? TEXT("guards") : bMarksman ? TEXT("marksmen") : TEXT("skeletons"), Crowd->GetArmyCount());
		}));

	FAutoConsoleCommandWithWorldAndArgs WavesCommand(
		TEXT("Wiz.Waves"),
		TEXT("Wiz.Waves [0|1=toggle]: turns automatic enemy waves on or off."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UCrowdSubsystem* Crowd = GetCrowd(World))
			{
				Crowd->SetWavesEnabled(ReadSwitch(Args, Crowd->AreWavesEnabled()));
				UE_LOG(LogWizard, Display, TEXT("Wiz: waves are %s"), OnOff(Crowd->AreWavesEnabled()));
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs GodCommand(
		TEXT("Wiz.God"),
		TEXT("Wiz.God [0|1=toggle]: stops the Overwhelm meter from filling."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UCrowdSubsystem* Crowd = GetCrowd(World))
			{
				Crowd->SetGodMode(ReadSwitch(Args, Crowd->IsGodMode()));
				UE_LOG(LogWizard, Display, TEXT("Wiz: god mode is %s"), OnOff(Crowd->IsGodMode()));
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs ObstaclesCommand(
		TEXT("Wiz.Obstacles"),
		TEXT("Wiz.Obstacles [0|1=toggle]: puts the practice walls in or takes them out. Stand in the open when switching them on."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UCrowdSubsystem* Crowd = GetCrowd(World))
			{
				Crowd->SetObstaclesEnabled(ReadSwitch(Args, Crowd->AreObstaclesEnabled()));
				UE_LOG(LogWizard, Display, TEXT("Wiz: obstacles are %s (%d walls)"), OnOff(Crowd->AreObstaclesEnabled()), Crowd->GetObstacleCount());
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs ThrowSpearCommand(
		TEXT("Wiz.ThrowSpear"),
		TEXT("Wiz.ThrowSpear [X Y=600 units along the aim]: throws the Spear of Sacrifice at that point on the ground. It costs 3 skeletons and must be unlocked (Wiz.Buy Spear or Wiz.UnlockSpear 1)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UCrowdSubsystem* Crowd = GetCrowd(World);
			const AWizardPawn* Wizard = Crowd ? Crowd->GetWizard() : nullptr;
			if (!Wizard)
			{
				return;
			}
			const FVector2D Target = Args.Num() >= 2
				? FVector2D(FCString::Atof(*Args[0]), FCString::Atof(*Args[1]))
				: Wizard->GetPosition2D() + Wizard->GetAimDirection() * 600.0;
			if (Crowd->ThrowSpear(Target))
			{
				UE_LOG(LogWizard, Display, TEXT("Wiz: spear thrown at %.0f, %.0f (army %d now)"), Target.X, Target.Y, Crowd->GetArmyCount());
			}
			else
			{
				UE_LOG(LogWizard, Display, TEXT("Wiz: spear not thrown (%s, needs %d skeletons, cooldown %.1f s)"),
					Crowd->IsSpearUnlocked() ? TEXT("unlocked") : TEXT("locked"), Crowd->GetSpearSkeletonCost(), Crowd->GetSpearCooldown());
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs BlinkCommand(
		TEXT("Wiz.Blink"),
		TEXT("Wiz.Blink [X Y=the way he moves, or faces when standing]: the wizard blinks along that direction (Wiz.Tune BlinkRange 700, BlinkSeconds 0.5, BlinkCooldown 3, BlinkSteer 0.35)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UCrowdSubsystem* Crowd = GetCrowd(World);
			const AWizardPawn* Wizard = Crowd ? Crowd->GetWizard() : nullptr;
			if (!Wizard)
			{
				return;
			}
			const FVector2D Direction = (Args.Num() >= 2 ? FVector2D(FCString::Atof(*Args[0]), FCString::Atof(*Args[1]))
				: Wizard->GetMoveDirection().IsNearlyZero() ? Wizard->GetAimDirection() : Wizard->GetMoveDirection()).GetSafeNormal();
			if (Crowd->StartBlink(Direction))
			{
				UE_LOG(LogWizard, Display, TEXT("Wiz: blink along %.1f, %.1f"), Direction.X, Direction.Y);
			}
			else
			{
				UE_LOG(LogWizard, Display, TEXT("Wiz: blink not cast (%s, cooldown %.1f s)"), *Crowd->GetBlinkStateName(), Crowd->GetBlinkCooldown());
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs CommandCommand(
		TEXT("Wiz.Command"),
		TEXT("Wiz.Command [Count=half the army] [Kind=Any] [X Y=the aim point]: sends that many of the army (Kind: Any, Skeleton, Guard) to hold that place. They rejoin when the wizard comes within CommandRejoinRadius (Wiz.Tune, 300) of it."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UCrowdSubsystem* Crowd = GetCrowd(World);
			const AWizardPawn* Wizard = Crowd ? Crowd->GetWizard() : nullptr;
			if (!Wizard)
			{
				return;
			}
			ECrowdCommandKind Kind = ECrowdCommandKind::Any;
			if (Args.Num() > 1 && Args[1].Equals(TEXT("Skeleton"), ESearchCase::IgnoreCase))
			{
				Kind = ECrowdCommandKind::Skeleton;
			}
			else if (Args.Num() > 1 && Args[1].Equals(TEXT("Guard"), ESearchCase::IgnoreCase))
			{
				Kind = ECrowdCommandKind::Guard;
			}
			else if (Args.Num() > 1 && !Args[1].Equals(TEXT("Any"), ESearchCase::IgnoreCase))
			{
				UE_LOG(LogWizard, Warning, TEXT("Wiz: unknown kind '%s'. Use Any, Skeleton or Guard."), *Args[1]);
				return;
			}
			const int32 Count = Args.Num() > 0 ? FMath::Max(FCString::Atoi(*Args[0]), 0) : (Crowd->GetSim().CountCommandable(Kind) + 1) / 2;
			const FVector2D Point = Args.Num() >= 4 ? FVector2D(FCString::Atof(*Args[2]), FCString::Atof(*Args[3])) : Wizard->GetAimPoint2D();
			const int32 Sent = Crowd->SendCommand(Point, Count, Kind);
			UE_LOG(LogWizard, Display, TEXT("Wiz: sent %d to %.0f, %.0f (%d sent in all, %d with the wizard)"), Sent, Point.X, Point.Y,
				Crowd->GetCommandedCount(), Crowd->GetArmyCount() - Crowd->GetCommandedCount());
		}));

	FAutoConsoleCommandWithWorldAndArgs CommandModeCommand(
		TEXT("Wiz.CommandMode"),
		TEXT("Wiz.CommandMode [0|1=toggle]: enters or leaves command mode (the F key). It does not enter with no army to send."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UCrowdSubsystem* Crowd = GetCrowd(World))
			{
				Crowd->SetCommandMode(ReadSwitch(Args, Crowd->IsCommandMode()));
				UE_LOG(LogWizard, Display, TEXT("Wiz: command mode is %s (send %d of %d, kind %s)"), OnOff(Crowd->IsCommandMode()),
					Crowd->GetCommandCount(), Crowd->GetCommandAvailable(), *Crowd->GetCommandKindName());
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs RecallCommand(
		TEXT("Wiz.Recall"),
		TEXT("Wiz.Recall: every sent unit comes back to the wizard (the C key)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UCrowdSubsystem* Crowd = GetCrowd(World))
			{
				UE_LOG(LogWizard, Display, TEXT("Wiz: recalled %d"), Crowd->Recall());
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs AssassinCommand(
		TEXT("Wiz.Assassin"),
		TEXT("Wiz.Assassin: sends one assassin now from the nearest manned post (Wiz.Tune AssassinInterval sets how often one comes by itself; 0 = never)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UCrowdSubsystem* Crowd = GetCrowd(World))
			{
				UE_LOG(LogWizard, Display, TEXT("Wiz: assassin %s (%d sent in this run)"), Crowd->SendAssassin() ? TEXT("sent") : TEXT("not sent: no manned post"), Crowd->GetAssassinsSent());
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs KeyCommand(
		TEXT("Wiz.Key"),
		TEXT("Wiz.Key <Control> <Key> | capture <Control> | reset: puts a control on another key, as the Controls tab does (two controls swap when the key is taken; R, Enter, Escape, the wheel and the testing keys are refused). Controls: MoveForward, MoveLeft, MoveBack, MoveRight, Bolt, Shrine, Pause, Summon, Spear, Blink, Command, Recall. Keys by the engine's names: G, LeftShift, SpaceBar, LeftMouseButton, MiddleMouseButton, ThumbMouseButton. capture makes the tab wait for a key press; reset puts every key back."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UCrowdSubsystem* Crowd = GetCrowd(World);
			AWizardPawn* Wizard = Crowd ? Crowd->GetWizard() : nullptr;
			if (!Wizard || Args.Num() == 0)
			{
				return;
			}
			const auto Find = [](const FString& Text)
			{
				for (const FName& Action : FWizardKeyMap::Changeable())
				{
					if (Action.ToString().Replace(TEXT(" "), TEXT("")).Equals(Text.Replace(TEXT(" "), TEXT("")), ESearchCase::IgnoreCase))
					{
						return Action;
					}
				}
				return FName(*Text);
			};
			const TCHAR* Results[] = { TEXT("set"), TEXT("swapped"), TEXT("same"), TEXT("refused") };
			FString Result(TEXT("none"));
			if (Args[0].Equals(TEXT("reset"), ESearchCase::IgnoreCase))
			{
				Wizard->ResetKeys();
				Result = TEXT("reset");
			}
			else if (Args[0].Equals(TEXT("capture"), ESearchCase::IgnoreCase) && Args.Num() > 1)
			{
				Wizard->BeginKeyCapture(Find(Args[1]));
				Result = Wizard->GetCapturingAction().IsNone() ? TEXT("refused") : TEXT("waiting");
			}
			else if (Args.Num() > 1)
			{
				Result = Results[static_cast<int32>(Wizard->RebindKey(Find(Args[0]), FKey(*Args[1])))];
			}
			FString List;
			for (const FName& Action : FWizardKeyMap::Changeable())
			{
				List += FString::Printf(TEXT("%s%s=%s"), List.IsEmpty() ? TEXT("") : TEXT(", "), *Action.ToString(), *WizardKeys().Get(Action).ToString());
			}
			UE_LOG(LogWizard, Display, TEXT("Wiz: key %s; \"%s\"; keys %s"), *Result, *Wizard->GetKeyMessage(), *List);
		}));

	FAutoConsoleCommand KeysCommand(
		TEXT("Wiz.Keys"),
		TEXT("Wiz.Keys: prints every control and its one key."),
		FConsoleCommandDelegate::CreateLambda([]()
		{
			FString List;
			for (const FWizardKeyBinding& Binding : AWizardPawn::GetKeyBindings())
			{
				List += FString::Printf(TEXT("%s%s=%s"), List.IsEmpty() ? TEXT("") : TEXT(", "), *Binding.Action.ToString(), *Binding.Key.ToString());
			}
			UE_LOG(LogWizard, Display, TEXT("Wiz: keys %s"), *List);
		}));

	FAutoConsoleCommandWithWorldAndArgs SummonCommand(
		TEXT("Wiz.Summon"),
		TEXT("Wiz.Summon: casts Summon the Afterlife, which spends held bones on plain skeletons (3 bones each; Wiz.Tune SummonMax <N> limits one cast to N, 0 = all)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UCrowdSubsystem* Crowd = GetCrowd(World))
			{
				const int32 Raised = Crowd->Summon();
				UE_LOG(LogWizard, Display, TEXT("Wiz: summoned %d skeletons (army %d, bones %d)"), Raised, Crowd->GetArmyCount(), Crowd->GetBonesHeld());
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs BuyCommand(
		TEXT("Wiz.Buy"),
		TEXT("Wiz.Buy <Item> [Count=1]: buys from the shrine with held bones first and then the army with the wizard, who are sacrificed, from anywhere (Item: Spear, Armor, Blades, Focus are research, bought once; Guard is a unit, Count 1-40 or Max)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UCrowdSubsystem* Crowd = GetCrowd(World);
			if (!Crowd)
			{
				return;
			}
			int32 Found = INDEX_NONE;
			for (int32 Index = 0; Index < AWizardHUD::ShrineMenuRows; ++Index)
			{
				if (Args.Num() > 0 && Args[0].Equals(FCrowdSim::GetShopName(static_cast<ECrowdShopItem>(Index)), ESearchCase::IgnoreCase))
				{
					Found = Index;
				}
			}
			if (Found == INDEX_NONE)
			{
				UE_LOG(LogWizard, Display, TEXT("Wiz: unknown item (use Spear, Armor, Blades, Focus or Guard)"));
				return;
			}
			const ECrowdShopItem Item = static_cast<ECrowdShopItem>(Found);
			const int32 Count = Args.Num() < 2 ? 1 : Args[1].Equals(TEXT("Max"), ESearchCase::IgnoreCase) ? MAX_int32 : FCString::Atoi(*Args[1]);
			const int32 Bought = Crowd->Buy(Item, Count);
			UE_LOG(LogWizard, Display, TEXT("Wiz: bought %d %s for %d bones (bones %d, army %d, research %d, spear %s)"),
				Bought, FCrowdSim::GetShopName(Item), Bought * Crowd->GetShopCost(Found), Crowd->GetBonesHeld(), Crowd->GetArmyCount(),
				Crowd->GetResearchCount(), Crowd->IsSpearUnlocked() ? TEXT("unlocked") : TEXT("locked"));
		}));

	FAutoConsoleCommandWithWorldAndArgs UnlockSpearCommand(
		TEXT("Wiz.UnlockSpear"),
		TEXT("Wiz.UnlockSpear [0|1=toggle]: unlocks or locks the Spear of Sacrifice without paying for it."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UCrowdSubsystem* Crowd = GetCrowd(World))
			{
				Crowd->SetSpearUnlocked(ReadSwitch(Args, Crowd->IsSpearUnlocked()));
				UE_LOG(LogWizard, Display, TEXT("Wiz: the spear is %s"), Crowd->IsSpearUnlocked() ? TEXT("unlocked") : TEXT("locked"));
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs MapCommand(
		TEXT("Wiz.Map"),
		TEXT("Wiz.Map [Name]: loads a level and starts the run over in it (Name: Outskirts, Practice). No argument prints the current one."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UCrowdSubsystem* Crowd = GetCrowd(World);
			if (!Crowd)
			{
				return;
			}
			if (Args.Num() > 0 && !Crowd->LoadMap(Args[0]))
			{
				UE_LOG(LogWizard, Display, TEXT("Wiz: unknown map (use %s)"), *FString::Join(FLevelLayout::GetNames(), TEXT(", ")));
				return;
			}
			UE_LOG(LogWizard, Display, TEXT("Wiz: map is %s (%d pieces, %d walls, %d posts, %d enemies at posts)"),
				*Crowd->GetMapName(), Crowd->GetMapPieceCount(), Crowd->GetObstacleCount(), Crowd->GetMapPostCount(), Crowd->GetAsleepCount());
		}));

	FAutoConsoleCommandWithWorldAndArgs MapScaleCommand(
		TEXT("Wiz.MapScale"),
		TEXT("Wiz.MapScale [UnitsPerPixel=15]: how large the level is beside the wizard, 8 to 22 (10 was the first block-out). Loads the level again. No argument prints the current scale."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UCrowdSubsystem* Crowd = GetCrowd(World))
			{
				if (Args.Num() > 0)
				{
					Crowd->SetMapScale(FCString::Atof(*Args[0]));
				}
				UE_LOG(LogWizard, Display, TEXT("Wiz: map scale is %.1f units a pixel (%s, %d enemies at posts, %d of %d roads patrolled)"),
					Crowd->GetMapScale(), *Crowd->GetMapName(), Crowd->GetAsleepCount(), Crowd->GetRoadPatrolCount(), Crowd->GetMapRouteCount());
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs ModelCommand(
		TEXT("Wiz.Model"),
		TEXT("Wiz.Model [0|1=toggle]: shows the Wizard King model (1) or the simple shapes (0)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UCrowdSubsystem* Crowd = GetCrowd(World);
			if (AWizardPawn* Wizard = Crowd ? Crowd->GetWizard() : nullptr)
			{
				Wizard->SetModelShown(ReadSwitch(Args, Wizard->IsModelShown()));
				UE_LOG(LogWizard, Display, TEXT("Wiz: model is %s (%s)"), OnOff(Wizard->IsModelShown()), *Wizard->GetModelName());
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs SeeThroughCommand(
		TEXT("Wiz.SeeThrough"),
		TEXT("Wiz.SeeThrough [Radius=0.13] [X Y]: buildings and walls are cut away in a circle around the cursor. Radius is a share of the screen's height, 0 to 0.5 (0: off). X Y (0 to 1 across and down the screen) put the circle there instead of at the mouse; -1 -1 follows the mouse again. No argument prints the state."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UCrowdSubsystem* Crowd = GetCrowd(World))
			{
				if (Args.Num() > 0)
				{
					Crowd->SetSeeThroughRadius(FCString::Atof(*Args[0]));
				}
				if (Args.Num() > 2)
				{
					Crowd->SetSeeThroughAt(FVector2D(FCString::Atod(*Args[1]), FCString::Atod(*Args[2])));
				}
				UE_LOG(LogWizard, Display, TEXT("Wiz: see-through radius is %.2f (%s)"), Crowd->GetSeeThroughRadius(),
					Crowd->IsSeeThroughReady() ? TEXT("material ready") : TEXT("material missing"));
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs HouseBoxesCommand(
		TEXT("Wiz.HouseBoxes"),
		TEXT("Wiz.HouseBoxes [0|1=toggle]: the plain boxes that stand in for houses are drawn and in the way (1) or gone (0). Changing it starts the run over."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UCrowdSubsystem* Crowd = GetCrowd(World))
			{
				Crowd->SetHouseBoxesShown(ReadSwitch(Args, Crowd->AreHouseBoxesShown()));
				UE_LOG(LogWizard, Display, TEXT("Wiz: house boxes are %s (%d placed buildings)"), OnOff(Crowd->AreHouseBoxesShown()), Crowd->GetPlacedBuildingCount());
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs GotoCommand(
		TEXT("Wiz.Goto"),
		TEXT("Wiz.Goto <x> <y> | shrine | gate: puts the wizard at a place on the ground, just inside the shrine's range, or in the kingdom gate."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UCrowdSubsystem* Crowd = GetCrowd(World);
			AWizardPawn* Wizard = Crowd ? Crowd->GetWizard() : nullptr;
			if (!Wizard || Args.Num() == 0)
			{
				return;
			}
			FVector2D Where;
			if (Args[0].Equals(TEXT("shrine"), ESearchCase::IgnoreCase))
			{
				Where = Crowd->GetShrinePosition() + FVector2D(-0.85 * Crowd->GetShrineRange(), 0.0);
			}
			else if (Args[0].Equals(TEXT("gate"), ESearchCase::IgnoreCase) && Crowd->HasGate())
			{
				Where = Crowd->GetGatePosition();
			}
			else if (Args.Num() >= 2)
			{
				Where = FVector2D(FCString::Atod(*Args[0]), FCString::Atod(*Args[1]));
			}
			else
			{
				return;
			}
			Wizard->SetActorLocation(FVector(Where.X, Where.Y, Wizard->GetActorLocation().Z));
			UE_LOG(LogWizard, Display, TEXT("Wiz: wizard is at %.0f, %.0f"), Where.X, Where.Y);
		}));

	FAutoConsoleCommandWithWorldAndArgs WeaponsCommand(
		TEXT("Wiz.Weapons"),
		TEXT("Wiz.Weapons: what each kind of figure holds and the socket it is held by."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (const UCrowdSubsystem* Crowd = GetCrowd(World))
			{
				UE_LOG(LogWizard, Display, TEXT("Wiz: weapons %s"), *Crowd->GetWeaponSummary());
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs EffectsCommand(
		TEXT("Wiz.Effects"),
		TEXT("Wiz.Effects: the material and shape the Bolt, the enemy's warnings and the spear's mark are drawn with."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (const UCrowdSubsystem* Crowd = GetCrowd(World))
			{
				UE_LOG(LogWizard, Display, TEXT("Wiz: effects %s"), *Crowd->GetEffectSummary());
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs SoundCommand(
		TEXT("Wiz.Sound"),
		TEXT("Wiz.Sound [Cue]: plays a sound effect (denied, blink, swordhitsenemy, swordhitsarmor, summoned, skeletondies, hammerhitsground, firethrow, fireimpact, daggermiss, bowshoot, bodyfalldirt1, bodyfalldirt2, bodyfallgrass1, bodyfallgrass2, bonepickup, gateopen, commanderspearthrow). No argument prints how many times each has started."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UWizardAudioSubsystem* Audio = World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UWizardAudioSubsystem>() : nullptr;
			if (!Audio)
			{
				return;
			}
			if (Args.Num() > 0)
			{
				const bool bPlayed = Audio->PlayCueByName(Args[0]);
				UE_LOG(LogWizard, Display, TEXT("Wiz: sound %s %s"), *Args[0], bPlayed ? TEXT("played") : TEXT("not played"));
				return;
			}
			FString Line;
			for (int32 Index = 0; Index < static_cast<int32>(EWizardCue::Count); ++Index)
			{
				const EWizardCue Cue = static_cast<EWizardCue>(Index);
				Line += FString::Printf(TEXT("%s\"%s\":%d"), Index > 0 ? TEXT(",") : TEXT(""), FWizardSoundRules::Info(Cue).Name, Audio->GetCuePlays(Cue));
			}
			UE_LOG(LogWizard, Display, TEXT("Wiz: sounds {%s,\"total\":%d,\"last\":\"%s\",\"music\":\"%s\",\"musicChanges\":%d,\"placed\":%d,\"followed\":%d,\"followedMoves\":%d,\"heard\":\"%.2f %.2f %.2f\"}"), *Line, Audio->GetTotalCuePlays(), *Audio->GetLastCueName(), *Audio->GetMusicName(), Audio->GetMusicChanges(),
				Audio->GetPlacedCuePlays(), Audio->GetFollowedCuePlays(), Audio->GetFollowedMoves(), Audio->GetChannelVolume(1), Audio->GetChannelVolume(2), Audio->GetChannelVolume(3));
		}));

	FAutoConsoleCommandWithWorldAndArgs MusicCommand(
		TEXT("Wiz.Music"),
		TEXT("Wiz.Music [menu|game|end|none]: changes the music (the game puts its own back at the next frame it draws). No argument prints what is playing."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UWizardAudioSubsystem* Audio = World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UWizardAudioSubsystem>() : nullptr;
			if (!Audio)
			{
				return;
			}
			if (Args.Num() > 0 && !Audio->SetMusicByName(Args[0]))
			{
				UE_LOG(LogWizard, Display, TEXT("Wiz: no music named %s"), *Args[0]);
				return;
			}
			UE_LOG(LogWizard, Display, TEXT("Wiz: music is %s (%d changes)"), *Audio->GetMusicName(), Audio->GetMusicChanges());
		}));

	FAutoConsoleCommandWithWorldAndArgs GroundCommand(
		TEXT("Wiz.Ground"),
		TEXT("Wiz.Ground <x> <y>: whether the ground at a place is painted dirt or grass (from the baked grid)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			const UCrowdSubsystem* Crowd = GetCrowd(World);
			if (!Crowd || Args.Num() < 2)
			{
				return;
			}
			const FVector2D Where(FCString::Atod(*Args[0]), FCString::Atod(*Args[1]));
			UE_LOG(LogWizard, Display, TEXT("Wiz: ground at %.0f, %.0f is %s (grid %s)"), Where.X, Where.Y, Crowd->IsDirtAt(Where) ? TEXT("dirt") : TEXT("grass"), Crowd->HasGroundPaint() ? TEXT("loaded") : TEXT("missing"));
		}));

	FAutoConsoleCommandWithWorldAndArgs FloorCommand(
		TEXT("Wiz.Floor"),
		TEXT("Wiz.Floor <x0> <y0> <x1> <y1> [Steps=20]: what the wizard would stand on along a line: height, slope (1 is flat) and what it is, for each step."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (!World || Args.Num() < 4)
			{
				return;
			}
			const FVector2D From(FCString::Atod(*Args[0]), FCString::Atod(*Args[1]));
			const FVector2D To(FCString::Atod(*Args[2]), FCString::Atod(*Args[3]));
			const int32 Steps = FMath::Clamp(Args.Num() > 4 ? FCString::Atoi(*Args[4]) : 20, 1, 200);
			FString Line;
			for (int32 Step = 0; Step <= Steps; ++Step)
			{
				const FVector2D At = FMath::Lerp(From, To, static_cast<double>(Step) / Steps);
				FHitResult Hit;
				const bool bHit = World->LineTraceSingleByChannel(Hit, FVector(At.X, At.Y, 3000.0), FVector(At.X, At.Y, -500.0), ECC_Pawn, FCollisionQueryParams(SCENE_QUERY_STAT(WizardFloor), false));
				Line += FString::Printf(TEXT(" [%.0f,%.0f z%.0f n%.2f %s]"), At.X, At.Y, bHit ? Hit.Location.Z : -999.0, bHit ? Hit.Normal.Z : 0.0,
					bHit && Hit.GetActor() ? *Hit.GetActor()->GetActorNameOrLabel().Left(14) : TEXT("-"));
			}
			UE_LOG(LogWizard, Display, TEXT("Wiz: floor%s"), *Line);
		}));

	FAutoConsoleCommandWithWorldAndArgs OverwhelmCommand(
		TEXT("Wiz.Overwhelm"),
		TEXT("Wiz.Overwhelm [Fill=0.08] [Drain=0.2] [Delay=2]: the meter's numbers. Fill: a share of the meter for each enemy on the wizard a second. Drain: a share a second once it goes down. Delay: seconds without build-up before it goes down (0 to 10). No arguments prints them."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UCrowdSubsystem* Crowd = GetCrowd(World))
			{
				FOverwhelmTuning& Tuning = Crowd->GetOverwhelmTuning();
				if (Args.Num() > 0)
				{
					Tuning.FillPerEnemyPerSecond = FMath::Clamp(FCString::Atof(*Args[0]), 0.0f, 5.0f);
				}
				if (Args.Num() > 1)
				{
					Tuning.DrainPerSecond = FMath::Clamp(FCString::Atof(*Args[1]), 0.0f, 5.0f);
				}
				if (Args.Num() > 2)
				{
					Tuning.DrainDelay = FMath::Clamp(FCString::Atof(*Args[2]), 0.0f, 10.0f);
				}
				UE_LOG(LogWizard, Display, TEXT("Wiz: overwhelm fill %.3f, drain %.3f, delay %.1f"), Tuning.FillPerEnemyPerSecond, Tuning.DrainPerSecond, Tuning.DrainDelay);
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs OpenGateCommand(
		TEXT("Wiz.OpenGate"),
		TEXT("Wiz.OpenGate: opens the kingdom gate with the commander alive, so walking into it finishes the game."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UCrowdSubsystem* Crowd = GetCrowd(World))
			{
				const bool bOpen = Crowd->OpenGate();
				UE_LOG(LogWizard, Display, TEXT("Wiz: the kingdom gate is %s"), bOpen ? TEXT("open") : TEXT("not in this level"));
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs ArmyModelCommand(
		TEXT("Wiz.ArmyModel"),
		TEXT("Wiz.ArmyModel [0|1=toggle]: draws skeletons, Guards and grunts as their models (1) or as simple shapes (0)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UCrowdSubsystem* Crowd = GetCrowd(World))
			{
				Crowd->SetArmyModelShown(ReadSwitch(Args, Crowd->IsArmyModelShown()));
				UE_LOG(LogWizard, Display, TEXT("Wiz: army model is %s"), OnOff(Crowd->IsArmyModelShown()));
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs ModelHeightCommand(
		TEXT("Wiz.ModelHeight"),
		TEXT("Wiz.ModelHeight [Units=220]: how tall the wizard's model stands, 120 to 400 (a grunt is 50, a house 420). No argument prints the current height."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UCrowdSubsystem* Crowd = GetCrowd(World);
			if (AWizardPawn* Wizard = Crowd ? Crowd->GetWizard() : nullptr)
			{
				if (Args.Num() > 0)
				{
					Wizard->SetModelHeight(FCString::Atof(*Args[0]));
				}
				UE_LOG(LogWizard, Display, TEXT("Wiz: model height is %.0f"), Wizard->GetModelHeight());
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs GarrisonCommand(
		TEXT("Wiz.Garrison"),
		TEXT("Wiz.Garrison <Grunts> [Brutes=0] [Distance=1500]: mans a post that far along the aim. The group stands until the wizard or a skeleton comes within 900 units (Wiz.Tune WakeRadius), or one of them is hit."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UCrowdSubsystem* Crowd = GetCrowd(World);
			const AWizardPawn* Wizard = Crowd ? Crowd->GetWizard() : nullptr;
			if (!Wizard)
			{
				return;
			}
			const int32 Grunts = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 0;
			const int32 Brutes = Args.Num() > 1 ? FCString::Atoi(*Args[1]) : 0;
			const double Distance = Args.Num() > 2 ? FCString::Atof(*Args[2]) : 1500.0;
			const int32 Placed = Crowd->SpawnGarrison(Wizard->GetPosition2D() + Wizard->GetAimDirection() * Distance, Grunts, Brutes);
			UE_LOG(LogWizard, Display, TEXT("Wiz: post manned with %d of %d (%d at posts now)"), Placed, Grunts + Brutes, Crowd->GetAsleepCount());
		}));

	FAutoConsoleCommandWithWorldAndArgs CommanderCommand(
		TEXT("Wiz.Commander"),
		TEXT("Wiz.Commander [Distance=1500] [EscortGrunts=0] [EscortBrutes=0]: puts a commander at a post that far along the aim. Killing him takes the area and ends the patrols."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UCrowdSubsystem* Crowd = GetCrowd(World);
			const AWizardPawn* Wizard = Crowd ? Crowd->GetWizard() : nullptr;
			if (!Wizard)
			{
				return;
			}
			const double Distance = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 1500.0;
			const bool bPlaced = Crowd->SpawnCommander(Wizard->GetPosition2D() + Wizard->GetAimDirection() * Distance,
				Args.Num() > 1 ? FCString::Atoi(*Args[1]) : 0, Args.Num() > 2 ? FCString::Atoi(*Args[2]) : 0);
			UE_LOG(LogWizard, Display, TEXT("Wiz: commander %s (%d at posts now)"), bPlaced ? TEXT("placed") : TEXT("not placed"), Crowd->GetAsleepCount());
		}));

	FAutoConsoleCommandWithWorldAndArgs WakeAllCommand(
		TEXT("Wiz.WakeAll"),
		TEXT("Wiz.WakeAll: every enemy standing at a post charges."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UCrowdSubsystem* Crowd = GetCrowd(World))
			{
				const int32 Woken = Crowd->GetAsleepCount();
				Crowd->WakeAll();
				UE_LOG(LogWizard, Display, TEXT("Wiz: woke %d enemies"), Woken);
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs PatrolsCommand(
		TEXT("Wiz.Patrols"),
		TEXT("Wiz.Patrols [0|1|now=toggle]: turns patrols on or off, or sends one now. A patrol is a few grunts from the next manned post (Wiz.Tune PatrolInterval, PatrolSize). Off also stops dead road patrols being replaced (Wiz.Tune RoadPatrolSize, RoadPatrolRespawn, PatrolWalkSpeed)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UCrowdSubsystem* Crowd = GetCrowd(World))
			{
				if (Args.Num() > 0 && Args[0].Equals(TEXT("now"), ESearchCase::IgnoreCase))
				{
					UE_LOG(LogWizard, Display, TEXT("Wiz: patrol of %d sent"), Crowd->SendPatrol());
					return;
				}
				Crowd->SetPatrolsEnabled(ReadSwitch(Args, Crowd->ArePatrolsEnabled()));
				UE_LOG(LogWizard, Display, TEXT("Wiz: patrols are %s"), OnOff(Crowd->ArePatrolsEnabled()));
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs ShrineCommand(
		TEXT("Wiz.Shrine"),
		TEXT("Wiz.Shrine [0|1=toggle]: opens or closes the shrine's menu. It opens only when the wizard stands in the shrine's circle."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UCrowdSubsystem* Crowd = GetCrowd(World))
			{
				if (ReadSwitch(Args, Crowd->IsShrineOpen()) != Crowd->IsShrineOpen())
				{
					Crowd->ToggleShrine();
				}
				UE_LOG(LogWizard, Display, TEXT("Wiz: the shrine is %s (the wizard is %s its circle)"),
					Crowd->IsShrineOpen() ? TEXT("open") : TEXT("closed"), Crowd->IsWizardAtShrine() ? TEXT("in") : TEXT("out of"));
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs PauseCommand(
		TEXT("Wiz.Pause"),
		TEXT("Wiz.Pause [0|1=toggle]: pauses or resumes the game and shows the pause menu."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UCrowdSubsystem* Crowd = GetCrowd(World);
			AWizardPawn* Wizard = Crowd ? Crowd->GetWizard() : nullptr;
			if (Wizard)
			{
				if (ReadSwitch(Args, World->IsPaused()) != World->IsPaused())
				{
					Wizard->TogglePause();
				}
				UE_LOG(LogWizard, Display, TEXT("Wiz: the game is %s"), World->IsPaused() ? TEXT("paused") : TEXT("running"));
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs TakeAreaCommand(
		TEXT("Wiz.TakeArea"),
		TEXT("Wiz.TakeArea: the area is taken as if the commander had just died: the message plays and the kingdom gate opens."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UCrowdSubsystem* Crowd = GetCrowd(World))
			{
				Crowd->TakeArea();
				UE_LOG(LogWizard, Display, TEXT("Wiz: the area is taken"));
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs StartMenuCommand(
		TEXT("Wiz.StartMenu"),
		TEXT("Wiz.StartMenu [0|1|controls=toggle]: shows the start menu with the game paused behind it (1), its controls screen (controls), or closes it and starts the run (0)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UCrowdSubsystem* Crowd = GetCrowd(World);
			AWizardPawn* Wizard = Crowd ? Crowd->GetWizard() : nullptr;
			if (Wizard)
			{
				if (Args.Num() > 0 && Args[0].Equals(TEXT("controls"), ESearchCase::IgnoreCase))
				{
					// The controls are a tab of the settings screen now, which the start menu's second button opens.
					Wizard->ShowStartMenu(true);
					Wizard->ShowSettings(true);
				}
				else
				{
					Wizard->ShowStartMenu(ReadSwitch(Args, Wizard->IsMenuOpen()));
				}
				UE_LOG(LogWizard, Display, TEXT("Wiz: the start menu is %s"), *Wizard->GetMenuName());
			}
		}));

	void LogSettings(UWorld* World)
	{
		UCrowdSubsystem* Crowd = GetCrowd(World);
		const AWizardPawn* Wizard = Crowd ? Crowd->GetWizard() : nullptr;
		const UGameInstance* Game = World ? World->GetGameInstance() : nullptr;
		const UWizardSettingsSubsystem* Store = Game ? Game->GetSubsystem<UWizardSettingsSubsystem>() : nullptr;
		if (!Store)
		{
			return;
		}
		UE_LOG(LogWizard, Display, TEXT("Wiz: settings {\"screen\":\"%s\",\"quality\":%d,\"renderScale\":%d,\"hudSize\":%d,\"edges\":%d,\"frameRate\":%d,\"graphicsApplied\":%s,\"paused\":%s}"),
			Wizard ? *Wizard->GetSettingsName() : TEXT("closed"), Store->GetQuality(), Store->GetRenderScale(), Store->GetHudSize(), Store->GetDangerEdges(),
			Store->IsFrameRateShown() ? 1 : 0, Store->AreGraphicsApplied() ? TEXT("true") : TEXT("false"), World->IsPaused() ? TEXT("true") : TEXT("false"));
		UE_LOG(LogWizard, Display, TEXT("Wiz: sound {\"master\":%d,\"music\":%d,\"effects\":%d,\"interface\":%d,\"muted\":\"%d%d%d%d\",\"loudness\":\"%.2f %.2f %.2f %.2f\",\"backgroundMute\":%d,\"channelsReady\":%s,\"testSounds\":%d}"),
			Store->GetVolume(0), Store->GetVolume(1), Store->GetVolume(2), Store->GetVolume(3),
			Store->IsMuted(0) ? 1 : 0, Store->IsMuted(1) ? 1 : 0, Store->IsMuted(2) ? 1 : 0, Store->IsMuted(3) ? 1 : 0,
			Store->GetLoudness(0), Store->GetLoudness(1), Store->GetLoudness(2), Store->GetLoudness(3),
			Store->IsBackgroundMuted() ? 1 : 0, Store->AreChannelsReady() ? TEXT("true") : TEXT("false"), Store->GetTestSoundsPlayed());
	}

	FAutoConsoleCommandWithWorldAndArgs SettingsCommand(
		TEXT("Wiz.Settings"),
		TEXT("Wiz.Settings [0|1|controls|sound|display=report]: opens the settings screen over the start or pause menu (pausing the game), on a tab if one is named, or closes it (0). With no argument it only reports the screen and every setting."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UCrowdSubsystem* Crowd = GetCrowd(World);
			AWizardPawn* Wizard = Crowd ? Crowd->GetWizard() : nullptr;
			if (Wizard && Args.Num() > 0)
			{
				const TCHAR* Tabs[] = { TEXT("controls"), TEXT("sound"), TEXT("display") };
				int32 Tab = INDEX_NONE;
				for (int32 Index = 0; Index < UE_ARRAY_COUNT(Tabs); ++Index)
				{
					if (Args[0].Equals(Tabs[Index], ESearchCase::IgnoreCase))
					{
						Tab = Index;
					}
				}
				if (Tab != INDEX_NONE)
				{
					Wizard->ShowSettings(true);
					Wizard->SetSettingsTab(static_cast<EWizardSettingsTab>(Tab));
				}
				else
				{
					Wizard->ShowSettings(FCString::Atoi(*Args[0]) != 0);
				}
			}
			LogSettings(World);
		}));

	FAutoConsoleCommandWithWorldAndArgs SettingCommand(
		TEXT("Wiz.Setting"),
		TEXT("Wiz.Setting <quality|renderscale|hudsize|edges|framerate|reset|master|music|effects|interface|mute|background|resetsound|testsound> [Value]: sets one setting, as the settings screen does (applied at once). quality 0 low, 1 medium, 2 high; renderscale 50-100 in steps of 10; hudsize 80-130 in steps of 5; edges 0-100 in steps of 5; framerate 0 or 1; reset puts the Display tab back to its defaults. Sound: master, music, effects, interface <0-100 in steps of 5>; mute <channel> [0|1=toggle]; background <0 keep playing|1 mute when not in front>; resetsound; testsound [effects|interface=effects]."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UGameInstance* Game = World ? World->GetGameInstance() : nullptr;
			UWizardSettingsSubsystem* Store = Game ? Game->GetSubsystem<UWizardSettingsSubsystem>() : nullptr;
			if (Store && Args.Num() > 0)
			{
				const int32 Channel = FWizardSettings::ChannelByName(Args[0]);
				if (Args[0].Equals(TEXT("reset"), ESearchCase::IgnoreCase))
				{
					Store->ResetDisplay();
				}
				else if (Args[0].Equals(TEXT("resetsound"), ESearchCase::IgnoreCase))
				{
					Store->ResetSound();
				}
				else if (Args[0].Equals(TEXT("testsound"), ESearchCase::IgnoreCase))
				{
					Store->PlayTestSound(Args.Num() > 1 ? FWizardSettings::ChannelByName(Args[1]) : static_cast<int32>(EWizardSoundChannel::Effects));
				}
				else if (Args[0].Equals(TEXT("mute"), ESearchCase::IgnoreCase) && Args.Num() > 1)
				{
					const int32 Muted = FWizardSettings::ChannelByName(Args[1]);
					Store->SetMuted(Muted, Args.Num() > 2 ? FCString::Atoi(*Args[2]) != 0 : !Store->IsMuted(Muted));
				}
				else if (Args[0].Equals(TEXT("background"), ESearchCase::IgnoreCase) && Args.Num() > 1)
				{
					Store->SetBackgroundMute(FCString::Atoi(*Args[1]) != 0);
				}
				else if (Channel != INDEX_NONE && Args.Num() > 1)
				{
					Store->SetVolume(Channel, FCString::Atoi(*Args[1]));
				}
				else if (Args.Num() > 1)
				{
					Store->Set(FWizardSettings::RowByName(Args[0]), FCString::Atoi(*Args[1]));
				}
			}
			LogSettings(World);
		}));

	FAutoConsoleCommandWithWorldAndArgs AddBonesCommand(
		TEXT("Wiz.AddBones"),
		TEXT("Wiz.AddBones <Count>: drops that many bones on the wizard, who collects them at once. Bones are spent by Wiz.Summon and Wiz.Buy."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UCrowdSubsystem* Crowd = GetCrowd(World))
			{
				const int32 Added = Crowd->AddBones(Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 0);
				UE_LOG(LogWizard, Display, TEXT("Wiz: dropped %d bones (%d held, %d on the ground)"), Added, Crowd->GetBonesHeld(), Crowd->GetBonesOnGround());
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs HealthBarsCommand(
		TEXT("Wiz.HealthBars"),
		TEXT("Wiz.HealthBars [0|1=toggle]: shows or hides the health bars over enemies that were just hit."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (AWizardHUD* HUD = GetWizardHUD(World))
			{
				HUD->SetHealthBarsVisible(ReadSwitch(Args, HUD->AreHealthBarsVisible()));
				UE_LOG(LogWizard, Display, TEXT("Wiz: health bars are %s"), OnOff(HUD->AreHealthBarsVisible()));
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs TuneCommand(
		TEXT("Wiz.Tune"),
		TEXT("Wiz.Tune [Name] [Value]: changes one crowd number until the run restarts, for example Wiz.Tune LeashRadius 600. ")
		TEXT("With a name only it prints that value; with nothing it lists every name and value ")
		TEXT("(RoamRadius, LeashRadius, ArmyEngageRange, BlockRange, EnemyFlankDegrees, GruntSpeed and more)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UCrowdSubsystem* Crowd = GetCrowd(World);
			if (!Crowd)
			{
				return;
			}
			FCrowdTuning& Tuning = Crowd->EditTuning();
			if (Args.Num() == 0)
			{
				FString List;
				for (const FCrowdTuningEntry& Entry : GetCrowdTuningEntries())
				{
					List += FString::Printf(TEXT("%s%s=%g"), List.IsEmpty() ? TEXT("") : TEXT(", "), Entry.Name, Tuning.*Entry.Member);
				}
				UE_LOG(LogWizard, Display, TEXT("Wiz: tuning %s"), *List);
				return;
			}
			float* Value = FindCrowdTuningValue(Tuning, Args[0]);
			if (!Value)
			{
				UE_LOG(LogWizard, Warning, TEXT("Wiz: no tuning value named '%s'. Wiz.Tune with nothing after it lists them."), *Args[0]);
				return;
			}
			if (Args.Num() > 1)
			{
				if (!FCString::IsNumeric(*Args[1]))
				{
					UE_LOG(LogWizard, Warning, TEXT("Wiz: '%s' is not a number"), *Args[1]);
					return;
				}
				*Value = FMath::Max(FCString::Atof(*Args[1]), 0.0f);
			}
			UE_LOG(LogWizard, Display, TEXT("Wiz: tuning %s is %g"), *Args[0], *Value);
		}));

	FAutoConsoleCommandWithWorldAndArgs PerfTestCommand(
		TEXT("Wiz.PerfTest"),
		TEXT("Wiz.PerfTest [Mode=0]: a test switch for finding hitches. 0 normal; 1 figures never change the pose they copy; 2 unused figures stay shown under the ground in place of being hidden."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UCrowdSubsystem* Crowd = World ? World->GetSubsystem<UCrowdSubsystem>() : nullptr)
			{
				Crowd->SetPerfTest(Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 0);
				UE_LOG(LogWizard, Display, TEXT("Wiz: perf test is %d"), Crowd->GetPerfTest());
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs PerfCommand(
		TEXT("Wiz.Perf"),
		TEXT("Wiz.Perf [Reset]: prints what drawing the unit models has cost since the last reset as one JSON line (frame time, time in the model code, figures created, pose switches, show/hide flips; milliseconds). Reset starts counting again."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UCrowdSubsystem* Crowd = World ? World->GetSubsystem<UCrowdSubsystem>() : nullptr;
			if (!Crowd)
			{
				return;
			}
			if (Args.Num() > 0 && Args[0].Equals(TEXT("Reset"), ESearchCase::IgnoreCase))
			{
				Crowd->ResetModelCost();
				UE_LOG(LogWizard, Display, TEXT("Wiz: perf reset"));
				return;
			}
			UE_LOG(LogWizard, Display, TEXT("Wiz: perf %s"), *Crowd->GetModelCostText());
		}));

	FAutoConsoleCommandWithWorldAndArgs StateCommand(
		TEXT("Wiz.State"),
		TEXT("Wiz.State: prints army, enemies, Overwhelm, camera mode, waves, god mode, bolts fired, the wizard position, the wall count, bones and groups as one JSON line."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			const UCrowdSubsystem* Crowd = GetCrowd(World);
			if (!Crowd)
			{
				return;
			}
			const AWizardPawn* Wizard = Crowd->GetWizard();
			const FString Camera = Wizard ? AWizardPawn::GetCameraModeName(Wizard->GetCameraMode()) : TEXT("none");
			const FVector2D Position = Wizard ? Wizard->GetPosition2D() : FVector2D::ZeroVector;
			const TCHAR* Yes = TEXT("true");
			const TCHAR* No = TEXT("false");
			UE_LOG(LogWizard, Display,
				TEXT("Wiz: {\"army\":%d,\"enemies\":%d,\"overwhelm\":%.2f,\"camera\":\"%s\",\"waves\":%s,\"god\":%s,\"falling\":%s,\"raised\":%d,\"killed\":%d,\"bolts\":%d,\"seconds\":%.0f,\"x\":%.0f,\"y\":%.0f,\"walls\":%d,\"bones\":%d,\"bonesOnGround\":%d,\"groups\":%d,\"spears\":%d,\"spearUnlocked\":%s,\"research\":%d,\"summon\":%d,\"atPosts\":%d,\"patrols\":%d,\"areaTaken\":%s,\"timeScale\":%.2f,\"map\":\"%s\",\"roadPatrols\":%d,\"mapScale\":%.1f,\"model\":\"%s\",\"modelMoving\":%s,\"modelHeight\":%.0f,\"blink\":\"%s\",\"blinkCooldown\":%.1f,\"blinks\":%d,\"heights\":\"%.0f/%.0f/%.0f/%.0f/%.0f\",\"sent\":%d,\"commandMode\":%s,\"commandCount\":%d,\"commandKind\":\"%s\",\"arrows\":%d,\"arrowHits\":%d,\"assassinsSent\":%d,\"hunting\":%d,\"armyValue\":%d,\"boneWorth\":%d,\"boltInterval\":%.2f,\"hidden\":%s,\"away\":%d,\"bonesInView\":%d,\"armyModel\":%s,\"armyModels\":%d,\"sacrificed\":%d,\"seeThrough\":%.2f,\"seeThroughReady\":%s,\"slams\":%d,\"slamHits\":%d,\"slamming\":%d,\"swingsReady\":%d,\"buildings\":%d,\"houseBoxes\":%s,\"searching\":%d,\"returning\":%d,\"placedPieces\":%d,\"placedPosts\":%d,\"gate\":\"%s\",\"commanderNear\":%s,\"slowed\":%s,\"marksmen\":%d,\"javelinHits\":%d,\"slashHits\":%d,\"holding\":%s,\"menu\":\"%s\",\"outside\":%s,\"leftField\":%s,\"music\":\"%s\",\"cues\":%d,\"lastCue\":\"%s\",\"corpses\":%d,\"deaths\":%d,\"lastDeath\":\"%s\",\"bodyFalls\":%d,\"fallShown\":%.2f,\"sweeps\":%d,\"sweepHits\":%d,\"shrine\":%s}"),
				Crowd->GetArmyCount(), Crowd->GetEnemyCount(), Crowd->GetOverwhelm(), *Camera,
				Crowd->AreWavesEnabled() ? Yes : No, Crowd->IsGodMode() ? Yes : No, Crowd->IsFalling() ? Yes : No,
				Crowd->GetTotalRaised(), Crowd->GetTotalKilled(), Crowd->GetBoltsFired(), Crowd->GetRunSeconds(), Position.X, Position.Y, Crowd->GetObstacleCount(),
				Crowd->GetBonesHeld(), Crowd->GetBonesOnGround(), Crowd->GetGroupCount(), Crowd->GetSpearsThrown(),
				Crowd->IsSpearUnlocked() ? Yes : No, Crowd->GetResearchCount(), Crowd->GetSummonCount(),
				Crowd->GetAsleepCount(), Crowd->GetPatrolsSent(), Crowd->IsAreaTaken() ? Yes : No, Crowd->GetTimeScale(), *Crowd->GetMapName(),
				Crowd->GetRoadPatrolCount(), Crowd->GetMapScale(),
				Wizard ? *Wizard->GetModelName() : TEXT("none"), Wizard && Wizard->IsModelMoving() ? Yes : No,
				Wizard ? Wizard->GetModelHeight() : 0.0f, *Crowd->GetBlinkStateName(), Crowd->GetBlinkCooldown(), Crowd->GetBlinksCast(),
				Crowd->GetUnitHeight(ECrowdKind::Skeleton), Crowd->GetUnitHeight(ECrowdKind::Grunt), Crowd->GetUnitHeight(ECrowdKind::Guard),
				Crowd->GetUnitHeight(ECrowdKind::Brute), Crowd->GetUnitHeight(ECrowdKind::Commander),
				Crowd->GetCommandedCount(), Crowd->IsCommandMode() ? Yes : No, Crowd->GetCommandCount(), *Crowd->GetCommandKindName(),
				Crowd->GetArrowCount(), Crowd->GetArrowHits(), Crowd->GetAssassinsSent(), Crowd->GetHuntingCount(), Crowd->GetArmyBoneValue(),
				Crowd->GetBoneWorth(), Crowd->GetTuning().BoltInterval, Crowd->IsWizardHidden() ? Yes : No, Crowd->GetAwayCount(),
				Crowd->GetBonesInView(), Crowd->IsArmyModelShown() ? Yes : No, Crowd->GetArmyModelsDrawn(), Crowd->GetSacrificed(), Crowd->GetSeeThroughRadius(),
				Crowd->IsSeeThroughReady() ? Yes : No, Crowd->GetSlamsLanded(), Crowd->GetSlamHits(), Crowd->GetSlammingCount(), Crowd->GetSwingsReady(),
				Crowd->GetPlacedBuildingCount(), Crowd->AreHouseBoxesShown() ? Yes : No,
				Crowd->GetSearchingCount(), Crowd->GetReturningCount(), Crowd->GetPlacedPieceCount(), Crowd->GetPlacedPostCount(), *Crowd->GetGateStateName(),
				Crowd->IsCommanderNear() ? Yes : No, Crowd->IsWizardSlowed() ? Yes : No, Crowd->GetMarksmanCount(), Crowd->GetJavelinHits(), Crowd->GetSlashHits(),
				Crowd->IsOverwhelmHolding() ? Yes : No, Wizard ? *Wizard->GetMenuName() : TEXT("none"),
				Crowd->IsOutsideBattlefield() ? Yes : No, Crowd->HasLeftBattlefield() ? Yes : No,
				*Crowd->GetMusicName(), Crowd->GetCuePlays(), *Crowd->GetLastCueName(), Crowd->GetCorpsesShown(), Crowd->GetDeathsShown(), *Crowd->GetLastDeathName(),
				Crowd->GetBodyFalls(), Wizard ? Wizard->GetFallShown() : 0.0f, Crowd->GetSweepsLanded(), Crowd->GetSweepHits(),
				Crowd->IsShrineOpen() ? Yes : No);
		}));
}
