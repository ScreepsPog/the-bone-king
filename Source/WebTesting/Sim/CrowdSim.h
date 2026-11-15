#pragma once

#include "Math/RandomStream.h"
#include "Sim/CrowdFlowField.h"
#include "Sim/CrowdTypes.h"

/**
 * The whole crowd as plain data: enemies, skeletons, bolts and walls on a 2D plane.
 * No engine objects, so it runs in unit tests and costs one loop per frame.
 */
/** A place part of the army was sent to hold, with the way there around walls and water. */
struct FCrowdOrder
{
	FVector2D Point = FVector2D::ZeroVector;
	FCrowdFlowField Field;
	// The enemy part of the group has gone after (a unit's Id), or 0. HuntIndex is where it is in the list this step.
	int32 HuntId = 0;
	int32 HuntIndex = INDEX_NONE;
	FCrowdFlowField HuntField;
};

/** What one enemy group knows of the wizard: whether any of them sees him, where he was last seen, the ways around walls. */
struct FCrowdSight
{
	// The group's number; for an enemy in no group, a number of its own (below INDEX_NONE).
	int32 Key = INDEX_NONE;
	bool bSees = false;
	bool bSaw = false;
	bool bAlive = false;
	FVector2D LastSeen = FVector2D::ZeroVector;
	FCrowdFlowField SeekField;
	// The way back: to where the first of the group to turn back stood. Each walks straight to its own spot once it can.
	bool bReturnBuilt = false;
	FCrowdFlowField ReturnField;
};

class FCrowdSim
{
public:
	FCrowdTuning Tuning;

	/** Every random choice (roam spots, flank side, speed share) comes from here, so a test run repeats exactly. */
	FRandomStream Random = FRandomStream(12345);

	/** How tall the wizard stands. Every unit's size, and the room it takes, is a share of this. */
	float WizardTall = 220.0f;

	/** How far a blow reaches between two kinds: the melee range, or their two bodies when those are wider. */
	double GetReach(ECrowdKind Attacker, ECrowdKind Target) const;

	/** How much room a kind of unit takes: the radius of its body. */
	float GetBodyRadius(ECrowdKind Kind) const;

	/**
	 * How much of a blow's damage a unit takes when the blow comes from From: 1 for all but a brute, which takes less
	 * from the front and more from the back.
	 */
	float GetDamageTaken(const FCrowdUnit& Target, FVector2D From) const;

	/** True while a brute's circle (or the commander's, for his slash) grows and until it lands. */
	bool IsSlamming(const FCrowdUnit& Unit) const
	{
		return (Unit.Kind == ECrowdKind::Brute || Unit.Kind == ECrowdKind::Commander) && Unit.SlamTime >= 0.0f;
	}

	/** True while a brute winds up its sweep (not its slam). */
	bool IsSweeping(const FCrowdUnit& Unit) const { return Unit.Kind == ECrowdKind::Brute && Unit.bSweep && Unit.SlamTime >= 0.0f; }

	/** True when a place is inside the sweep of a brute standing at Unit's place and facing Unit's way. */
	bool IsInSweep(const FCrowdUnit& Unit, FVector2D Place) const;

	/** How wide the full circle of this unit's slam or slash is: its radius. */
	float GetSlamRadius(const FCrowdUnit& Unit) const;

	/** True while the commander stands drawing back for a spear throw. */
	bool IsWindingThrow(const FCrowdUnit& Unit) const { return Unit.Kind == ECrowdKind::Commander && Unit.ThrowWind >= 0.0f; }

	/** How far through drawing back he is, 0 to 1. 0 when he is not throwing. */
	float GetThrowShare(const FCrowdUnit& Unit) const;

	/** True for the last moment before the throw, when the way the spear will go no longer follows the wizard. */
	bool IsThrowLocked(const FCrowdUnit& Unit) const;

	/** How far along Unit.ThrowAim the spear can fly: its range, or less when a wall stands in the way. */
	double GetThrowReach(const FCrowdUnit& Unit) const;

	/** The commander's spears in the air. */
	const TArray<FCrowdJavelin>& GetJavelins() const { return Javelins; }

