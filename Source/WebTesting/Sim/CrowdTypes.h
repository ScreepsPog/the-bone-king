#pragma once

#include "CoreMinimal.h"

enum class ECrowdTeam : uint8 { Army, Enemy };

// Enemies: Grunt (weak, many), Brute (tough, slow, hits hard), Commander, Archer (shoots from a distance and runs when
// charged), Assassin (fast, cloaked, goes around the army for the wizard). Army: Skeleton, Guard (from a brute's bones),
// Marksman (stands off and shoots: archers first, then the enemy with the most health).
enum class ECrowdKind : uint8 { Skeleton, Grunt, Brute, Guard, Commander, Archer, Assassin, Marksman };

/**
 * What an enemy from a post or a patrol is doing about the wizard. Chase: one of its group sees him. Seek: nobody does,
 * it goes to where he was last seen. Look: it stands there and looks about. Return: it runs back to where it stood
 * when it noticed him, and takes up its post or its patrol again.
 */
enum class ECrowdAlert : uint8 { Chase, Seek, Look, Return };

/** Which of the army a command sends. The order is the order right mouse steps through. */
enum class ECrowdCommandKind : uint8 { Any, Skeleton, Guard };
inline constexpr int32 CrowdCommandKindCount = 3;

/**
 * What the shrine sells. The order is the order of the rows in its menu.
 * Spear, Armor, Blades and Focus are research: bought once. Guard and Marksman are units, bought as often as bones allow.
 */
enum class ECrowdShopItem : uint8 { Spear, Armor, Blades, Focus, Guard, Marksman };
inline constexpr int32 CrowdShopItemCount = 6;

/** What a purchase at the shrine takes: bones held first, then plain skeletons, then Guards, who are sacrificed. */
struct FCrowdShopPrice
{
	int32 Bones = 0;
	int32 Skeletons = 0;
	int32 Guards = 0;
	// False when bones and army together do not reach the price.
	bool bCanPay = false;
};

/** The numbers that differ between kinds of unit. */
struct FCrowdKindStats
{
	float Health = 0.0f;
	float Speed = 0.0f;
	float DamagePerSecond = 0.0f;
};

struct FCrowdUnit
{
	FVector2D Position = FVector2D::ZeroVector;
	float Health = 0.0f;
	float MaxHealth = 0.0f;
	// Seconds left to show this unit's health bar. Set when it takes damage, so a bar is drawn only for a unit that was just hit.
	float HurtTimer = 0.0f;
	ECrowdTeam Team = ECrowdTeam::Enemy;
	ECrowdKind Kind = ECrowdKind::Grunt;
	int32 TargetIndex = INDEX_NONE;

	// Army: where this skeleton idles, as an offset from the wizard, and how long until it picks another spot.
	FVector2D RoamOffset = FVector2D::ZeroVector;
	float RoamTimer = 0.0f;
	// Army: idle from where it stands now (set when it is raised and when a fight ends), so it does not run back to an old spot.
	bool bSettle = true;
	bool bFighting = false;
	// Army: which group this skeleton stands in, or INDEX_NONE before it has chosen one.
	int32 Group = INDEX_NONE;

	// Enemy: -1 to 1, bends the approach to one side so a wave arcs in and surrounds instead of queueing.
	float Flank = 0.0f;
	// Enemy: this unit's share of the kind's speed, so a wave strings out.
	float SpeedScale = 1.0f;
	// Enemy: standing at its post. It does not move or fight until its group is woken.
	bool bAsleep = false;
	// Enemy: the group it stands with, or INDEX_NONE. A group wakes together.
	int32 Squad = INDEX_NONE;
	// Enemy at a post: the spot it was posted at, and whether it walks about near it or stands still.
	FVector2D Home = FVector2D::ZeroVector;
	bool bWanders = false;
	// Enemy at a post: how far from the spot it was posted at it walks about. 0: Tuning.PostWanderRadius.
	float PostRadius = 0.0f;
	// Enemy on patrol: the route it walks until it notices the wizard, the point it is heading for, and which way along the route.
	int32 Route = INDEX_NONE;
	int32 RoutePoint = 0;
	int32 RouteStep = 1;
	// Enemy from a post or a patrol: it can lose the wizard. One that was sent after him (a wave, a raid) never does.
	bool bPosted = false;
	ECrowdAlert Alert = ECrowdAlert::Chase;
	// Seconds left of what it is doing: reaching the place, looking about, or getting back.
	float AlertTimer = 0.0f;
	// Where it stood when it noticed him: where it runs back to.
	FVector2D ReturnPoint = FVector2D::ZeroVector;
	// Its health when it last took up its post: it is roused by being hurt below this.
	float WakeHealth = 0.0f;
	// Archer: seconds until it can loose another arrow.
	float AttackTimer = 0.0f;

