#include "Game/WizardHUD.h"
#include "Sim/CrowdSim.h"
#include "Sim/OverwhelmMeter.h"
#include "Tests/CrowdTestHelpers.h"
#include "Tests/WizTestFlags.h"

#if WITH_DEV_AUTOMATION_TESTS

// Direction of 2026-11-14 (11:00 PST):
// - The commander slashes in front of him, quicker than a brute's slam and with the same warning; throws a spear at the
//   wizard that passes through the army, slows him and fills Overwhelm; always goes for the wizard; counts most on the
//   meter; and while he is near, enemies fill it faster.
// - Overwhelm does not start to go down until 2 seconds without any build-up.
// - The wizard's spear costs what one plain skeleton costs.
// - The Marksman: shoots archers first and kills them in one, else the enemy with the most health; a little more to a
//   brute than a skeleton's blow. Arrows (the enemy's too) fly in an arc over what stands between, and hit where they land.

namespace NewRulesTest
{
	struct FTotals
	{
		int32 SlamsLanded = 0;
		int32 SlashesLanded = 0;
		int32 SlashesHitWizard = 0;
		int32 JavelinsThrown = 0;
		int32 JavelinsHitWizard = 0;
		int32 ArmyArrowsFired = 0;
		int32 EnemiesKilled = 0;
	};

	FTotals Run(FCrowdSim& Sim, float Seconds, FVector2D Wizard = CrowdTest::Origin)
	{
		FTotals Total;
		for (float Time = 0.0f; Time < Seconds; Time += 0.05f)
		{
			const FCrowdStepResult Result = Sim.Step(0.05f, Wizard);
			Total.SlamsLanded += Result.SlamsLanded;
			Total.SlashesLanded += Result.SlashesLanded;
			Total.SlashesHitWizard += Result.SlashesHitWizard;
			Total.JavelinsThrown += Result.JavelinsThrown;
			Total.JavelinsHitWizard += Result.JavelinsHitWizard;
			Total.ArmyArrowsFired += Result.ArmyArrowsFired;
			Total.EnemiesKilled += Result.EnemiesKilled;
		}
		return Total;
	}