	/** True while an awake commander is within Tuning.CommanderAuraRadius of the wizard: enemies on him fill the meter faster. */
	bool IsCommanderNear(FVector2D WizardPosition) const;

	/** How high over where it was loosed an arrow is now: nothing at the bow and at its mark, most half way. */
	float GetArrowHeight(const FCrowdArrow& Arrow) const;

	/** How steeply it climbs (above zero) or falls (below) now, as rise over run. */
	float GetArrowSlope(const FCrowdArrow& Arrow) const;

	/** How large the circle of a slamming brute is now, as a share of its full size: tiny at the start, 1 when full. 0 when it is not slamming. */
	float GetSlamShare(const FCrowdUnit& Unit) const;

	/**
	 * True while the unit is in an attack its model should show, with the seconds until its blow lands (its arrow
	 * leaves, its slam comes down). Below zero: it landed that long ago and the unit is following through.
	 */
	bool GetStrike(const FCrowdUnit& Unit, float& OutTimeToBlow) const;

	/**
	 * True while the commander draws back for a throw or follows one through, with the seconds until the spear
	 * leaves his hand (below zero: it left that long ago).
	 */
	bool GetThrow(const FCrowdUnit& Unit, float& OutTimeToThrow) const;

	/** What happened in the last step that is shown or heard: blows, deaths, arrows loosed. Empty again at the next step. */
	const TArray<FCrowdEvent>& GetEvents() const { return Events; }

	/** Where the commander's throw is aimed from where he stands: at the wizard, or ahead of him when the throw leads. */
	FVector2D GetThrowAimPoint(const FCrowdUnit& Unit, FVector2D WizardPosition) const;
	// What one slam takes from this unit of the army, standing where the wizard is.
	float GetSlamDamage(const FCrowdUnit& Unit, FVector2D WizardPosition) const;

	/** Brutes in a slam now. */
	int32 CountSlamming() const;

	/** Where slams just landed. */
	const TArray<FCrowdBlast>& GetSlamHits() const { return SlamHits; }

	/** Seconds the simulation has run: the clock the picture times its swings by. */
	double GetClock() const { return Clock; }

	/** Where skeletons were just cut down to make a spear. */
	const TArray<FCrowdSlash>& GetSlashes() const { return Slashes; }

	/** While set (the blink), arrows fly through the wizard. */
	bool bWizardInvulnerable = false;

	const TArray<FCrowdArrow>& GetArrows() const { return Arrows; }

	/** How much of a unit shows, 0 to 1. Only an assassin is ever hidden: 0 beyond its reveal range, fading in as it closes. */
	float GetShown(const FCrowdUnit& Unit, FVector2D WizardPosition) const;

	/** True for an assassin beyond its reveal range: it is not drawn and the army does not see it. */
	bool IsCloaked(const FCrowdUnit& Unit, FVector2D WizardPosition) const { return GetShown(Unit, WizardPosition) <= 0.0f; }

	/** True for an assassin the army cannot see yet: farther from the wizard than Tuning.AssassinArmyRange. */
	bool IsHiddenFromArmy(const FCrowdUnit& Unit, FVector2D WizardPosition) const;

	/** Sent units that have left their place to go after an enemy that shot at their group. */
	int32 CountHunting() const;

	/** What the army counts for in bones: each unit is worth Tuning.ArmyValueShare of what it cost, the total rounded down. */
	int32 GetArmyBoneValue() const;

	/** Bones held plus what the army counts for. */
	int32 GetBoneWorth() const { return BonesHeld + GetArmyBoneValue(); }

	/** Bones one of the army leaves when it falls in a fight: Tuning.ArmyDropShare of its cost, a whole number. */
	int32 GetFallenBoneValue(ECrowdKind Kind) const;

	/** True for one of the army within the wizard's presence. Outside it, it is weaker. */
	bool IsInPresence(const FCrowdUnit& Unit, FVector2D WizardPosition) const;

	/** The ground the camera shows, as a box. Bones inside it do not fade; their time starts when they leave it. */
	void SetView(FVector2D Min, FVector2D Max);
	void ClearView() { bHasView = false; }
	bool IsInView(FVector2D Position) const;

	/** Army units of a kind that are with the wizard and can be sent. */
	int32 CountCommandable(ECrowdCommandKind Kind) const;