	// Army: sent to hold a place. It stays there, fights what comes near, and rejoins the wizard when he comes close to
	// the place, once he has been away from it since the order.
	bool bCommanded = false;
	bool bCommandArmed = false;
	FVector2D CommandPoint = FVector2D::ZeroVector;
	// Army, sent: how far from the place it may stand and wander. Set from the size of the group that was sent.
	float CommandRadius = 0.0f;
	// Army, sent: it has left its place to go after an enemy that shot at the group.
	bool bHunting = false;

	// Never reused, so a unit can be found again after the list has been reordered.
	int32 Id = 0;
	// Which way it faces: the way it last walked, or toward what it strikes. Zero until it has moved. A brute turns slowly.
	FVector2D Facing = FVector2D::ZeroVector;
	// Which way it wants to face this step. A brute turns toward it slowly; everything else faces it at once.
	FVector2D FaceGoal = FVector2D::ZeroVector;
	// True in a step in which it stands in reach of its target, striking, or (a brute) is in its slam. A unit that
	// was striking at the end of a step does not walk in the next one, and it holds the way it faces until the blow lands.
	bool bStriking = false;
	// Whether it walked in this step, and how fast and which way. An archer aims ahead of a moving target with it.
	bool bMoving = false;
	FVector2D Velocity = FVector2D::ZeroVector;
	// Where it stood when this step began.
	FVector2D StepStart = FVector2D::ZeroVector;
	// Army: seconds until it may next give up a spot it cannot reach or that is taken.
	float RepickTimer = 0.0f;
	// The enemy (a unit's Id) it last started a blow at, or 0. While that one runs from it, the blow stays wound up.
	int32 StrikeId = 0;
	// Brute: seconds into its slam, or below zero when it is not slamming, and the middle of the circle it will land in.
	float SlamTime = -1.0f;
	FVector2D SlamCenter = FVector2D::ZeroVector;
	// Commander: seconds until he can throw his spear again, and seconds into drawing back for a throw (below zero: not throwing).
	float ThrowTimer = 0.0f;
	float ThrowWind = -1.0f;
	// Commander, drawing back: the way the spear will go. It follows the wizard, then holds for the last moment before the throw.
	FVector2D ThrowAim = FVector2D::ZeroVector;
	// Commander: this throw is aimed at where the wizard will be, not where he is. Decided when he starts to draw back.
	bool bThrowLeads = false;
	// Marksman: seconds left of backing away from an enemy that came close, the way it backs, and whether an enemy
	// is close now (it decides once each time one comes close whether to move).
	float RelocateTimer = 0.0f;
	FVector2D RelocateAway = FVector2D::ZeroVector;
	bool bThreatened = false;
	// Archer: it stands with a target in range, shooting. (The models draw their bows by it.)
	bool bAiming = false;
	// Brute and commander: seconds since its slam landed, while it rests after it. Below zero: no slam just landed.
	float BlowAgo = -1.0f;
	// Brute: the attack it is in, or was last in, is its sweep and not its slam; and the seconds until it can slam again.
	bool bSweep = false;
	float HeavyTimer = 0.0f;
	// Commander: seconds since his spear left his hand, while he follows the throw through. Below zero: none just thrown.
	float ThrowAgo = -1.0f;
	// Assassin: it is in a swing at the wizard. (Its swing does nothing in the numbers; it is seen and heard.)
	bool bSwinging = false;
	// Where the last thing to hurt it came from: which way it falls when that was the end of it.
	FVector2D LastHitFrom = FVector2D::ZeroVector;
	bool bWasHit = false;
	// The last place it stood that was clear of every wall and all water.
	FVector2D LastFree = FVector2D::ZeroVector;
	bool bHasLastFree = false;
};

/**
 * An arrow in the air: an enemy archer's, or a Marksman's (bArmy). It flies in an arc from Start to Target, over the
 * heads of whatever stands between, and can hit only as it comes down near Target: the wizard or his army (an enemy's
 * arrow), or an enemy (a Marksman's). It stops at walls.
 */
struct FCrowdArrow
{
	// Who loosed it (a unit's Id).
	int32 Shooter = 0;
	FVector2D Position = FVector2D::ZeroVector;
	FVector2D Velocity = FVector2D::ZeroVector;
	float Life = 0.0f;
	FVector2D Start = FVector2D::ZeroVector;
	FVector2D Target = FVector2D::ZeroVector;
	bool bArmy = false;
	// A Marksman out of the wizard's presence hits less hard.
	float DamageScale = 1.0f;
};

/** The commander's spear in the air. It flies straight at where the wizard stood, through the army, and stops at walls. */
struct FCrowdJavelin
{
	FVector2D Position = FVector2D::ZeroVector;
	FVector2D Velocity = FVector2D::ZeroVector;
	float Life = 0.0f;
	// Its own number (never 0), so what follows it (its sound) can find it again.
	int32 Id = 0;
	// The army's units it has already gone through: each is hit once.
	TArray<int32> HitIds;
};

struct FCrowdBolt
{
	FVector2D Position = FVector2D::ZeroVector;
	FVector2D Velocity = FVector2D::ZeroVector;
	float Life = 0.0f;
};