	const FCrowdUnit* FirstOf(const FCrowdSim& Sim, ECrowdKind Kind)
	{
		for (const FCrowdUnit& Unit : Sim.GetUnits())
		{
			if (Unit.Kind == Kind)
			{
				return &Unit;
			}
		}
		return nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOverwhelmHolds, "WebTesting.Overwhelm.HoldsTwoSecondsBeforeItGoesDown", WIZ_TEST_FLAGS)
bool FOverwhelmHolds::RunTest(const FString& Parameters)
{
	const TArray<FVector2D> None;
	TArray<FVector2D> Five;
	Five.Init(FVector2D(50.0, 0.0), 5);

	FOverwhelmMeter Meter;
	TestEqual(TEXT("it waits two seconds"), Meter.Tuning.DrainDelay, 2.0f);
	Meter.Step(1.0f, Five);
	const float Filled = Meter.GetValue();
	Meter.Step(1.0f, None);
	Meter.Step(0.9f, None);
	TestEqual(TEXT("1.9 seconds clear: it has not gone down"), Meter.GetValue(), Filled);
	TestTrue(TEXT("it says it is holding"), Meter.IsHolding());
	Meter.Step(1.1f, None);
	TestTrue(TEXT("a second past the wait it has gone down by one second's worth"),
		FMath::IsNearlyEqual(Meter.GetValue(), Filled - Meter.Tuning.DrainPerSecond, 0.001f));

	// A hit starts the wait again.
	Meter.Add(0.1f);
	const float Hit = Meter.GetValue();
	Meter.Step(1.5f, None);
	TestEqual(TEXT("after a hit it holds again"), Meter.GetValue(), Hit);

	// With the commander near, the same enemies fill it faster.
	FOverwhelmMeter Plain;
	FOverwhelmMeter Pressed;
	Plain.Step(1.0f, Five);
	Pressed.Step(1.0f, Five, 1.5f);
	TestTrue(TEXT("half as fast again"), FMath::IsNearlyEqual(Pressed.GetValue(), Plain.GetValue() * 1.5f, 0.001f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpearCostsOne, "WebTesting.Spear.CostsWhatOneSkeletonCosts", WIZ_TEST_FLAGS)
bool FSpearCostsOne::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	CrowdTest::FreezeArmy(Sim);
	Sim.SetSpearUnlocked(true);
	TestEqual(TEXT("in bones: what a skeleton is summoned for"), Sim.Tuning.SpearBoneCost, Sim.Tuning.BonesPerSkeleton);
	TestEqual(TEXT("in skeletons: one"), Sim.Tuning.SpearSkeletonCost, 1.0f);
	Sim.AddSkeleton(FVector2D(-100.0, 0.0));
	Sim.AddSkeleton(FVector2D(-140.0, 0.0));
	TestTrue(TEXT("one skeleton pays for a spear"), Sim.ThrowSpear(CrowdTest::Origin, FVector2D(600.0, 0.0)));
	TestEqual(TEXT("one was taken"), Sim.CountTeam(ECrowdTeam::Army), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommanderSlashes, "WebTesting.Commander.SlashesInFrontQuickerThanASlam", WIZ_TEST_FLAGS)
bool FCommanderSlashes::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	CrowdTest::MakeEnemiesPlain(Sim);
	Sim.Tuning.CommanderSpeed = 0.0f;
	TestTrue(TEXT("the warning is shorter than a brute's"),
		Sim.Tuning.CommanderSlashGrow + Sim.Tuning.CommanderSlashHold < Sim.Tuning.BruteSlamGrow + Sim.Tuning.BruteSlamHold);
	TestTrue(TEXT("a slash that catches him fills more of the meter than a slam"), Sim.Tuning.CommanderSlashOverwhelm > Sim.Tuning.BruteSlamOverwhelm);
	// Close enough for the slash and too close for the spear.
	Sim.AddEnemy(ECrowdKind::Commander, FVector2D(200.0, 0.0));

	// The same warning as the slam: a circle that grows, seen before it lands.
	bool bWarned = false;
	NewRulesTest::FTotals Total;
	for (int32 Step = 0; Step < 60 && Total.SlashesLanded == 0; ++Step)
	{
		const FCrowdStepResult Result = Sim.Step(0.05f, CrowdTest::Origin);
		Total.SlashesLanded += Result.SlashesLanded;
		Total.SlashesHitWizard += Result.SlashesHitWizard;
		Total.SlamsLanded += Result.SlamsLanded;
		const FCrowdUnit* Commander = NewRulesTest::FirstOf(Sim, ECrowdKind::Commander);
		if (Commander && Sim.IsSlamming(*Commander))
		{
			bWarned = true;
			TestEqual(TEXT("the circle is the slash's size"), Sim.GetSlamRadius(*Commander), Sim.Tuning.CommanderSlashRadius);
			TestTrue(TEXT("and it lies in front of him, toward the wizard"), Commander->SlamCenter.X < Commander->Position.X);
		}
	}
	TestTrue(TEXT("the warning was shown before it landed"), bWarned);
	TestEqual(TEXT("the slash landed"), Total.SlashesLanded, 1);
	TestEqual(TEXT("and caught the wizard standing in it"), Total.SlashesHitWizard, 1);
	TestEqual(TEXT("it is not counted as a brute's slam"), Total.SlamsLanded, 0);

	// A blink gets him out of it.
	FCrowdSim Blinked;
	CrowdTest::MakeEnemiesPlain(Blinked);
	Blinked.Tuning.CommanderSpeed = 0.0f;
	Blinked.AddEnemy(ECrowdKind::Commander, FVector2D(200.0, 0.0));
	int32 Hits = 0;
	bool bStarted = false;
	for (int32 Step = 0; Step < 80; ++Step)
	{
		const FCrowdUnit* Commander = NewRulesTest::FirstOf(Blinked, ECrowdKind::Commander);
		bStarted = bStarted || (Commander && Blinked.IsSlamming(*Commander));
		// Once the warning shows, he blinks.
		Blinked.bWizardInvulnerable = bStarted;
		Hits += Blinked.Step(0.05f, CrowdTest::Origin).SlashesHitWizard;
	}
	TestEqual(TEXT("a blink through the slash is not hit"), Hits, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommanderThrows, "WebTesting.Commander.HisSpearPassesTheArmyAndHitsTheWizard", WIZ_TEST_FLAGS)
bool FCommanderThrows::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	CrowdTest::MakeEnemiesPlain(Sim);
	CrowdTest::FreezeArmy(Sim);
	Sim.Tuning.CommanderSpeed = 0.0f;
	Sim.AddEnemy(ECrowdKind::Commander, FVector2D(800.0, 0.0));
	// One of the army stands right on the line between them.
	Sim.AddSkeleton(FVector2D(400.0, 0.0));
	const NewRulesTest::FTotals Total = NewRulesTest::Run(Sim, 3.0f);
	TestTrue(TEXT("he threw"), Total.JavelinsThrown >= 1);
	TestTrue(TEXT("it reached the wizard through the skeleton"), Total.JavelinsHitWizard >= 1);
	const FCrowdUnit* Skeleton = NewRulesTest::FirstOf(Sim, ECrowdKind::Skeleton);
	// Changed 2026-11-15 02:45 PST (director: the spear hits all targets in its path): it was "the skeleton was not hurt by it".
	TestTrue(TEXT("the skeleton in its way was hit as it passed"), !Skeleton || Skeleton->Health < Skeleton->MaxHealth);
	TestTrue(TEXT("it slows him"), Sim.Tuning.CommanderSpearSlow < 1.0f && Sim.Tuning.CommanderSpearSlowSeconds > 0.0f);
	TestTrue(TEXT("and fills the meter"), Sim.Tuning.CommanderSpearOverwhelm > 0.0f);

	// A wall stops it, and he does not throw at a wall.
	FCrowdSim Walled;
	CrowdTest::MakeEnemiesPlain(Walled);
	Walled.Tuning.CommanderSpeed = 0.0f;
	Walled.SetObstacles({ CrowdTest::Wall(400.0, 0.0, 30.0, 600.0) });
	Walled.AddEnemy(ECrowdKind::Commander, FVector2D(800.0, 0.0));
	TestEqual(TEXT("no spear at a wall"), NewRulesTest::Run(Walled, 3.0f).JavelinsThrown, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommanderThrowWarns, "WebTesting.Commander.HisSpearIsWarnedOfAndCanBeSteppedOutOf", WIZ_TEST_FLAGS)
bool FCommanderThrowWarns::RunTest(const FString& Parameters)
{
	// Direction of 2026-11-14 (11:40 PST): "Have a warning for the spear throw for the commander."
	FCrowdSim Sim;
	CrowdTest::MakeEnemiesPlain(Sim);
	Sim.Tuning.CommanderSpeed = 0.0f;
	Sim.AddEnemy(ECrowdKind::Commander, FVector2D(800.0, 0.0));
	FVector2D Wizard = CrowdTest::Origin;
	int32 WarnedSteps = 0;
	int32 LockedSteps = 0;
	int32 Thrown = 0;
	int32 Hits = 0;
	float LastShare = 0.0f;
	bool bGrew = true;
	bool bFollowed = false;
	FVector2D LockedAim = FVector2D::ZeroVector;
	for (int32 Step = 0; Step < 60 && Thrown == 0; ++Step)
	{
		const FCrowdStepResult Result = Sim.Step(0.05f, Wizard);
		Thrown += Result.JavelinsThrown;
		const FCrowdUnit* Commander = NewRulesTest::FirstOf(Sim, ECrowdKind::Commander);
		if (!Commander || !Sim.IsWindingThrow(*Commander))
		{
			continue;
		}
		++WarnedSteps;
		bGrew = bGrew && Sim.GetThrowShare(*Commander) >= LastShare;
		LastShare = Sim.GetThrowShare(*Commander);
		if (!Sim.IsThrowLocked(*Commander))
		{
			// While it still follows, he walks to one side: the lane follows him.
			Wizard = FVector2D(0.0, 150.0);
			bFollowed = bFollowed || Commander->ThrowAim.Y > 0.05;
		}
		else
		{
			++LockedSteps;
			if (LockedAim.IsNearlyZero())
			{
				LockedAim = Commander->ThrowAim;
			}
			TestTrue(TEXT("once it holds, it no longer follows him"), Commander->ThrowAim.Equals(LockedAim, 0.001));
			// He steps well out of the lane.
			Wizard = FVector2D(0.0, -400.0);
		}
	}
	TestTrue(TEXT("the warning shows for most of the time he draws back"), WarnedSteps >= FMath::FloorToInt32(Sim.Tuning.CommanderSpearWindup / 0.05f) - 2);
	TestTrue(TEXT("it grows"), bGrew && LastShare > 0.8f);
	TestTrue(TEXT("it follows the wizard at first"), bFollowed);
	TestTrue(TEXT("then it holds for a moment before the throw"), LockedSteps >= 2);
	TestEqual(TEXT("he throws"), Thrown, 1);
	if (TestEqual(TEXT("one spear flies"), Sim.GetJavelins().Num(), 1))
	{
		TestTrue(TEXT("the way the warning showed"), Sim.GetJavelins()[0].Velocity.GetSafeNormal().Equals(LockedAim, 0.001));
	}
	for (int32 Step = 0; Step < 30; ++Step)
	{
		Hits += Sim.Step(0.05f, Wizard).JavelinsHitWizard;
	}
	TestEqual(TEXT("stepping out of the lane once it holds dodges it"), Hits, 0);
	TestTrue(TEXT("the lane reaches as far as the spear flies"), FMath::IsNearlyEqual(Sim.GetThrowReach(*NewRulesTest::FirstOf(Sim, ECrowdKind::Commander)), 0.0, 1.0)
		|| Sim.Tuning.CommanderSpearRange > 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHudSideWeights, "WebTesting.Hud.EdgesCloseInFromEverySideAtOnce", WIZ_TEST_FLAGS)
bool FHudSideWeights::RunTest(const FString& Parameters)
{
	// In the world +X is up the screen and +Y is to the right, with the camera's yaw at 0.
	TArray<FVector2D> Offsets;
	Offsets.Init(FVector2D(0.0, 100.0), 6);
	Offsets.Add(FVector2D(100.0, 0.0));
	Offsets.Add(FVector2D(100.0, 10.0));
	Offsets.Add(FVector2D(90.0, -10.0));
	FWizardEdgeWeights Sides = AWizardHUD::SideWeights(Offsets, 0.0f);
	TestEqual(TEXT("the busiest side is full"), Sides.Right, 1.0f);
	TestEqual(TEXT("three of six at the top: half"), Sides.Top, 0.5f);
	TestEqual(TEXT("nobody on the left"), Sides.Left, 0.0f);
	TestEqual(TEXT("nobody at the bottom"), Sides.Bottom, 0.0f);

	TArray<FVector2D> Around = { FVector2D(100.0, 0.0), FVector2D(-100.0, 0.0), FVector2D(0.0, 100.0), FVector2D(0.0, -100.0) };
	Sides = AWizardHUD::SideWeights(Around, 0.0f);
	TestTrue(TEXT("surrounded: every side is full"), Sides.Left == 1.0f && Sides.Right == 1.0f && Sides.Top == 1.0f && Sides.Bottom == 1.0f);

	Sides = AWizardHUD::SideWeights(TArray<FVector2D>(), 0.0f);
	TestTrue(TEXT("nobody on him: no edge"), Sides.Left + Sides.Right + Sides.Top + Sides.Bottom == 0.0f);

	// Turned a quarter, what was up the screen is now at its side.
	Sides = AWizardHUD::SideWeights({ FVector2D(0.0, 100.0) }, 90.0f);
	TestEqual(TEXT("the camera's turn is taken into account"), Sides.Top, 1.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommanderGoesForWizard, "WebTesting.Commander.GoesForTheWizardAndCountsMost", WIZ_TEST_FLAGS)
bool FCommanderGoesForWizard::RunTest(const FString& Parameters)
{
	// Far from the wizard, a grunt turns on one of the army near it. The commander does not: he goes for the wizard.
	FCrowdSim Sim;
	CrowdTest::MakeEnemiesPlain(Sim);
	CrowdTest::FreezeArmy(Sim);
	Sim.Tuning.CommanderSpearInterval = 0.0f;
	Sim.AddEnemy(ECrowdKind::Commander, FVector2D(2000.0, 0.0));
	Sim.AddSkeleton(FVector2D(2000.0, 280.0));
	NewRulesTest::Run(Sim, 2.0f);
	const FCrowdUnit* Commander = NewRulesTest::FirstOf(Sim, ECrowdKind::Commander);
	if (!TestNotNull(TEXT("the commander"), Commander))
	{
		return false;
	}
	TestTrue(TEXT("he has come toward the wizard"), Commander->Position.X < 1750.0);
	TestTrue(TEXT("and not turned aside to the skeleton"), FMath::Abs(Commander->Position.Y) < 80.0);

	// On the wizard he counts for more than anything else, and while he is near, the others fill the meter faster.
	FCrowdSim Close;
	Close.Tuning.CommanderSpeed = 0.0f;
	Close.AddEnemy(ECrowdKind::Commander, FVector2D(100.0, 0.0));
	TArray<FVector2D> Touching;
	Close.GetEnemiesTouching(CrowdTest::Origin, Touching);
	TestTrue(TEXT("he counts for more than an assassin"), Touching.Num() > FMath::RoundToInt32(Close.Tuning.AssassinTouchWeight));
	TestTrue(TEXT("he is near"), Close.IsCommanderNear(CrowdTest::Origin));
	TestTrue(TEXT("and that makes the meter fill faster"), Close.Tuning.CommanderAuraFill > 1.0f);
	TestFalse(TEXT("far away he is not"), Close.IsCommanderNear(FVector2D(-5000.0, 0.0)));

	FCrowdSim Posted;
	Posted.AddGarrisonEnemy(ECrowdKind::Commander, FVector2D(100.0, 0.0), 1);
	TestFalse(TEXT("a commander still at his post presses nobody"), Posted.IsCommanderNear(CrowdTest::Origin));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarksmanShoots, "WebTesting.Marksman.ArchersFirstThenTheStrongest", WIZ_TEST_FLAGS)
bool FMarksmanShoots::RunTest(const FString& Parameters)
{
	const FVector2D Wizard(-200.0, 0.0);
	FCrowdSim Sim;
	CrowdTest::MakeEnemiesPlain(Sim);
	CrowdTest::FreezeArmy(Sim);
	Sim.Tuning.BruteSpeed = 0.0f;
	Sim.Tuning.GruntSpeed = 0.0f;
	Sim.Tuning.ArcherSpeed = 0.0f;
	// The archer shoots back; this is about the marksman.
	Sim.Tuning.ArcherInterval = 1000.0f;
	Sim.Tuning.ArrowDamage = 0.0f;
	Sim.Tuning.GruntHealth = 1000.0f;
	TestTrue(TEXT("it is bought at the shrine"), Sim.GetShopCost(ECrowdShopItem::Marksman) > 0);
	TestTrue(TEXT("frailer than a skeleton"), Sim.GetStats(ECrowdKind::Marksman).Health < Sim.GetStats(ECrowdKind::Skeleton).Health);
	// Against the numbers as they ship: this test's army is frozen and does no damage.
	const FCrowdTuning Shipped;
	TestTrue(TEXT("a little more to a brute than a skeleton's blow"),
		Shipped.MarksmanBruteDamage > Shipped.SkeletonDamagePerSecond * Shipped.MeleeInterval
		&& Shipped.MarksmanBruteDamage <= Shipped.SkeletonDamagePerSecond * Shipped.MeleeInterval * 1.5f);

	Sim.AddSkeleton(FVector2D(0.0, 0.0), ECrowdKind::Marksman);
	// A grunt stands right in front of the archer, on the line of the shot, and a brute to one side.
	Sim.AddEnemy(ECrowdKind::Grunt, FVector2D(350.0, 0.0));
	Sim.AddEnemy(ECrowdKind::Archer, FVector2D(700.0, 0.0));
	Sim.AddEnemy(ECrowdKind::Brute, FVector2D(500.0, 400.0));

	const NewRulesTest::FTotals First = NewRulesTest::Run(Sim, 2.0f, Wizard);
	TestTrue(TEXT("it shot"), First.ArmyArrowsFired >= 1);
	TestNull(TEXT("the archer went first, and one arrow killed it"), NewRulesTest::FirstOf(Sim, ECrowdKind::Archer));
	const FCrowdUnit* Grunt = NewRulesTest::FirstOf(Sim, ECrowdKind::Grunt);
	const FCrowdUnit* Brute = NewRulesTest::FirstOf(Sim, ECrowdKind::Brute);
	TestTrue(TEXT("the arrow flew over the grunt in front"), Grunt && Grunt->Health == Grunt->MaxHealth);
	const float BruteBefore = Brute ? Brute->Health : 0.0f;

	// No archers left: the one with the most health. The grunt here has more than the brute.
	NewRulesTest::Run(Sim, 2.5f, Wizard);
	Grunt = NewRulesTest::FirstOf(Sim, ECrowdKind::Grunt);
	Brute = NewRulesTest::FirstOf(Sim, ECrowdKind::Brute);
	TestTrue(TEXT("then it shoots the one with the most health"), Grunt && Grunt->Health < Grunt->MaxHealth);
	TestTrue(TEXT("and leaves the weaker one"), Brute && Brute->Health == BruteBefore);

	// It does not walk up to fight.
	const FCrowdUnit* Marksman = NewRulesTest::FirstOf(Sim, ECrowdKind::Marksman);
	TestTrue(TEXT("it stayed where it stood"), Marksman && Marksman->Position.Size() < 50.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArrowArcs, "WebTesting.Archer.ArrowsArcAndHitWhereTheyLand", WIZ_TEST_FLAGS)
bool FArrowArcs::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	CrowdTest::MakeEnemiesPlain(Sim);
	Sim.Tuning.ArcherSpeed = 0.0f;
	Sim.Tuning.ArcherSpreadDegrees = 0.0f;
	Sim.AddEnemy(ECrowdKind::Archer, FVector2D(700.0, 0.0));
	float Highest = 0.0f;
	float AtStart = -1.0f;
	int32 Hits = 0;
	for (int32 Step = 0; Step < 40 && Hits == 0; ++Step)
	{
		Hits += Sim.Step(0.05f, CrowdTest::Origin).ArrowsHitWizard;
		for (const FCrowdArrow& Arrow : Sim.GetArrows())
		{
			if (AtStart < 0.0f)
			{
				AtStart = Sim.GetArrowHeight(Arrow);
				TestTrue(TEXT("it is aimed at where the wizard stands"), FVector2D::Distance(Arrow.Target, CrowdTest::Origin) < 30.0);
			}
			Highest = FMath::Max(Highest, Sim.GetArrowHeight(Arrow));
		}
	}
	TestTrue(TEXT("it leaves the bow low"), AtStart >= 0.0f && AtStart < 60.0f);
	TestTrue(TEXT("it rises over the heads between"), Highest > 80.0f);
	TestEqual(TEXT("and comes down on the wizard"), Hits, 1);

	// One of the army standing between the archer and its mark is flown over: the arrow lands on the mark.
	FCrowdSim Over;
	CrowdTest::MakeEnemiesPlain(Over);
	CrowdTest::FreezeArmy(Over);
	Over.Tuning.ArcherSpeed = 0.0f;
	Over.Tuning.ArcherSpreadDegrees = 0.0f;
	Over.Tuning.ArcherRange = 750.0f;
	Over.Tuning.ArcherFleeRange = 100.0f;
	Over.AddEnemy(ECrowdKind::Archer, FVector2D(700.0, 0.0));
	Over.AddSkeleton(FVector2D(350.0, 0.0));
	// The skeleton is nearer, so it is the mark; the wizard far behind it is not hit by that arrow.
	int32 WizardHits = 0;
	for (int32 Step = 0; Step < 30; ++Step)
	{
		WizardHits += Over.Step(0.05f, CrowdTest::Origin).ArrowsHitWizard;
	}
	const FCrowdUnit* Skeleton = NewRulesTest::FirstOf(Over, ECrowdKind::Skeleton);
	TestTrue(TEXT("the mark is hit"), !Skeleton || Skeleton->Health < Skeleton->MaxHealth);
	TestEqual(TEXT("an arrow that lands on its mark goes no farther"), WizardHits, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarksmanBought, "WebTesting.Shop.AMarksmanIsBoughtLikeAGuard", WIZ_TEST_FLAGS)
bool FMarksmanBought::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	const int32 Cost = Sim.GetShopCost(ECrowdShopItem::Marksman);
	Sim.DropBone(CrowdTest::Origin, Cost * 2 + 1);
	Sim.Step(0.05f, CrowdTest::Origin);
	TestEqual(TEXT("the bones pay for two"), Sim.GetShopMax(ECrowdShopItem::Marksman), 2);
	TestEqual(TEXT("asking for five buys the two"), Sim.Buy(ECrowdShopItem::Marksman, 5, CrowdTest::Origin), 2);
	int32 Marksmen = 0;
	for (const FCrowdUnit& Unit : Sim.GetUnits())
	{
		Marksmen += Unit.Kind == ECrowdKind::Marksman && Unit.Team == ECrowdTeam::Army ? 1 : 0;
	}
	TestEqual(TEXT("two marksmen stand with him"), Marksmen, 2);
	TestEqual(TEXT("one bone is left"), Sim.GetBonesHeld(), 1);
	TestFalse(TEXT("it is a unit, not research"), Sim.IsResearched(ECrowdShopItem::Marksman));
	return true;
}

#endif