	/** Sends up to Count of the army of that kind, those nearest the place first, to hold it. Returns how many went. */
	int32 Command(FVector2D Point, int32 Count, ECrowdCommandKind Kind);

	/** How wide a sent group of this many spreads around its place. */
	float GetCommandRadius(int32 Count) const;

	/** Every sent unit comes back to the wizard. Returns how many were out. */
	int32 Recall();

	/** Army units holding a place they were sent to. */
	int32 CountCommanded() const;

	/** The places units are holding, one for each order still standing. */
	void GetCommandPoints(TArray<FVector2D>& OutPoints) const;

	static const TCHAR* GetCommandKindName(ECrowdCommandKind Kind);

	/** False when the enemy cap is reached. A position inside a wall is moved out of it. */
	bool AddEnemy(ECrowdKind Kind, FVector2D Position);

	/** False when the skeleton cap is reached. A position inside a wall is moved out of it. */
	bool AddSkeleton(FVector2D Position, ECrowdKind Kind = ECrowdKind::Skeleton);

	/** An enemy that stands at its post until its group is woken. False when the enemy cap is reached. */
	bool AddGarrisonEnemy(ECrowdKind Kind, FVector2D Position, int32 Squad, float PostRadius = 0.0f, FVector2D Post = FVector2D::ZeroVector);

	/** A road for patrols to walk: a line of points. Returns its index. */
	int32 AddRoute(const TArray<FVector2D>& Points);
	void ClearRoutes() { Routes.Reset(); }
	int32 GetRouteCount() const { return Routes.Num(); }

	/**
	 * An enemy on patrol: it walks its route back and forth, heading first for RoutePoint, until its group notices the
	 * wizard the way a post does. False when the route does not exist or the enemy cap is reached.
	 */
	bool AddPatrolEnemy(ECrowdKind Kind, FVector2D Position, int32 Squad, int32 Route, int32 RoutePoint = 0, int32 RouteStep = 1);

	/** Living enemies of a group. */
	int32 CountSquad(int32 Squad) const;

	/** Enemies still standing at their posts. */
	int32 CountAsleep() const;

	/** Enemies doing this about the wizard now: looking for him (Seek, Look), going back (Return). Those at a post are not counted. */
	int32 CountAlert(ECrowdAlert Alert) const;

	/** True when this enemy can see that place: within Tuning.SightRadius and no wall between. Always true with SightRadius 0. */
	bool CanSee(const FCrowdUnit& Unit, FVector2D Place) const;

	/** Every enemy at a post charges. */
	void WakeAll();

	/** The manned post nearest a point: where one of its enemies stands. False when no post is manned. */
	bool GetNearestPost(FVector2D From, FVector2D& OutPost) const;

	/** True when this research has been bought. For the Guard, which is a unit, always false. */
	bool IsResearched(ECrowdShopItem Item) const;

	/** Health, speed and damage of a kind of unit, from Tuning. */
	FCrowdKindStats GetStats(ECrowdKind Kind) const;

	/** How tall a kind of unit stands beside a wizard of WizardHeight: its size share from Tuning times his height. */
	float GetUnitHeight(ECrowdKind Kind, float WizardHeight) const;

	/** True when the spear is off cooldown and the army and bones can pay for it. */
	bool CanThrowSpear() const;

	/**
	 * Takes the skeletons the spear costs (plain ones before Guards, the nearest to From first) and throws it at Target.
	 * False, with nothing spent, when CanThrowSpear is false.
	 */
	bool ThrowSpear(FVector2D From, FVector2D Target);

	const TArray<FCrowdSpear>& GetSpears() const { return Spears; }
	const TArray<FCrowdBlast>& GetBlasts() const { return Blasts; }

	/** Seconds until the spear can be thrown again. */
	float GetSpearCooldown() const { return SpearCooldown; }

	void FireBolt(FVector2D From, FVector2D Direction);

	/** Puts bones on the ground. False when the ground already holds Tuning.MaxBones piles. */
	bool DropBone(FVector2D Position, int32 Value, bool bGuard = false);

	void SetObstacles(const TArray<FCrowdObstacle>& InObstacles);
	const TArray<FCrowdObstacle>& GetObstacles() const { return Obstacles; }