/** Bones an enemy left where it died. The wizard collects them by walking near; enough of them assemble a skeleton. */
struct FCrowdBone
{
	FVector2D Position = FVector2D::ZeroVector;
	int32 Value = 1;
	float Life = 0.0f;
	// A brute's bones: collecting them assembles a Guard at once, and they do not add to the plain bone count.
	bool bGuard = false;
};

/** A bone spear in the air. It explodes when it reaches Target. */
struct FCrowdSpear
{
	FVector2D Position = FVector2D::ZeroVector;
	FVector2D Target = FVector2D::ZeroVector;
};

/** Where a skeleton was cut down for its bones to make a spear, kept for a moment so the picture can show the cut. */
struct FCrowdSlash
{
	FVector2D Position = FVector2D::ZeroVector;
	float Age = 0.0f;
};

/** Where a spear just exploded, or a slam or slash just landed, kept for a moment so the picture can show it. */
struct FCrowdBlast
{
	FVector2D Position = FVector2D::ZeroVector;
	float Age = 0.0f;
	// A slam or slash: how wide it was. 0: the picture's own size.
	float Radius = 0.0f;
};

/** A wall or pillar: a box on the ground plane, sides along the X and Y axes. Blocks units and bolts. */
struct FCrowdObstacle
{
	FVector2D Center = FVector2D::ZeroVector;
	FVector2D HalfSize = FVector2D(50.0, 50.0);
	// Water and the like: nothing walks through it, but Bolts and the spear fly over.
	bool bLow = false;
	// A house or a lamp post: nothing walks through it, but the wizard's Bolt passes through (director, 2026-11-15).
	bool bBoltPasses = false;

	/** True when the point is inside the box grown by Margin on every side. */
	bool Contains(FVector2D Point, double Margin) const
	{
		return FMath::Abs(Point.X - Center.X) < HalfSize.X + Margin && FMath::Abs(Point.Y - Center.Y) < HalfSize.Y + Margin;
	}

	/** Where a segment first enters the box grown by Margin, as 0 to 1 along the segment. False when it misses. */
	bool SegmentEnters(FVector2D From, FVector2D To, double Margin, double& OutAlpha) const
	{
		const FVector2D Min = Center - HalfSize - FVector2D(Margin, Margin);
		const FVector2D Max = Center + HalfSize + FVector2D(Margin, Margin);
		const FVector2D Delta = To - From;
		double Enter = 0.0;
		double Exit = 1.0;
		for (int32 Axis = 0; Axis < 2; ++Axis)
		{
			const double Start = From[Axis];
			const double Move = Delta[Axis];
			if (FMath::Abs(Move) < UE_DOUBLE_SMALL_NUMBER)
			{
				if (Start <= Min[Axis] || Start >= Max[Axis])
				{
					return false;
				}
				continue;
			}
			double Near = (Min[Axis] - Start) / Move;
			double Far = (Max[Axis] - Start) / Move;
			if (Near > Far)
			{
				Swap(Near, Far);
			}
			Enter = FMath::Max(Enter, Near);
			Exit = FMath::Min(Exit, Far);
			if (Enter > Exit)
			{
				return false;
			}
		}
		OutAlpha = Enter;
		return true;
	}

	/** Moves a point that is inside the grown box out through its nearest side. False when it was already outside. */
	bool PushOut(FVector2D& Point, double Margin) const
	{
		const FVector2D Offset = Point - Center;
		const double ReachX = HalfSize.X + Margin - FMath::Abs(Offset.X);
		const double ReachY = HalfSize.Y + Margin - FMath::Abs(Offset.Y);
		if (ReachX <= 0.0 || ReachY <= 0.0)
		{
			return false;
		}
		if (ReachX < ReachY)
		{
			Point.X += Offset.X >= 0.0 ? ReachX : -ReachX;
		}
		else
		{
			Point.Y += Offset.Y >= 0.0 ? ReachY : -ReachY;
		}
		return true;
	}
};

struct FCrowdTuning
{
	float GruntHealth = 10.0f;
	float GruntSpeed = 300.0f;
	float GruntDamagePerSecond = 5.0f;

	// The Brute: slow, takes eight bolts, and hits more than twice as hard as a grunt.
	float BruteHealth = 80.0f;
	float BruteSpeed = 190.0f;
	float BruteDamagePerSecond = 12.0f;

	// The Guard: the skeleton assembled from a brute's bones. Three skeletons' worth of health.
	float GuardHealth = 60.0f;
	float GuardSpeed = 420.0f;
	float GuardDamagePerSecond = 14.0f;

	// How tall each kind stands, as a share of the wizard's height (220 units), so the crowd keeps its scale beside him
	// whatever his height is set to. From the director's table: a plain skeleton 150 units (about two thirds of him),
	// a Guard 175, a grunt 170 (an even fight for a Guard), a brute 240 (taller than him: the threat). A commander
	// stands over the brute.
	float SkeletonSize = 150.0f / 220.0f;
	float GruntSize = 170.0f / 220.0f;
	float GuardSize = 175.0f / 220.0f;
	float BruteSize = 240.0f / 220.0f;
	float CommanderSize = 1.3f;

