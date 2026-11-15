#pragma once

#include "Game/WizardSounds.h"
#include "CoreMinimal.h"
#include "Sim/Battlefield.h"
#include "Sim/BlinkState.h"
#include "Sim/CommandState.h"
#include "Sim/CrowdSim.h"
#include "Sim/KingdomGate.h"
#include "Sim/LevelLayout.h"
#include "Sim/OverwhelmMeter.h"
#include "Sim/ShrineState.h"
#include "Subsystems/WorldSubsystem.h"
#include "CrowdSubsystem.generated.h"

class ACrowdRenderer;
class AWizardPawn;
class AKingdomGateActor;

/**
 * One manager for the whole crowd: owns the simulation and the Overwhelm meter, steps them once a frame,
 * spawns waves, and starts the fall and the restart. Nothing in the crowd is an actor or ticks by itself.
 */
UCLASS()
class UCrowdSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Grunts in a wave: 4 to 8 by Roll (0 to 4), plus one for every 30 seconds of the run. */
	static int32 WaveSize(float RunSeconds, int32 Roll);

	/** Brutes in a wave: none for the first 45 seconds, then one, and one more for every further 60 seconds, up to 4. */
	static int32 WaveBrutes(float RunSeconds);

	void RegisterWizard(AWizardPawn* InWizard);
	AWizardPawn* GetWizard() const { return Wizard.Get(); }

	void FireBolt(FVector From, FVector Direction);

	/** Spawns enemies in a ring 1800 to 2500 units from the wizard. Returns how many fit under the cap. */
	int32 SpawnEnemies(int32 Count, ECrowdKind Kind = ECrowdKind::Grunt);

	/** Adds skeletons beside the wizard. Returns how many fit under the cap. */
	int32 AddSkeletons(int32 Count, ECrowdKind Kind = ECrowdKind::Skeleton);

	/** Opens the shrine's menu when the wizard is in range, or closes it when it is open. True when it is open afterwards. */
	bool ToggleShrine();

	void CloseShrine() { Shrine.Close(); }

	UFUNCTION(BlueprintPure, Category = "Crowd")
	bool IsShrineOpen() const { return Shrine.IsOpen(); }

	/** True when the wizard stands close enough to the shrine to use it. */
	UFUNCTION(BlueprintPure, Category = "Crowd")
	bool IsWizardAtShrine() const;

	UFUNCTION(BlueprintPure, Category = "Crowd")
	FVector2D GetShrinePosition() const { return Shrine.Position; }

	UFUNCTION(BlueprintPure, Category = "Crowd")
	float GetShrineRange() const { return Shrine.Range; }

	/** Starts the run again from the beginning. */
	void RequestRestart();

	UFUNCTION(BlueprintPure, Category = "Crowd")
	int32 GetArmyCount() const { return Sim.CountTeam(ECrowdTeam::Army); }

	UFUNCTION(BlueprintPure, Category = "Crowd")
	int32 GetEnemyCount() const { return Sim.CountTeam(ECrowdTeam::Enemy); }

	/** Bones the wizard has collected and not yet spent. */
	UFUNCTION(BlueprintPure, Category = "Crowd")
	int32 GetBonesHeld() const { return Sim.GetBonesHeld(); }

	/** Bones one skeleton costs. */
	UFUNCTION(BlueprintPure, Category = "Crowd")
	int32 GetBonesPerSkeleton() const { return FMath::Max(FMath::RoundToInt32(Sim.Tuning.BonesPerSkeleton), 1); }

	/** Piles of bones lying on the ground. */
	UFUNCTION(BlueprintPure, Category = "Crowd")
	int32 GetBonesOnGround() const { return Sim.GetBones().Num(); }

	/** Groups the army stands in right now. */
	UFUNCTION(BlueprintPure, Category = "Crowd")
	int32 GetGroupCount() const { return Sim.GetGroupCount(Sim.CountTeam(ECrowdTeam::Army)); }

	/** Casts Summon the Afterlife. Returns how many skeletons it raised: none while the shrine's menu is open or during the fall. */
	int32 Summon();

	/** Skeletons a cast of Summon the Afterlife would raise right now. */
	UFUNCTION(BlueprintPure, Category = "Crowd")
	int32 GetSummonCount() const { return Sim.GetSummonCount(); }

	/** Buys up to Count of an item with held bones. Returns how many were bought. */
	int32 Buy(ECrowdShopItem Item, int32 Count);

	/** What one of an item costs, in bones. Item is the row in the shrine's menu: 0 Spear, 1 Guard, 2 Armor. */
	UFUNCTION(BlueprintPure, Category = "Crowd")
	int32 GetShopCost(int32 Item) const { return Sim.GetShopCost(static_cast<ECrowdShopItem>(Item)); }

	/** How many of an item can be bought right now. */
	UFUNCTION(BlueprintPure, Category = "Crowd")
	int32 GetShopMax(int32 Item) const { return Sim.GetShopMax(static_cast<ECrowdShopItem>(Item)); }

	UFUNCTION(BlueprintPure, Category = "Crowd")
	bool IsSpearUnlocked() const { return Sim.IsSpearUnlocked(); }

	void SetSpearUnlocked(bool bUnlocked) { Sim.SetSpearUnlocked(bUnlocked); }

	/** True when a research has been bought. Item is the row in the shrine's menu. */
	UFUNCTION(BlueprintPure, Category = "Crowd")
	bool IsResearched(int32 Item) const { return Sim.IsResearched(static_cast<ECrowdShopItem>(Item)); }

	/** How many researches have been bought. */
	UFUNCTION(BlueprintPure, Category = "Crowd")
	int32 GetResearchCount() const;

	/** How fast time runs right now: slower while the shrine's menu is open. */
	UFUNCTION(BlueprintPure, Category = "Crowd")
	float GetTimeScale() const { return Shrine.GetTimeScale(Sim.Tuning.ShrineTimeScale); }

	/** Mans a post: a group that stands there until the wizard or his army comes near, or one of them is hit. Returns how many fit under the cap. */
	int32 SpawnGarrison(FVector2D Post, int32 Grunts, int32 Brutes, int32 Archers = -1, float Radius = 0.0f);

	/**
	 * Where a group of Count stands on a post's ground: spread evenly over a circle of Radius around it, so no two
	 * start on top of one another. The first spot is the middle.
	 */
	static TArray<FVector2D> MakePostSpots(FVector2D Post, int32 Count, double Radius);

	/**
	 * Reads a level whose systems were placed as actors: pieces, posts, the commander, patrol roads, the shrine, the
	 * player start, and the buildings that block. What the level does not have is left as the layout had it.
	 */
	static void ReadPlacedLevel(const ULevel& Level, FLevelLayout& InOutLayout, TArray<FCrowdObstacle>& OutBuildings);

	/** Posts read from the level at the start of the run (the commander's counts as one); 0 when they come from code. */
	int32 GetPlacedPostCount() const { return PlacedPostCount; }

	/** Puts the area's commander at a post with an escort. Patrols start with him and end when he dies. */
	bool SpawnCommander(FVector2D Post, int32 EscortGrunts, int32 EscortBrutes, int32 EscortArchers = -1, float Radius = 0.0f);

	/** Every enemy at a post charges. */
	void WakeAll() { Sim.WakeAll(); }

	/** Enemies still standing at their posts. */
	UFUNCTION(BlueprintPure, Category = "Crowd")
	int32 GetAsleepCount() const { return Sim.CountAsleep(); }

	/** True once the area's commander is dead. */
	UFUNCTION(BlueprintPure, Category = "Crowd")
	bool IsAreaTaken() const { return bAreaTaken; }

	/** Seconds since the area was taken. */
	UFUNCTION(BlueprintPure, Category = "Crowd")
	float GetAreaTakenSeconds() const { return AreaTakenSeconds; }

	/** For testers: the area is taken as if the commander had just died, so the message shows and the gate opens. */
	void TakeArea();

	/** True when the level has a kingdom gate. */
	UFUNCTION(BlueprintPure, Category = "Crowd")
	bool HasGate() const { return Gate.IsPlaced(); }

	/** True once the commander is dead (or a tester opened it): the wizard can enter. */
	UFUNCTION(BlueprintPure, Category = "Crowd")
	bool IsGateOpen() const { return Gate.IsOpen(); }

	/** True once the wizard has entered the kingdom gate: the game is finished. */
	UFUNCTION(BlueprintPure, Category = "Crowd")
	bool IsRunWon() const { return Gate.IsEntered(); }

	/** Seconds since the game was finished. */
	UFUNCTION(BlueprintPure, Category = "Crowd")
	float GetWonSeconds() const { return Gate.GetEnteredSeconds(); }

	UFUNCTION(BlueprintPure, Category = "Crowd")
	FVector2D GetGatePosition() const { return Gate.GetCenter(); }

	/** none (no gate in the level), shut, open or entered. */
	UFUNCTION(BlueprintPure, Category = "Crowd")
	FString GetGateStateName() const;

	/** Opens the gate with the commander alive: for testers. False when the level has no gate. */
	bool OpenGate();

	/** True while an awake commander is near the wizard: every enemy on him fills Overwhelm faster. The HUD shows an icon. */
	UFUNCTION(BlueprintPure, Category = "Crowd")
	bool IsCommanderNear() const { return bCommanderNear; }

	/** How many times as fast the meter fills while the commander is near. */
	UFUNCTION(BlueprintPure, Category = "Crowd")
	float GetCommanderAuraFill() const { return Sim.Tuning.CommanderAuraFill; }

	/** True while the commander's spear has him slowed, and the seconds of it left. */
	UFUNCTION(BlueprintPure, Category = "Crowd")
	bool IsWizardSlowed() const { return SlowTimer > 0.0f; }

	UFUNCTION(BlueprintPure, Category = "Crowd")
	float GetSlowSeconds() const { return SlowTimer; }

	/** The commander's spears and slashes that have hit the wizard in this run. */
	UFUNCTION(BlueprintPure, Category = "Crowd")
	int32 GetJavelinHits() const { return JavelinHits; }

	UFUNCTION(BlueprintPure, Category = "Crowd")
	int32 GetSlashHits() const { return SlashHits; }

	/** The commander's spears in the air. */
	UFUNCTION(BlueprintPure, Category = "Crowd")
	int32 GetJavelinCount() const { return Sim.GetJavelins().Num(); }

	/** Marksmen in the army. */
	UFUNCTION(BlueprintPure, Category = "Crowd")
	int32 GetMarksmanCount() const;

	/** True while the meter has something in it and is waiting before it goes down. */
	UFUNCTION(BlueprintPure, Category = "Crowd")
	bool IsOverwhelmHolding() const { return Meter.IsHolding(); }

	/** The meter's own numbers: fill for each enemy a second, drain a second, the wait before it drains. For testers. */
	FOverwhelmTuning& GetOverwhelmTuning() { return Meter.Tuning; }

	void SetPatrolsEnabled(bool bEnabled) { bPatrolsEnabled = bEnabled; }

	UFUNCTION(BlueprintPure, Category = "Crowd")
	bool ArePatrolsEnabled() const { return bPatrolsEnabled; }

	UFUNCTION(BlueprintPure, Category = "Crowd")
	int32 GetPatrolsSent() const { return PatrolsSent; }

	/** Sends one patrol now from the nearest manned post. Returns how many enemies it has. */
	int32 SendPatrol();

	/** Throws the bone spear from the wizard at a point on the ground. False when it cannot be thrown right now. */
	bool ThrowSpear(FVector2D Target);

	UFUNCTION(BlueprintPure, Category = "Crowd")
	bool CanThrowSpear() const { return Sim.CanThrowSpear(); }

	/** Seconds until the spear can be thrown again. */
	UFUNCTION(BlueprintPure, Category = "Crowd")
	float GetSpearCooldown() const { return Sim.GetSpearCooldown(); }

	/** Skeletons one spear costs. */
	UFUNCTION(BlueprintPure, Category = "Crowd")
	int32 GetSpearSkeletonCost() const { return FMath::RoundToInt32(Sim.Tuning.SpearSkeletonCost); }

	UFUNCTION(BlueprintPure, Category = "Crowd")
	int32 GetSpearsThrown() const { return SpearsThrown; }

	/**
	 * Blink: the wizard dashes BlinkRange along Direction over BlinkSeconds, stopped by walls and water, and nothing
	 * can hurt him on the way. A direction he holds during it bends it. False during a blink, its cooldown, the fall
	 * or the shrine's menu, and with no direction.
	 */
	bool StartBlink(FVector2D Direction);

	/** True during the dash. */
	UFUNCTION(BlueprintPure, Category = "Crowd")
	bool IsBlinking() const { return Blink.IsActive(); }

	/** Seconds until Blink can be cast again. 0 during the dash and when it is ready. */
	UFUNCTION(BlueprintPure, Category = "Crowd")
	float GetBlinkCooldown() const { return Blink.GetCooldown(); }

	/** "active" during the dash, "cooldown" after it, else "ready". */
	UFUNCTION(BlueprintPure, Category = "Crowd")
	FString GetBlinkStateName() const;

	UFUNCTION(BlueprintPure, Category = "Crowd")
	int32 GetBlinksCast() const { return BlinksCast; }

	/**
	 * Command mode: the player chooses how many of the army to send and which kind, then sends them to a place.
	 * Enters when it is off and there is army to send; leaves when it is on. True when it is on afterwards.
	 */
	bool ToggleCommandMode();

	void SetCommandMode(bool bOn);

	UFUNCTION(BlueprintPure, Category = "Crowd")
	bool IsCommandMode() const { return Command.IsActive(); }

	/** How many a send would send right now. */
	UFUNCTION(BlueprintPure, Category = "Crowd")
	int32 GetCommandCount() const { return Command.GetCount(); }

	/** "Any", "Skeleton" or "Guard": the kind a send would send. */
	UFUNCTION(BlueprintPure, Category = "Crowd")
	FString GetCommandKindName() const { return FCrowdSim::GetCommandKindName(Command.GetKind()); }

	/** How many of the chosen kind are with the wizard and can be sent. */
	UFUNCTION(BlueprintPure, Category = "Crowd")
	int32 GetCommandAvailable() const { return Sim.CountCommandable(Command.GetKind()); }

	/** The mouse wheel in command mode: more or fewer, between 1 and what can be sent. */
	void ScrollCommand(int32 Notches);

	/** Right mouse in command mode: the next kind. */
	void CycleCommandKind();

	/** Left mouse in command mode: sends the chosen number to the place and leaves command mode. Returns how many went. */
	int32 ConfirmCommand(FVector2D Point);

	/** Sends up to Count of a kind to a place, with or without command mode. Returns how many went. */
	int32 SendCommand(FVector2D Point, int32 Count, ECrowdCommandKind Kind);

	/** Calls every sent unit back to the wizard. Returns how many were out. */
	int32 Recall();

	/** Army units holding a place they were sent to. */
	UFUNCTION(BlueprintPure, Category = "Crowd")
	int32 GetCommandedCount() const { return Sim.CountCommanded(); }

	/** How Count of a shrine item would be paid for right now: bones first, then skeletons, then Guards. */
	FCrowdShopPrice GetShopPrice(int32 Item, int32 Count) const { return Sim.GetShopPrice(static_cast<ECrowdShopItem>(Item), Count); }

	/** Army units given up to pay at the shrine in this run. */
	int32 GetSacrificed() const { return Sim.GetSacrificed(); }

	/**
	 * Buildings and walls are cut away in a circle around the cursor. The radius is a share of the screen's height
	 * (0 to 0.5; 0 switches it off).
	 */
	void SetSeeThroughRadius(float Share) { SeeThroughRadius = FMath::Clamp(Share, 0.0f, 0.5f); }
	float GetSeeThroughRadius() const { return SeeThroughRadius; }

	/** For tests: puts the circle at a place on screen (0 to 1 across and down) instead of at the mouse. A negative value follows the mouse again. */
	void SetSeeThroughAt(FVector2D ScreenShare) { SeeThroughForced = ScreenShare; }

	/** Where the circle is on screen right now, 0 to 1 across and down; off screen when there is no cursor. */
	FVector2D GetSeeThroughAt() const { return SeeThroughShown; }

	/** True when the see-through material is in the game. */
	bool IsSeeThroughReady() const;

	/** True while the wizard cannot be seen: from the start of a blink until it slows. */
	bool IsWizardHidden() const { return Blink.IsHidden(Sim.Tuning.BlinkHiddenShare); }

	/** Army units outside the wizard's presence: they hit less hard and are hurt more. */
	int32 GetAwayCount() const;

	/** Bone piles the camera shows. They do not fade. */
	int32 GetBonesInView() const;

	/** Plain skeletons drawn as the skeleton model (true) or as simple shapes. */
	void SetArmyModelShown(bool bShown);
	bool IsArmyModelShown() const;
	int32 GetArmyModelsDrawn() const;
	/** Hidden figures that have an attack animation to play: six for each attack of each kind that strikes. */
	int32 GetSwingsReady() const;

	/** True where the ground is painted dirt (from the baked grid): a body falling there sounds of dirt, elsewhere of grass. */
	bool IsDirtAt(FVector2D Position) const { return GroundPaint.IsDirt(Position); }
	bool HasGroundPaint() const { return GroundPaint.IsValid(); }
	/** Brute sweeps that have landed in this run, and how many of them caught the wizard. */
	int32 GetSweepsLanded() const { return SweepsLanded; }
	int32 GetSweepHits() const { return SweepHits; }
	/** What each kind of figure holds and by which socket: "Skeleton:SM_Steel_Sword@Sword_Socket ...". */
	FString GetWeaponSummary() const;
	/** The material and shape each effect is drawn with: "Bolt:M_FireBolt/Sphere Slam:M_GroundMark/Plane ...". */
	FString GetEffectSummary() const;
	/** Bodies that have met the ground, with their sound, in this run. */
	int32 GetBodyFalls() const { return BodyFalls; }
	/** Figures falling or lying dead now, how many deaths have been shown, and where the last one's blow came from ("front", "behind", "side", "none"). */
	int32 GetCorpsesShown() const;
	int32 GetDeathsShown() const;
	FString GetLastDeathName() const;
	/** The music that is playing ("menu", "game", "end", "none"), sound effects started since the game opened, and the last one's name. */
	FString GetMusicName() const;
	int32 GetCuePlays() const;
	FString GetLastCueName() const;
	/** Brute slams that have landed in this run, how many of them caught the wizard, and brutes in a slam now. */
	int32 GetSlamsLanded() const { return SlamsLanded; }
	int32 GetSlamHits() const { return SlamHits; }
	int32 GetSlammingCount() const { return Sim.CountSlamming(); }
	/** Enemies that have lost the wizard and are going to where he was last seen or looking about there; and those running back. */
	int32 GetSearchingCount() const { return Sim.CountAlert(ECrowdAlert::Seek) + Sim.CountAlert(ECrowdAlert::Look); }
	int32 GetReturningCount() const { return Sim.CountAlert(ECrowdAlert::Return); }
	/** What drawing the models has cost since the last reset, as one JSON object (times in milliseconds). */
	FString GetModelCostText() const;
	/** One line for the debug overlay: the last three seconds. */
	FString GetModelCostLine() const;
	/** The hitch-finding test switch: see ACrowdRenderer::SetPerfTest. */
	void SetPerfTest(int32 Mode);
	int32 GetPerfTest() const;
	void ResetModelCost();

	/** Sent units that have left their place to go after an enemy that shot at their group. */
	int32 GetHuntingCount() const { return Sim.CountHunting(); }

	/** What the army counts for in bones (a share of what it cost), and that plus the bones held. */
	int32 GetArmyBoneValue() const { return Sim.GetArmyBoneValue(); }
	int32 GetBoneWorth() const { return Sim.GetBoneWorth(); }

	/** How wide the group that command mode would send spreads around its place. */
	float GetCommandRadius() const { return Sim.GetCommandRadius(Command.GetCount()); }

	/** Spawns enemies within Scatter of a point. Returns how many were spawned. */
	int32 SpawnEnemiesAt(int32 Count, ECrowdKind Kind, FVector2D Point, float Scatter = 150.0f);

	/** Sends one assassin now, from the nearest manned post. False when no post is manned or the enemy cap is reached. */
	bool SendAssassin();

	UFUNCTION(BlueprintPure, Category = "Crowd")
	int32 GetAssassinsSent() const { return AssassinsSent; }

	/** Arrows in the air. */
	UFUNCTION(BlueprintPure, Category = "Crowd")
	int32 GetArrowCount() const { return Sim.GetArrows().Num(); }

	/** Arrows that have hit the wizard in this run. */
	UFUNCTION(BlueprintPure, Category = "Crowd")
	int32 GetArrowHits() const { return ArrowHits; }

	/** How tall a kind of unit stands: its size share (Wiz.Tune) of the wizard's height. */
	float GetUnitHeight(ECrowdKind Kind) const;

	/** Drops bones on the wizard, which he collects at once. Returns how many were dropped. */
	int32 AddBones(int32 Count);

	UFUNCTION(BlueprintPure, Category = "Crowd")
	float GetOverwhelm() const { return Meter.GetValue(); }

	UFUNCTION(BlueprintPure, Category = "Crowd")
	FVector2D GetPressureDirection() const { return Meter.GetPressureDirection(); }

	/** Offsets from the wizard to every enemy on him this frame (one that counts for several appears that often). */
	const TArray<FVector2D>& GetTouchingOffsets() const { return TouchingOffsets; }

	UFUNCTION(BlueprintPure, Category = "Crowd")
	bool IsFalling() const { return Meter.IsFalling(); }

	UFUNCTION(BlueprintPure, Category = "Crowd")
	float GetFallAlpha() const { return Meter.GetFallAlpha(); }

	UFUNCTION(BlueprintPure, Category = "Crowd")
	int32 GetTotalRaised() const { return TotalRaised; }

	UFUNCTION(BlueprintPure, Category = "Crowd")
	int32 GetTotalKilled() const { return TotalKilled; }

	UFUNCTION(BlueprintPure, Category = "Crowd")
	int32 GetBoltsFired() const { return BoltsFired; }

	UFUNCTION(BlueprintPure, Category = "Crowd")
	float GetRunSeconds() const { return RunSeconds; }

	UFUNCTION(BlueprintPure, Category = "Crowd")
	int32 GetUnitsDrawn() const;

	void SetWavesEnabled(bool bEnabled) { bWavesEnabled = bEnabled; }

	UFUNCTION(BlueprintPure, Category = "Crowd")
	bool AreWavesEnabled() const { return bWavesEnabled; }

	void SetGodMode(bool bEnabled) { Meter.bGodMode = bEnabled; }

	UFUNCTION(BlueprintPure, Category = "Crowd")
	bool IsGodMode() const { return Meter.bGodMode; }

	/**
	 * Loads a level by name (Outskirts, Practice): its pieces, walls, shrine, posts and commander. The run starts over
	 * at the level's start. False for an unknown name. The choice is kept when the run restarts.
	 */
	bool LoadMap(const FString& Name);

	UFUNCTION(BlueprintPure, Category = "Crowd")
	FString GetMapName() const { return Layout.Name; }

	UFUNCTION(BlueprintPure, Category = "Crowd")
	int32 GetMapPieceCount() const { return Layout.Pieces.Num(); }

	UFUNCTION(BlueprintPure, Category = "Crowd")
	int32 GetMapPostCount() const { return Layout.Posts.Num(); }

	/** Roads of this level that patrols walk. */
	UFUNCTION(BlueprintPure, Category = "Crowd")
	int32 GetMapRouteCount() const { return Layout.Routes.Num(); }

	/** Roads that have a living patrol on them right now. */
	UFUNCTION(BlueprintPure, Category = "Crowd")
	int32 GetRoadPatrolCount() const;

	/** Puts a patrol on a road, at the end farther from the wizard. 0 when that end is too close to him or the road does not exist. */
	int32 SpawnRoadPatrol(int32 RouteIndex);

	/** Units for one pixel of the level's drawing (Outskirts): how large the level is beside the wizard. */
	UFUNCTION(BlueprintPure, Category = "Crowd")
	float GetMapScale() const;

	/** Sets the scale (8 to 22) and loads the level again. Kept when the run restarts. */
	void SetMapScale(float UnitsPerPixel);

	static constexpr float MapScaleMin = 8.0f;
	static constexpr float MapScaleMax = 22.0f;

	/** The practice layout: walls, pillars, a corner and a gate around the start, to feel what moving around cover is like. */
	static TArray<FCrowdObstacle> MakePracticeObstacles();

	/** Puts the practice walls in or takes them out, for the simulation, the picture and the wizard's collision. */
	void SetObstaclesEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Crowd")
	bool AreObstaclesEnabled() const { return bObstaclesEnabled; }

	UFUNCTION(BlueprintPure, Category = "Crowd")
	int32 GetObstacleCount() const { return Sim.GetObstacles().Num(); }

	/**
	 * Buildings placed in the level by hand: static mesh actors whose mesh is in /Game/Buildings, or that carry the
	 * actor tag Blocker. Each blocks units, Bolts and the wizard.
	 */
	int32 GetPlacedBuildingCount() const { return PlacedBuildings.Num(); }
	/** Level Piece actors read from the level at the start of the run; 0 when the pieces come from code. */
	int32 GetPlacedPieceCount() const { return PlacedPieceCount; }
	/** True when the level's pieces are actors placed in it (the layout actor's Use Placed Pieces). */
	bool ArePiecesPlaced() const { return bPiecesPlaced; }

	/** The box a placed building blocks: its bounds on the ground, drawn in by BuildingBodyShare so eaves do not block. */
	static FCrowdObstacle MakeBuildingObstacle(const FBox& Bounds);

	/**
	 * The boxes a placed building blocks, from its mesh's own box and where it stands. One that stands square to the
	 * level is one box. One that is turned would need a box far larger than itself (boxes lie along X and Y), so it
	 * is made of a grid of small boxes that follow its walls: nothing is blocked more than about
	 * BuildingCellSize * 0.2 outside them.
	 */
	static void MakeBuildingObstacles(const FBox& MeshBox, const FTransform& Placed, TArray<FCrowdObstacle>& OutObstacles);

	/**
	 * Boxes that cover the shape a set of points makes on the ground (the outline around them): one box when the
	 * shape is square to the level, else a grid of small ones that follows it. A shape wider than SkipWiderThan
	 * (when above 0) is left out: it is something to walk on, like a bridge's deck, not something in the way.
	 * Returns how wide the shape is across its narrow way.
	 */
	static double MakeFootprintObstacles(const TArray<FVector2D>& Points, double SkipWiderThan, TArray<FCrowdObstacle>& OutObstacles);

	/**
	 * The boxes a placed mesh blocks, from the collision shapes made for it in the mesh editor (convex pieces and
	 * boxes), each as it lies on the ground where the mesh stands. With bWalkOnWide (a bridge), its wide pieces are
	 * the floor and only its narrow ones (rails, posts) block. A mesh with no collision shapes blocks its bounds.
	 */
	static void MakeCollisionObstacles(const class UStaticMesh& Mesh, const FTransform& Placed, bool bWalkOnWide, TArray<FCrowdObstacle>& OutObstacles);

	/** True for a mesh the wizard's Bolt passes through though it blocks walking: a house (/Game/Buildings) or a lamp post. */
	static bool BoltPassesMesh(const FString& MeshPath);

	/** A piece of a bridge wider than this across its narrow way is its deck. */
	static constexpr double WalkOnWidth = 150.0;

	/** True while the wizard is outside the battlefield and is being warned to come back. */
	UFUNCTION(BlueprintPure, Category = "Wizard")
	bool IsOutsideBattlefield() const { return Battlefield.IsWarning() && !Meter.IsFalling(); }

	/** Seconds he has left to come back before the mission fails. */
	UFUNCTION(BlueprintPure, Category = "Wizard")
	float GetBattlefieldSecondsLeft() const { return Battlefield.GetSecondsLeft(Sim.Tuning.BattlefieldWarnSeconds); }

	/** True when this run ended because he left the battlefield. */
	UFUNCTION(BlueprintPure, Category = "Wizard")
	bool HasLeftBattlefield() const { return bLeftBattlefield; }

	UFUNCTION(BlueprintPure, Category = "Wizard")
	bool HasBattlefield() const { return Battlefield.bActive; }

	/** A turned building is cut into boxes about this size. */
	static constexpr double BuildingCellSize = 220.0;
	static constexpr double BuildingBodyShare = 0.85;

	/** The plain boxes that stand in for houses: drawn and in the way (true), or gone. Changing it starts the run over. */
	void SetHouseBoxesShown(bool bShown);
	bool AreHouseBoxesShown() const;

	const FCrowdTuning& GetTuning() const { return Sim.Tuning; }

	/** For the Wiz.Tune command. */
	FCrowdTuning& EditTuning() { return Sim.Tuning; }

	const FCrowdSim& GetSim() const { return Sim; }

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	static constexpr float WaveInterval = 4.0f;
	static constexpr float FirstWaveDelay = 2.0f;
	static constexpr float FirstBruteSeconds = 45.0f;
	static constexpr float SpawnRingInner = 1800.0f;
	static constexpr float SpawnRingOuter = 2500.0f;
	// A patrol appears no farther from the wizard than this, so it can find its way to him.
	static constexpr float PatrolSpawnDistance = 2600.0f;
	static constexpr float PostScatter = 170.0f;
	// A road patrol does not appear closer to the wizard than this.
	static constexpr float RoadPatrolSpawnClear = 1800.0f;
	static constexpr float RoadPatrolScatter = 60.0f;
	// How far the wizard's middle stays from a wall when a blink ends: just inside his capsule's radius (40), so a
	// wizard who stands against a wall is not counted as inside it.
	static constexpr float BlinkClearance = 38.0f;
	// A screen corner that looks over the horizon counts as this far away.
	static constexpr double ViewFarthest = 8000.0;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	void RestartRun();
	void PlaySounds(FVector2D WizardPosition);
	void PlayCue(EWizardCue Cue, FVector2D Position, float SkipSeconds = 0.0f) const;
	void LoadGroundPaint();
	class UWizardAudioSubsystem* GetAudio() const;

	FGroundPaint GroundPaint;
	// Deaths whose bodies have not met the ground yet: when on the simulation's clock, and where.
	struct FPendingFall
	{
		double Time = 0.0;
		FVector2D Position = FVector2D::ZeroVector;
		EWizardCue Cue = EWizardCue::BodyFallGrass1;
		float Skip = 0.0f;
	};
	int32 BodyFallPicks = 0;
	int32 SweepsLanded = 0;
	int32 SweepHits = 0;
	TArray<FPendingFall> PendingFalls;
	int32 BodyFalls = 0;

	void ApplyObstacles();
	void PlaceWizardAtStart();

	FLevelLayout Layout;
	TArray<FCrowdObstacle> PlacedBuildings;
	int32 PlacedPieceCount = 0;
	int32 PlacedPostCount = 0;
	bool bPiecesPlaced = false;

	FCrowdSim Sim;
	FOverwhelmMeter Meter;
	FShrineState Shrine;
	FKingdomGate Gate;
	TWeakObjectPtr<AKingdomGateActor> GateActor;
	FBlinkState Blink;
	FCommandState Command;
	TArray<FVector2D> TouchingOffsets;

	TWeakObjectPtr<AWizardPawn> Wizard;

	UPROPERTY(Transient)
	TObjectPtr<ACrowdRenderer> Renderer;

	float SeeThroughRadius = 0.13f;
	FVector2D SeeThroughForced = FVector2D(-1.0, -1.0);
	FVector2D SeeThroughShown = FVector2D(-10.0, -10.0);

	/** Tells the simulation which ground the camera shows. */
	void UpdateView(const AWizardPawn& WizardPawn);

	float RunSeconds = 0.0f;
	float WaveTimer = FirstWaveDelay;
	int32 TotalRaised = 0;
	int32 TotalKilled = 0;
	int32 BoltsFired = 0;
	int32 SpearsThrown = 0;
	// The sound of each commander's spear in the air, by the spear's number: it is moved with its spear.
	TMap<int32, TWeakObjectPtr<class UAudioComponent>> SpearSounds;
	int32 BlinksCast = 0;
	int32 ArrowHits = 0;
	int32 JavelinHits = 0;
	int32 SlashHits = 0;
	float SlowTimer = 0.0f;
	bool bCommanderNear = false;
	int32 SlamsLanded = 0;
	int32 SlamHits = 0;
	int32 AssassinsSent = 0;
	float AssassinTimer = 0.0f;
	bool bWavesEnabled = true;
	bool bObstaclesEnabled = false;
	bool bRestarting = false;
	int32 NextSquad = 1;
	bool bPatrolsEnabled = true;
	bool bCommanderPlaced = false;
	bool bAreaTaken = false;
	FBattlefield Battlefield;
	bool bLeftBattlefield = false;
	float AreaTakenSeconds = 0.0f;
	float PatrolTimer = 0.0f;
	int32 PatrolsSent = 0;
	float AppliedTimeScale = 1.0f;

	// One for each road of the level: the group walking it, and the seconds until a dead group is replaced.
	struct FRoadPatrol
	{
		int32 Squad = INDEX_NONE;
		float Timer = 0.0f;
	};
	TArray<FRoadPatrol> RoadPatrols;
};