	/** True when a unit can walk the straight line between the two points without meeting a wall. */
	bool HasClearPath(FVector2D From, FVector2D To) const;

	/** Advances the crowd. A step longer than Tuning.MaxStep is treated as MaxStep. */
	FCrowdStepResult Step(float DeltaSeconds, FVector2D WizardPosition);

	const TArray<FCrowdUnit>& GetUnits() const { return Units; }
	const TArray<FCrowdBolt>& GetBolts() const { return Bolts; }
	int32 CountTeam(ECrowdTeam Team) const;

	const TArray<FCrowdBone>& GetBones() const { return Bones; }

	/** Bones a dead unit of this kind leaves: 0 for the army's own kinds. */
	int32 GetBoneValue(ECrowdKind Kind) const;

	/** Skeletons one cast of Summon the Afterlife would raise right now: limited by bones held, SummonMax and the army cap. */
	int32 GetSummonCount() const;

	/** Summon the Afterlife: spends held bones on plain skeletons beside the wizard. Returns how many were raised. */
	int32 Summon(FVector2D WizardPosition);

	/** The Spear of Sacrifice is bought at the shrine. */
	bool IsSpearUnlocked() const { return bSpearUnlocked; }
	void SetSpearUnlocked(bool bUnlocked) { bSpearUnlocked = bUnlocked; }

	/** What one Bolt does, with research. */
	float GetBoltDamage() const { return Tuning.BoltDamage + (bFocus ? Tuning.FocusBoltDamage : 0.0f); }

	/** What one of the army counts for in bones: Tuning.ArmyValueShare of its cost, a whole number. */
	int32 GetUnitWorth(ECrowdKind Kind) const;

	/**
	 * How Count of an item would be paid for right now: bones held first, then plain skeletons with the wizard, then
	 * Guards with him (never Guards for a Guard). Sent units do not pay.
	 */
	FCrowdShopPrice GetShopPrice(ECrowdShopItem Item, int32 Count) const;

	/** Army units given up to pay at the shrine in this run. */
	int32 GetSacrificed() const { return Sacrificed; }

	/** What one of this item costs, in bones. */
	int32 GetShopCost(ECrowdShopItem Item) const;

	/** How many of this item can be bought right now: limited by bones held, the army cap, and the item itself. */
	int32 GetShopMax(ECrowdShopItem Item) const;

	/** Buys up to Count of an item in one go. Returns how many were bought; bones are spent only for those. */
	int32 Buy(ECrowdShopItem Item, int32 Count, FVector2D WizardPosition);

	static const TCHAR* GetShopName(ECrowdShopItem Item);

	/** Bones the wizard has collected and not yet spent. */
	int32 GetBonesHeld() const { return BonesHeld; }

	/** How many groups an army of this size stands in: 0 when groups are off. */
	int32 GetGroupCount(int32 ArmySize) const;

	/** Offsets from the wizard to every enemy within Tuning.TouchRange. */
	void GetEnemiesTouching(FVector2D WizardPosition, TArray<FVector2D>& OutOffsets) const;