	// Blink: a dash the way he is moving that nothing can hurt him during. It covers BlinkRange in BlinkSeconds as a
	// burst (fastest at the start, slowing to a stop), slides along walls and water, and can be cast again
	// BlinkCooldown seconds after it ends.
	float BlinkRange = 700.0f;
	float BlinkSeconds = 0.65f;
	float BlinkCooldown = 3.0f;
	// Holding a direction during the blink bends it: held for the whole blink, it moves him sideways this share of
	// BlinkRange, at an even pace, so the end of the blink steers as much as its start.
	float BlinkSteer = 0.6f;

	// The Archer: weaker than a grunt. It stands off and looses an arrow every ArcherInterval seconds at the wizard or
	// the nearest of his army within ArcherRange, when no wall is in the way. An arrow does more than a grunt does in
	// a second: ArrowDamage to a skeleton, ArrowOverwhelm (a share of the meter) to the wizard. It runs from the wizard
	// and from any of his army that come within ArcherFleeRange, and does not shoot while it runs.
	float ArcherHealth = 6.0f;
	float ArcherSpeed = 280.0f;
	float ArcherRange = 800.0f;
	float ArcherInterval = 1.6f;
	float ArcherFleeRange = 380.0f;
	float ArrowSpeed = 1100.0f;
	float ArrowDamage = 12.0f;
	float ArrowOverwhelm = 0.12f;
	float ArrowRadius = 45.0f;
	float BonesPerArcher = 1.0f;
	float ArcherSize = 165.0f / 220.0f;
	// This share of the grunts at a post are archers instead.
	float PostArcherShare = 0.2f;

	// The Assassin: fast and frail. It never fights the army: it steers around skeletons within AssassinAvoidRange and
	// goes for the wizard, where it counts as AssassinTouchWeight enemies on the Overwhelm meter. Its cloak hides it
	// from the player until it is within AssassinRevealRange of the wizard; it fades in over the next
	// AssassinRevealFade units, and out again if he gets away. The army sees it only within AssassinArmyRange.
	// One is sent every AssassinInterval seconds while the commander lives (0: never).
	float AssassinHealth = 8.0f;
	float AssassinSpeed = 540.0f;
	float AssassinTouchWeight = 4.0f;
	float AssassinRevealRange = 800.0f;
	float AssassinRevealFade = 500.0f;
	float AssassinAvoidRange = 170.0f;
	float BonesPerAssassin = 2.0f;
	float AssassinSize = 165.0f / 220.0f;
	float AssassinInterval = 40.0f;

	// Commanding the army: sent skeletons stand within CommandScatter of the place, and rejoin the wizard when he
	// comes within CommandRejoinRadius of it.
	float CommandRejoinRadius = 300.0f;
	float CommandScatter = 90.0f;
	// A sent group spreads over a circle that grows with its size: CommandSpacing times the square root of how many
	// went, and never less than CommandScatter. They walk about inside it.
	float CommandSpacing = 55.0f;
	// Shot at from out of reach, a sent group sends this share of itself after the shooter, no farther than
	// CommandRespondRange from the place. The rest hold.
	float CommandRespondShare = 0.4f;
	float CommandRespondRange = 2000.0f;

	// An enemy fights the nearest of the army within EnemyEngageRange of it when the wizard is farther than
	// EnemyWizardRange from it. With the wizard nearer than that, it goes for him as before.
	float EnemyEngageRange = 350.0f;
	float EnemyWizardRange = 700.0f;

	// The army sees an assassin only within this distance of the wizard, whatever the player can already see.
	float AssassinArmyRange = 450.0f;

	// Blink: BlinkSteerEarly of the steering follows the burst (most of it at the start), the rest goes at an even
	// pace. He cannot be seen from the start of the blink until BlinkHiddenShare of its time has passed.
	float BlinkSteerEarly = 0.6f;
	float BlinkHiddenShare = 0.7f;

	// One of the army that falls in a fight leaves this share of what it cost as bones, rounded to a whole number.
	float ArmyDropShare = 1.0f / 3.0f;

	// An archer aims where its target will be when the arrow lands: ArcherLead of the way there (1 is exact, 0 is
	// where the target stands now). Each arrow is off by up to ArcherSpreadDegrees to either side.
	float ArcherLead = 1.0f;
	float ArcherSpreadDegrees = 4.0f;

	// The wizard's presence. One of the army farther than PresenceRadius from him does AwayDamageShare of its damage
	// and takes AwayDamageTaken times the damage dealt to it.
	float PresenceRadius = 600.0f;
	float AwayDamageShare = 0.5f;
	float AwayDamageTaken = 1.5f;

	// Melee is one blow at a time: a unit in reach of its target strikes every MeleeInterval seconds, for its damage
	// per second times that. The first blow lands MeleeFirstShare of an interval after it gets in reach. 0: a steady drain.
	float MeleeInterval = 1.0f;
	float MeleeFirstShare = 0.5f;

	// How much room a unit takes: its body is a circle of this share of its height in radius (a brute's is
	// BruteBodyScale times wider). Two units never stand closer than their two radii, or SeparationRadius if that is more.
	float BodyShare = 0.25f;
	// The wizard's own body: an enemy going for him stops where its body meets his.
	float WizardBodyRadius = 50.0f;
	float BruteBodyScale = 1.4f;

	// The brute's armor is at its front. A blow or Bolt that comes from within its front arc does BruteFrontTaken of
	// its damage; from within the back arc, BruteBackTaken. BruteArc is how wide each arc is (0.5: 60 degrees to
	// either side). It turns to face what it goes for at BruteTurnDegrees a second, so it can be got behind.
	float BruteFrontTaken = 0.5f;
	float BruteBackTaken = 2.0f;
	float BruteArc = 0.5f;
	float BruteTurnDegrees = 140.0f;

	// The brute's attack is a slam. When what it goes for is in front of it, a red circle BruteSlamRadius across the
	// middle, BruteSlamDistance ahead of the brute, grows from BruteSlamStartShare of its size to full over
	// BruteSlamGrow seconds. BruteSlamHold seconds after it is full the slam lands: BruteSlamDamage to each of the
	// army inside the circle, BruteSlamOverwhelm (a share of the meter) to the wizard if he is inside it. The brute
	// stands and does not turn from the start of the circle until the slam lands, then rests BruteSlamRest seconds.
	float BruteSlamRadius = 170.0f;
	float BruteSlamDistance = 190.0f;
	float BruteSlamStartShare = 0.08f;
	float BruteSlamGrow = 1.0f;
	float BruteSlamHold = 0.3f;
	float BruteSlamDamage = 40.0f;
	// One slam never kills a guard, nor a skeleton once bone armor is researched: instead of BruteSlamDamage it takes
	// from them all but this share of their full health (so a hurt one can still die to it, and a second slam kills).
	float BruteSlamArmoredLeft = 1.0f / 3.0f;
	float BruteSlamGuardLeft = 0.5f;
	float BruteSlamOverwhelm = 0.35f;
	float BruteSlamRest = 1.2f;
	// The slam starts when what the brute goes for is within this share of the circle's radius of its middle.
	float BruteSlamAimShare = 0.75f;
	// The brute's plain attack is a sweep of its hammer (director, 2026-11-15 00:20 PST): everything of the army
	// within BruteSweepRadius of it and inside BruteSweepArc degrees across its front (a third of a circle) takes
	// BruteSweepDamage, and the wizard, if he is in it, BruteSweepOverwhelm of the meter. It winds up for
	// BruteSweepWindup seconds, standing, then rests BruteSweepRest. The slam is its heavy attack: it slams when
	// one is ready (BruteSlamEvery seconds after the last began) and what it goes for is in the circle, and sweeps
	// otherwise. With BruteSlamEvery 0 it only ever slams, as before.
	float BruteSweepRadius = 250.0f;
	float BruteSweepArc = 120.0f;
	float BruteSweepWindup = 0.5f;
	float BruteSweepRest = 0.9f;
	float BruteSweepDamage = 16.0f;
	float BruteSweepOverwhelm = 0.15f;
	float BruteSlamEvery = 6.0f;
	// Seconds the flash stays on screen where a slam landed.
	float SlamFlashSeconds = 0.3f;