	/** Removes every unit, bolt and bone, and empties the wizard's bones. Walls stay. */
	void Reset();

private:
	int32 WakeGarrisons(FVector2D WizardPosition);
	void UpdateSight(float DeltaSeconds, FVector2D WizardPosition);
	static int32 GetSightKey(const FCrowdUnit& Unit) { return Unit.Squad != INDEX_NONE ? Unit.Squad : -2 - Unit.Id; }
	int32 FindSight(int32 Key) const;
	int32 FindOrAddSight(int32 Key, FVector2D Place);
	void Rouse(FCrowdUnit& Unit, FVector2D Place);
	void ChooseTargets(FVector2D WizardPosition);
	void MoveUnits(float DeltaSeconds, FVector2D WizardPosition, FCrowdStepResult& Result);
	bool HasClearShot(FVector2D From, FVector2D To) const;
	void MoveAtPost(FCrowdUnit& Unit, float DeltaSeconds);
	void UpdateRoam(FCrowdUnit& Unit, float DeltaSeconds, FVector2D WizardPosition);
	FVector2D PickRoamOffset(FVector2D WizardPosition);
	FVector2D PickGroupOffset(FCrowdUnit& Unit, FVector2D WizardPosition);
	FVector2D GetGroupMiddle(int32 Group, int32 GroupCount) const;
	void UpdateBones(float DeltaSeconds, FVector2D WizardPosition, FCrowdStepResult& Result);
	void SeparateUnits();
	void KeepOutOfObstacles();
	void PushOutOfObstacles(FVector2D& Position) const;
	void ApplyMelee(float DeltaSeconds, FVector2D WizardPosition, FCrowdStepResult& Result);
	void UpdateSlam(FCrowdUnit& Unit, float DeltaSeconds, FVector2D WizardPosition, FCrowdStepResult& Result);
	bool UpdateThrow(FCrowdUnit& Unit, float DeltaSeconds, FVector2D WizardPosition, FCrowdStepResult& Result);
	void UpdateMarksman(FCrowdUnit& Unit, float DeltaSeconds, FVector2D WizardPosition, FCrowdStepResult& Result);
	void MoveJavelins(float DeltaSeconds, FVector2D WizardPosition, FCrowdStepResult& Result);
	void UpdateFacing(float DeltaSeconds, FVector2D WizardPosition);
	int32 CountSpearSkeletons() const;
	bool IsSpotTaken(const FCrowdUnit& Unit, FVector2D Spot) const;
	bool PickOpenOffset(FVector2D WizardPosition, FVector2D& OutOffset);
	void MoveBolts(float DeltaSeconds);
	void MoveArcher(FCrowdUnit& Unit, float DeltaSeconds, FVector2D WizardPosition, FCrowdStepResult& Result);
	void MoveArrows(float DeltaSeconds, FVector2D WizardPosition, FCrowdStepResult& Result);
	void UpdateCommands(FVector2D WizardPosition, FCrowdStepResult& Result);
	void RespondToShooter(FVector2D Point, int32 ShooterId);
	void EndHunt(FCrowdOrder& Order);
	FVector2D PickCommandOffset(const FCrowdUnit& Unit);
	int32 FindOrder(FVector2D Point) const;
	int32 FindUnit(int32 Id) const;
	bool IsInsideObstacle(FVector2D Position, double Margin) const;
	void MoveSpears(float DeltaSeconds, FCrowdStepResult& Result);
	void RemoveDead(FCrowdStepResult& Result);

	void AddEvent(ECrowdEvent Type, FVector2D Position, ECrowdKind Kind = ECrowdKind::Grunt, ECrowdTeam Team = ECrowdTeam::Enemy);
	void UpdateAssassinSwing(FCrowdUnit& Unit, float DeltaSeconds, FVector2D WizardPosition);
	TArray<FCrowdEvent> Events;

	TArray<FCrowdUnit> Units;
	TArray<FCrowdBolt> Bolts;
	TArray<FCrowdArrow> Arrows;
	TArray<FCrowdJavelin> Javelins;
	int32 NextJavelinId = 1;
	TArray<FCrowdBone> Bones;
	int32 BonesHeld = 0;
	int32 GuardsOwed = 0;
	int32 Sacrificed = 0;
	bool bArmor = false;
	bool bBlades = false;
	bool bFocus = false;
	bool bSpearUnlocked = false;
	TArray<FCrowdSpear> Spears;
	TArray<FCrowdBlast> Blasts;
	TArray<FCrowdSlash> Slashes;
	TArray<FCrowdBlast> SlamHits;
	double Clock = 0.0;
	float SpearCooldown = 0.0f;
	TArray<FCrowdObstacle> Obstacles;
	TArray<TArray<FVector2D>> Routes;
	FCrowdFlowField Flow;
	TArray<FCrowdOrder> Orders;
	TArray<FCrowdSight> Sights;
	bool bHasView = false;
	FVector2D ViewMin = FVector2D::ZeroVector;
	FVector2D ViewMax = FVector2D::ZeroVector;
	// How the wizard moved over the last step, for archers to aim ahead of him.
	FVector2D WizardVelocity = FVector2D::ZeroVector;
	FVector2D LastWizardPosition = FVector2D::ZeroVector;
	bool bHasLastWizardPosition = false;
	int32 NextId = 1;
};