	// The commander. He always goes for the wizard and does not trade blows: he slashes. The slash is the brute's slam
	// made quick: the same growing circle, CommanderSlashRadius across the middle, CommanderSlashDistance ahead of him,
	// full after CommanderSlashGrow seconds and landing CommanderSlashHold after that. CommanderSlashDamage to each of
	// the army in it, CommanderSlashOverwhelm (a share of the meter) to the wizard. He turns as he strikes, so it is
	// always in front of him, and it is how he gets through an army that blocks him.
	float CommanderSlashRadius = 190.0f;
	float CommanderSlashDistance = 170.0f;
	float CommanderSlashGrow = 0.5f;
	float CommanderSlashHold = 0.15f;
	float CommanderSlashDamage = 20.0f;
	float CommanderSlashOverwhelm = 0.4f;
	float CommanderSlashRest = 0.9f;
	// His spear. Every CommanderSpearInterval seconds (0: never), with the wizard between CommanderSpearMinRange and
	// CommanderSpearRange of him and no wall between, he stands and draws back for CommanderSpearWindup seconds, then
	// throws at where the wizard stands. It flies at CommanderSpearSpeed through the army and stops at walls. Within
	// CommanderSpearRadius of the wizard it hits: CommanderSpearOverwhelm (a share of the meter), and he walks at
	// CommanderSpearSlow of his speed for CommanderSpearSlowSeconds. A blink goes through it.
	float CommanderSpearInterval = 6.0f;
	float CommanderSpearMinRange = 350.0f;
	float CommanderSpearRange = 1200.0f;
	float CommanderSpearWindup = 0.7f;
	// The warning: while he draws back, a red lane on the ground runs from him the way the spear will go, growing to
	// its full reach. It follows the wizard until CommanderSpearLock seconds before the throw; then it holds and
	// shows bright, and that is where the spear flies. Stepping out of the bright lane, or a blink, is the dodge.
	float CommanderSpearLock = 0.25f;
	// Some throws are aimed ahead of him, at where he will be when the spear gets there if he keeps going as he is
	// (director, 2026-11-14: "predict sometimes", not every time): this share of them. The warning lane shows it.
	float CommanderSpearLeadChance = 0.5f;
	float CommanderSpearSpeed = 1500.0f;
	float CommanderSpearRadius = 70.0f;
	float CommanderSpearOverwhelm = 0.2f;
	float CommanderSpearSlow = 0.5f;
	float CommanderSpearSlowSeconds = 2.5f;
	// What his spear does to each of the army it flies through (each once; it does not stop for them).
	float CommanderSpearDamage = 20.0f;
	// On the wizard he counts as this many enemies on the meter (an assassin: AssassinTouchWeight). While he is awake
	// and within CommanderAuraRadius of the wizard, every enemy on the wizard fills the meter CommanderAuraFill times as fast.
	float CommanderTouchWeight = 5.0f;
	float CommanderAuraRadius = 900.0f;
	float CommanderAuraFill = 1.5f;

	// The Marksman: one of the army that never closes to fight. Every MarksmanInterval seconds it shoots an enemy within
	// MarksmanRange that no wall hides: the nearest archer, or with none, the enemy with the most health. Its arrow
	// kills an archer outright, does MarksmanBruteDamage to a brute (a little more than a skeleton's blow, before the
	// brute's armor) and MarksmanDamage to anything else. Bought at the shrine for MarksmanCost bones.
	float MarksmanHealth = 15.0f;
	float MarksmanSpeed = 430.0f;
	float MarksmanRange = 900.0f;
	float MarksmanInterval = 1.5f;
	float MarksmanDamage = 8.0f;
	float MarksmanBruteDamage = 10.0f;
	float MarksmanCost = 6.0f;
	// It stands still to shoot: for the last MarksmanDraw seconds before each arrow, while it has something to shoot
	// at, it does not follow the wizard. After the arrow it catches up.
	float MarksmanDraw = 0.8f;
	// It keeps away from the enemy: each time one comes within MarksmanKeepAway of it, MarksmanRelocateChance is the
	// chance that it backs away for MarksmanRelocateSeconds (and does not shoot while it does). Not always.
	float MarksmanKeepAway = 260.0f;
	float MarksmanRelocateChance = 0.6f;
	float MarksmanRelocateSeconds = 1.2f;
	// The battlefield has no wall: outside it he is warned to come back. The mission fails after this long outside,
	// or at once this far outside.
	float BattlefieldWarnSeconds = 6.0f;
	float BattlefieldFarDistance = 900.0f;
	float MarksmanSize = 160.0f / 220.0f;

	// Arrows fly in an arc: at the middle of the flight an arrow is ArrowArc of the flight's length above where it
	// started, so it goes over the heads between the bow and its mark. It can hit only within ArrowDropDistance of
	// its mark, on the way down (0: anywhere along the flight, a flat shot), and it stops at its mark.
	float ArrowArc = 0.2f;
	float ArrowDropDistance = 140.0f;

	// Seconds a cut stays on screen where a skeleton was taken for the spear.
	float SlashSeconds = 0.35f;

	// Seconds between Bolts while the button is held.
	float BoltInterval = 0.45f;
	// The Bolt leaves his hand this long after the cast starts, at the "Fire" moment of his casting animation (the
	// start of the animation is played faster to get there in that time). 0, or no animation: at once.
	float BoltWindup = 0.2f;

	// What one of the army counts for toward the wizard's bone worth: this share of what it cost to make.
	float ArmyValueShare = 1.0f / 3.0f;

	float SkeletonHealth = 20.0f;
	// Speed while chasing an enemy.
	float SkeletonSpeed = 450.0f;
	float SkeletonDamagePerSecond = 8.0f;

	// The army stays with the wizard. A skeleton idles at a spot inside the roam circle, which moves with the wizard,
	// and walks to a new spot every few seconds.
	float RoamRadius = 440.0f;
	float RoamInnerRadius = 110.0f;
	float RoamPauseMin = 1.5f;
	float RoamPauseMax = 4.5f;
	// Idle speed eases with distance: slow when near its spot, fast when the wizard runs ahead.
	float SkeletonRoamSpeed = 120.0f;
	float SkeletonCatchUpSpeed = 800.0f;
	float RoamEase = 6.0f;
	// A skeleton attacks an enemy only when the enemy is this close to it...
	float ArmyEngageRange = 300.0f;
	// ...and no farther than this from the wizard. A skeleton past this distance drops its fight and comes back.
	float LeashRadius = 500.0f;
	// A skeleton that is already fighting keeps its fight this much farther out (both distances above), so an enemy
	// standing right on a limit does not make it start and stop every frame.
	float LeashSlack = 80.0f;
	// The army stands in groups around the wizard, like walls. The number of groups is the army size divided by
	// GroupTargetSize, kept between GroupsMin and GroupsMax, and skeletons are shared out evenly between them.
	// GroupTargetSize 0 switches groups off (every skeleton roams by itself).
	float GroupTargetSize = 7.0f;
	float GroupsMin = 3.0f;
	float GroupsMax = 6.0f;
	// How far from the wizard the middle of each group is, and how far from that middle a skeleton may stand.
	float GroupDistance = 300.0f;
	float GroupShuffle = 130.0f;

	// A dead enemy leaves bones. The wizard collects the ones within BonePickupRadius of him; bones nobody collects
	// fade after BoneLife seconds. Every BonesPerSkeleton bones he holds assemble one skeleton beside him.
	float BonesPerGrunt = 1.0f;
	// Every kind of enemy leaves its own number of bones, in one pile.
	float BonesPerBrute = 3.0f;

	// Summon the Afterlife: skeletons one cast raises at most. 0 raises as many as the bones held pay for.
	float SummonMax = 0.0f;

	// The shrine. Costs are in bones.
	float SpearUnlockCost = 12.0f;
	float GuardCost = 8.0f;
	// Research, each bought once. Bone armor: health added to every plain skeleton.
	float ArmorCost = 15.0f;
	float ArmorHealth = 15.0f;
	// Bone blades: damage a second added to every plain skeleton.
	float BladesCost = 15.0f;
	float BladesDamage = 4.0f;
	// Dark focus: damage added to Bolt.
	float FocusCost = 15.0f;
	float FocusBoltDamage = 5.0f;
	// Kept so old tuning names still resolve; armor has one level now.
	float ArmorMaxLevel = 1.0f;

	// Time runs at this share of its speed while the shrine's menu is open.
	float ShrineTimeScale = 0.5f;

	// Garrisons: a group at its post charges when the wizard or a skeleton comes this close to one of them.
	float WakeRadius = 900.0f;
	// Losing him. An enemy from a post or a patrol sees the wizard within SightRadius when no wall is between them
	// (water does not hide him). While any of its group sees him the whole group knows where he is. When none does,
	// each goes to where he was last seen (giving up on reaching it after SeekSeconds, and counting as there within
	// SearchArriveRadius of it), looks about for SearchSeconds, then runs back to where it stood when it noticed him
	// (giving up after ReturnSeconds) and takes up its post or patrol again. Seeing him at any point ends all of it.
	// A wall also keeps a group from noticing him in the first place. SightRadius 0: they always know where he is.
	float SightRadius = 1300.0f;
	float SearchSeconds = 3.0f;
	float SeekSeconds = 10.0f;
	float ReturnSeconds = 30.0f;
	float SearchArriveRadius = 200.0f;
	// The commander caps an area.
	float CommanderHealth = 400.0f;
	float CommanderSpeed = 230.0f;
	float CommanderDamagePerSecond = 25.0f;
	float BonesPerCommander = 15.0f;
	// Patrols: a few grunts come from the next manned post this often, in seconds. 0 switches them off.
	float PatrolInterval = 25.0f;
	float PatrolSize = 5.0f;
	// Life at a post: most of a group walks about within this distance of where it was posted, at this speed.
	// PostIdleShare of them stand still. A radius of 0 makes everyone stand.
	float PostWanderRadius = 220.0f;
	float PostWanderSpeed = 70.0f;
	float PostIdleShare = 0.35f;
	// Road patrols: groups that walk the roads until they notice the wizard. A group that is killed is replaced after
	// RoadPatrolRespawn seconds while the commander lives (0: never). RoadPatrolSize 0 switches them off.
	float PatrolWalkSpeed = 150.0f;
	float RoadPatrolSize = 5.0f;
	float RoadPatrolRespawn = 40.0f;
	float BonesPerSkeleton = 2.0f;
	float BonePickupRadius = 130.0f;
	float BoneLife = 30.0f;

	// The bone spear. It costs what one plain skeleton costs (director, 2026-11-14): SpearBoneCost bones when he holds
	// that many (the same as BonesPerSkeleton); when he does not, SpearSkeletonCost plain skeletons with him are cut
	// down for it instead (never another kind) and his bones stay. It flies to where the wizard aims,
	// no farther than SpearRange and no farther than the first wall, and damages every enemy within SpearRadius.
	// SpearDamage kills a grunt outright and takes less than half of a brute.
	float SpearSkeletonCost = 1.0f;
	float SpearBoneCost = 2.0f;
	float SpearDamage = 30.0f;
	float SpearRadius = 260.0f;
	float SpearRange = 900.0f;
	float SpearSpeed = 1600.0f;
	float SpearCooldown = 1.0f;
	// How long the blast stays on screen.
	float BlastSeconds = 0.45f;

	// A unit's health bar is shown for this long after it takes damage.
	float HealthBarSeconds = 2.5f;

	// Enemies head for the wizard. One fights a skeleton only when the skeleton is this close and in front of it.
	float BlockRange = 90.0f;
	// How far in front: 1 is dead ahead, 0 is anywhere in the front half.
	float BlockCone = 0.3f;
	// The most an enemy's approach bends away from the straight line, fading out between the far and near distances.
	float EnemyFlankDegrees = 35.0f;
	float FlankNear = 250.0f;
	float FlankFar = 900.0f;
	// Each enemy's speed differs from the kind's speed by up to this share.
	float EnemySpeedVariation = 0.12f;

	float BoltSpeed = 2400.0f;
	float BoltDamage = 10.0f;
	float BoltRadius = 40.0f;
	float BoltLife = 1.5f;
	// How far a Bolt flies. Shorter than WakeRadius, so the wizard cannot hit a group that has not noticed him. 0: no limit but BoltLife.
	float BoltRange = 850.0f;

	float MeleeRange = 70.0f;
	float TouchRange = 150.0f;
	float MaxStep = 0.1f;

	// Units closer than this push each other apart, so a crowd reads as a crowd and not one stacked shape.
	float SeparationRadius = 70.0f;
	// How far a unit's middle stays from a wall.
	float UnitRadius = 30.0f;

	int32 MaxEnemies = 320;
	int32 MaxSkeletons = 40;
	int32 MaxBones = 200;
};

/** A tuning value a tester can change by name (Wiz.Tune). */
struct FCrowdTuningEntry
{
	const TCHAR* Name;
	float FCrowdTuning::* Member;
};

TConstArrayView<FCrowdTuningEntry> GetCrowdTuningEntries();

/** The named value in Tuning, in any letter case, or null when there is no such name. */
float* FindCrowdTuningValue(FCrowdTuning& Tuning, const FString& Name);

/** A moment in a step that is shown or heard. */
enum class ECrowdEvent : uint8
{
	// One of the army landed a blow on an enemy (Kind: the enemy's).
	ArmyBlow,
	// A Bolt met an enemy (Kind: the enemy's), or a wall or building.
	BoltHit,
	BoltWall,
	// A unit died (Kind, Team, Facing: its own; From: where the last blow came from; Id: its own).
	Death,
	// A brute's slam landed on nobody.
	SlamMiss,
	// An archer or a Marksman loosed an arrow (Kind: the shooter's).
	ArrowLoosed,
	// An assassin's swing at the wizard: it reached him, or he was gone.
	AssassinBlow,
	AssassinMiss,
	// The wizard picked up a pile of bones (Position: where it lay).
	BonePicked,
	// The commander's spear left his hand (Position, Kind, Team: his; Id: the spear's own number).
	SpearThrown,
};

struct FCrowdEvent
{
	ECrowdEvent Type = ECrowdEvent::ArmyBlow;
	FVector2D Position = FVector2D::ZeroVector;
	ECrowdKind Kind = ECrowdKind::Grunt;
	ECrowdTeam Team = ECrowdTeam::Enemy;
	FVector2D Facing = FVector2D::ZeroVector;
	FVector2D From = FVector2D::ZeroVector;
	int32 Id = 0;
};

struct FCrowdStepResult
{
	int32 EnemiesKilled = 0;
	int32 SkeletonsRaised = 0;
	int32 SkeletonsLost = 0;
	int32 BonesCollected = 0;
	int32 SpearsExploded = 0;
	int32 CommandersKilled = 0;
	// Groups of enemies that left their posts in this step.
	int32 SquadsWoken = 0;
	int32 ArrowsFired = 0;
	int32 ArrowsHitWizard = 0;
	// Brute slams that landed in this step, and how many of them caught the wizard.
	int32 SlamsLanded = 0;
	int32 SlamsHitWizard = 0;
	// Brutes' sweeps that landed this step, and how many of them caught the wizard.
	int32 SweepsLanded = 0;
	int32 SweepsHitWizard = 0;
	// Sent skeletons that rejoined the wizard in this step.
	int32 Rejoined = 0;
	// The commander's slashes that landed in this step, and how many of them caught the wizard.
	int32 SlashesLanded = 0;
	int32 SlashesHitWizard = 0;
	// The commander's spears thrown in this step, and how many hit the wizard.
	int32 JavelinsThrown = 0;
	int32 JavelinsHitWizard = 0;
	// Units of the army a commander's spear went through this step.
	int32 JavelinsHitArmy = 0;
	// Arrows loosed by Marksmen in this step.
	int32 ArmyArrowsFired = 0;
};
