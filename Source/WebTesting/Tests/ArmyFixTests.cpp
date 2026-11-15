#include "Sim/BlinkState.h"
#include "Sim/CrowdSim.h"
#include "Tests/CrowdTestHelpers.h"
#include "Tests/WizTestFlags.h"

#if WITH_DEV_AUTOMATION_TESTS

// Direction of 2026-11-13 (15:10 PST): Blink steers more at its start and hides the wizard until it slows; a fallen
// ally leaves a third of its cost; archers aim ahead, not perfectly; the army does not shove its own; bones in view do
// not fade; the army stands still beside a wall; and the army is weaker away from the wizard.

namespace FixTest
{
	const TArray<FCrowdObstacle> NoWalls;

	void Run(FCrowdSim& Sim, float Seconds, FVector2D Wizard)
	{
		for (float Time = 0.0f; Time < Seconds; Time += 0.05f)
		{
			Sim.Step(0.05f, Wizard);
		}
	}

	int32 BonesOnGround(const FCrowdSim& Sim)
	{
		int32 Value = 0;
		for (const FCrowdBone& Bone : Sim.GetBones())
		{
			Value += Bone.bGuard ? 0 : Bone.Value;
		}
		return Value;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBlinkEarlySteer, "WebTesting.Blink.SteersMostAtTheStart", WIZ_TEST_FLAGS)
bool FBlinkEarlySteer::RunTest(const FString& Parameters)
{
	const FVector2D Up(0.0, 1.0);
	FBlinkState Blink;
	Blink.SteerEarly = 0.6f;
	Blink.Start(FVector2D::ZeroVector, FVector2D(1.0, 0.0), 700.0f);
	// The sideways reach is 0.5 of 700. An even pace would give a fifth of it, 70, in the first fifth of the time.
	Blink.Step(0.1f, 0.5f, 3.0f, Up, 0.5f, FixTest::NoWalls, 38.0f);
	TestTrue(TEXT("the first fifth of the time gives over a quarter of the sideways reach"), Blink.GetPosition().Y > 0.27 * 350.0);
	Blink.Step(0.4f, 0.5f, 3.0f, Up, 0.5f, FixTest::NoWalls, 38.0f);
	TestEqual(TEXT("held throughout, the whole reach"), Blink.GetPosition().Y, 350.0, 1.0);
	TestEqual(TEXT("and the full length"), Blink.GetPosition().X, 700.0, 0.5);

	const FCrowdTuning Tuning;
	TestTrue(TEXT("most of the steering follows the burst"), Tuning.BlinkSteerEarly >= 0.5f && Tuning.BlinkSteerEarly <= 1.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBlinkHidden, "WebTesting.Blink.HeVanishesUntilItSlows", WIZ_TEST_FLAGS)
bool FBlinkHidden::RunTest(const FString& Parameters)
{
	const FVector2D None = FVector2D::ZeroVector;
	FBlinkState Blink;
	TestFalse(TEXT("seen before a blink"), Blink.IsHidden(0.7f));
	Blink.Start(FVector2D::ZeroVector, FVector2D(1.0, 0.0), 700.0f);
	TestTrue(TEXT("gone the moment it starts"), Blink.IsHidden(0.7f));
	Blink.Step(0.3f, 0.5f, 3.0f, None, 0.5f, FixTest::NoWalls, 38.0f);
	TestTrue(TEXT("still gone three fifths of the way through its time"), Blink.IsHidden(0.7f));
	Blink.Step(0.1f, 0.5f, 3.0f, None, 0.5f, FixTest::NoWalls, 38.0f);
	TestFalse(TEXT("back as it slows, before it ends"), Blink.IsHidden(0.7f));
	TestTrue(TEXT("and it has not ended yet"), Blink.IsActive());
	Blink.Step(0.2f, 0.5f, 3.0f, None, 0.5f, FixTest::NoWalls, 38.0f);
	TestFalse(TEXT("seen after it"), Blink.IsHidden(0.7f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFallenBones, "WebTesting.Bones.AFallenAllyLeavesAThirdOfItsCost", WIZ_TEST_FLAGS)
bool FFallenBones::RunTest(const FString& Parameters)
{
	FCrowdSim Costs;
	TestEqual(TEXT("a skeleton cost 3 and leaves 1"), Costs.GetFallenBoneValue(ECrowdKind::Skeleton), 1);
	TestEqual(TEXT("a Guard cost 8 and leaves 3 (2.67 rounded)"), Costs.GetFallenBoneValue(ECrowdKind::Guard), 3);
	TestEqual(TEXT("an enemy leaves its own bones, not these"), Costs.GetFallenBoneValue(ECrowdKind::Grunt), 0);

	for (const ECrowdKind Kind : { ECrowdKind::Skeleton, ECrowdKind::Guard })
	{
		FCrowdSim Sim;
		CrowdTest::MakeEnemiesPlain(Sim);
		CrowdTest::FreezeArmy(Sim);
		Sim.Tuning.GuardSpeed = 0.0f;
		Sim.Tuning.GuardDamagePerSecond = 0.0f;
		const FVector2D Place(3000.0, 0.0);
		Sim.AddSkeleton(Place, Kind);
		Sim.AddEnemy(ECrowdKind::Brute, Place + FVector2D(60.0, 0.0));
		FixTest::Run(Sim, 12.0f, CrowdTest::Origin);
		TestEqual(TEXT("it fell"), Sim.CountTeam(ECrowdTeam::Army), 0);
		TestEqual(TEXT("and left its bones"), FixTest::BonesOnGround(Sim), Kind == ECrowdKind::Guard ? 3 : 1);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArcherLeads, "WebTesting.Archer.AimsAheadOfAMovingTarget", WIZ_TEST_FLAGS)
bool FArcherLeads::RunTest(const FString& Parameters)
{
	// The wizard walks back and forth across the archer's front at 300 units a second. An arrow takes over half a
	// second to reach him, so one aimed at where he stood lands well behind him.
	FCrowdSim Sim;
	CrowdTest::MakeEnemiesPlain(Sim);
	Sim.Tuning.ArcherSpeed = 0.0f;
	Sim.Tuning.ArcherFleeRange = 0.0f;
	Sim.Tuning.ArcherSpreadDegrees = 0.0f;
	Sim.AddEnemy(ECrowdKind::Archer, FVector2D(650.0, 0.0));
	int32 Fired = 0;
	int32 Hits = 0;
	FVector2D Wizard(0.0, -350.0);
	double Way = 1.0;
	for (int32 Index = 0; Index < 400; ++Index)
	{
		Wizard.Y += Way * 300.0 * 0.05;
		if (FMath::Abs(Wizard.Y) >= 350.0)
		{
			Way = -Way;
		}
		const FCrowdStepResult Result = Sim.Step(0.05f, Wizard);
		Fired += Result.ArrowsFired;
		Hits += Result.ArrowsHitWizard;
	}
	TestTrue(TEXT("it shot a dozen arrows"), Fired >= 10);
	// Those in the air when he turns around miss: that is the dodge.
	TestTrue(TEXT("at least half of them hit him as he walks"), Hits * 2 >= Fired);
	TestTrue(TEXT("and turning around dodges some"), Hits < Fired);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArcherSpread, "WebTesting.Archer.IsNotAPerfectShot", WIZ_TEST_FLAGS)
bool FArcherSpread::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	CrowdTest::MakeEnemiesPlain(Sim);
	Sim.Tuning.ArcherSpeed = 0.0f;
	Sim.Tuning.ArcherFleeRange = 0.0f;
	Sim.Tuning.ArcherInterval = 0.05f;
	const FVector2D From(700.0, 0.0);
	Sim.AddEnemy(ECrowdKind::Archer, From);
	double Widest = 0.0;
	double Least = 1000.0;
	double Most = -1000.0;
	int32 Seen = 0;
	for (int32 Index = 0; Index < 60; ++Index)
	{
		Sim.Step(0.05f, CrowdTest::Origin);
		for (const FCrowdArrow& Arrow : Sim.GetArrows())
		{
			// Degrees off the straight line to the wizard, who stands still.
			const double Off = FMath::RadiansToDegrees(FMath::Atan2(Arrow.Velocity.Y, -Arrow.Velocity.X));
			Widest = FMath::Max(Widest, FMath::Abs(Off));
			Least = FMath::Min(Least, Off);
			Most = FMath::Max(Most, Off);
			++Seen;
		}
	}
	TestTrue(TEXT("arrows were seen"), Seen > 20);
	TestTrue(TEXT("they are not all the same shot: they spread over more than a degree"), Most - Least > 1.0);
	TestTrue(TEXT("to both sides"), Least < 0.0 && Most > 0.0);
	TestTrue(TEXT("and none is off by more than the spread"), Widest <= Sim.Tuning.ArcherSpreadDegrees + 0.01);
	TestTrue(TEXT("the spread is small: under 8 degrees"), Sim.Tuning.ArcherSpreadDegrees > 0.5f && Sim.Tuning.ArcherSpreadDegrees < 8.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArmyNoShove, "WebTesting.Army.DoesNotShoveItsOwn", WIZ_TEST_FLAGS)
bool FArmyNoShove::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	// Sent units stand exactly on their place and never stroll, so any move is a shove.
	Sim.Tuning.CommandScatter = 0.0f;
	Sim.Tuning.CommandSpacing = 0.0f;
	Sim.Tuning.RoamPauseMin = 100000.0f;
	Sim.Tuning.RoamPauseMax = 100000.0f;
	const FVector2D Near(600.0, 0.0);
	const FVector2D Far(1200.0, 0.0);
	Sim.AddSkeleton(Near);
	Sim.Command(Near, 1, ECrowdCommandKind::Any);
	FixTest::Run(Sim, 1.0f, CrowdTest::Origin);
	const FVector2D Held = Sim.GetUnits()[0].Position;

	// Three more are sent to a place straight past the first, so their way leads through where it stands.
	for (int32 Index = 0; Index < 3; ++Index)
	{
		Sim.AddSkeleton(FVector2D(-60.0 * Index, 0.0));
	}
	Sim.Command(Far, 3, ECrowdCommandKind::Any);
	double Shoved = 0.0;
	for (int32 Index = 0; Index < 120; ++Index)
	{
		Sim.Step(0.05f, CrowdTest::Origin);
		Shoved = FMath::Max(Shoved, FVector2D::Distance(Sim.GetUnits()[0].Position, Held));
	}
	TestTrue(TEXT("the one holding its place was not pushed off it"), Shoved < 8.0);

	// All three were sent to the same point. They do not fight over it: each ends on a spot of its own and stands.
	TArray<FVector2D> Before;
	for (const FCrowdUnit& Unit : Sim.GetUnits())
	{
		Before.Add(Unit.Position);
	}
	FixTest::Run(Sim, 2.0f, CrowdTest::Origin);
	for (int32 Index = 1; Index < 4; ++Index)
	{
		const FCrowdUnit& Unit = Sim.GetUnits()[Index];
		TestTrue(FString::Printf(TEXT("unit %d is at the place"), Index), FVector2D::Distance(Unit.Position, Far) < 150.0);
		TestTrue(FString::Printf(TEXT("unit %d stands still"), Index), FVector2D::Distance(Unit.Position, Before[Index]) < 3.0);
		for (int32 Other = Index + 1; Other < 4; ++Other)
		{
			TestTrue(TEXT("no two stand in each other"), FVector2D::Distance(Unit.Position, Sim.GetUnits()[Other].Position) > 40.0);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArmyBesideWall, "WebTesting.Army.StandsStillWhenTheWizardStopsAtAWall", WIZ_TEST_FLAGS)
bool FArmyBesideWall::RunTest(const FString& Parameters)
{
	for (const bool bWater : { false, true })
	{
		FCrowdSim Sim;
		// No strolling, so a unit that moves while the wizard stands is one that cannot reach its spot.
		Sim.Tuning.RoamPauseMin = 100000.0f;
		Sim.Tuning.RoamPauseMax = 100000.0f;
		FCrowdObstacle Wall = CrowdTest::Wall(0.0, 190.0, 1500.0, 100.0);
		Wall.bLow = bWater;
		Sim.SetObstacles({ Wall });
		FVector2D Wizard(0.0, -1200.0);
		for (int32 Index = 0; Index < 20; ++Index)
		{
			Sim.AddSkeleton(Wizard + FVector2D(-200.0 + Index * 20.0, 100.0));
		}
		FixTest::Run(Sim, 3.0f, Wizard);
		// He walks up to the wall and stops against it.
		for (int32 Index = 0; Index < 40; ++Index)
		{
			Wizard.Y += 30.0;
			Sim.Step(0.05f, Wizard);
		}
		FixTest::Run(Sim, 5.0f, Wizard);
		TArray<FVector2D> Last;
		for (const FCrowdUnit& Unit : Sim.GetUnits())
		{
			Last.Add(Unit.Position);
		}
		double Walked = 0.0;
		for (int32 Step = 0; Step < 60; ++Step)
		{
			Sim.Step(0.05f, Wizard);
			for (int32 Index = 0; Index < Sim.GetUnits().Num(); ++Index)
			{
				Walked += FVector2D::Distance(Sim.GetUnits()[Index].Position, Last[Index]);
				Last[Index] = Sim.GetUnits()[Index].Position;
			}
		}
		TestTrue(bWater ? TEXT("beside water, the army stands: under 10 units walked each in 3 seconds")
			: TEXT("beside a wall, the army stands: under 10 units walked each in 3 seconds"), Walked / 20.0 < 10.0);
		int32 WithHim = 0;
		for (const FCrowdUnit& Unit : Sim.GetUnits())
		{
			// The reach was 450 when the roam circle was 350; the circle grew with the figures (2026-11-13 17:21 PST).
			WithHim += Unit.Position.Y < 90.0 && FVector2D::Distance(Unit.Position, Wizard) < Sim.Tuning.RoamRadius + 100.0 ? 1 : 0;
		}
		TestEqual(TEXT("all of it on his side, within reach of him"), WithHim, 20);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBonesInView, "WebTesting.Bones.DoNotFadeWhileOnScreen", WIZ_TEST_FLAGS)
bool FBonesInView::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	Sim.Tuning.BoneLife = 2.0f;
	Sim.DropBone(FVector2D(1000.0, 0.0), 1);
	Sim.DropBone(FVector2D(5000.0, 0.0), 1);
	Sim.SetView(FVector2D(500.0, -500.0), FVector2D(1500.0, 500.0));
	TestTrue(TEXT("the first pile is on screen"), Sim.IsInView(FVector2D(1000.0, 0.0)));
	TestFalse(TEXT("the second is not"), Sim.IsInView(FVector2D(5000.0, 0.0)));
	FixTest::Run(Sim, 6.0f, CrowdTest::Origin);
	TestEqual(TEXT("after three times their life, the pile on screen is still there and the other is gone"), Sim.GetBones().Num(), 1);
	// The camera moves away: its time starts now.
	Sim.SetView(FVector2D(-500.0, -500.0), FVector2D(500.0, 500.0));
	FixTest::Run(Sim, 1.5f, CrowdTest::Origin);
	TestEqual(TEXT("off screen for less than its life: still there"), Sim.GetBones().Num(), 1);
	FixTest::Run(Sim, 1.0f, CrowdTest::Origin);
	TestEqual(TEXT("off screen for its whole life: gone"), Sim.GetBones().Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArmyPresence, "WebTesting.Army.WeakerAwayFromTheWizard", WIZ_TEST_FLAGS)
bool FArmyPresence::RunTest(const FString& Parameters)
{
	// A skeleton and a grunt trade blows for a second, once beside the wizard and once far from him.
	const auto Fight = [](FVector2D Wizard, float& OutSkeleton, float& OutGrunt)
	{
		FCrowdSim Sim;
		CrowdTest::MakeEnemiesPlain(Sim);
		Sim.Tuning.SkeletonSpeed = 0.0f;
		Sim.Tuning.SkeletonRoamSpeed = 0.0f;
		Sim.Tuning.SkeletonCatchUpSpeed = 0.0f;
		Sim.Tuning.GruntSpeed = 0.0f;
		Sim.AddSkeleton(FVector2D(3000.0, 0.0));
		Sim.AddEnemy(ECrowdKind::Grunt, FVector2D(3040.0, 0.0));
		// Sent to hold the spot, so it fights there however far the wizard is.
		Sim.Command(FVector2D(3000.0, 0.0), 1, ECrowdCommandKind::Any);
		FixTest::Run(Sim, 1.0f, Wizard);
		OutSkeleton = 0.0f;
		OutGrunt = 0.0f;
		for (const FCrowdUnit& Unit : Sim.GetUnits())
		{
			(Unit.Team == ECrowdTeam::Army ? OutSkeleton : OutGrunt) = Unit.Health;
		}
	};
	float NearSkeleton = 0.0f;
	float NearGrunt = 0.0f;
	float FarSkeleton = 0.0f;
	float FarGrunt = 0.0f;
	Fight(FVector2D(2700.0, 0.0), NearSkeleton, NearGrunt);
	Fight(CrowdTest::Origin, FarSkeleton, FarGrunt);
	// A skeleton does 8 a second and has 20 health; a grunt does 5 and has 10.
	TestEqual(TEXT("beside him the skeleton does its full 8"), NearGrunt, 2.0f, 0.6f);
	TestEqual(TEXT("and takes the grunt's 5"), NearSkeleton, 15.0f, 0.4f);
	TestEqual(TEXT("far from him it does half: 4"), FarGrunt, 6.0f, 0.4f);
	TestEqual(TEXT("and takes half as much again: 7.5"), FarSkeleton, 12.5f, 0.5f);

	FCrowdSim Sim;
	Sim.AddSkeleton(FVector2D(500.0, 0.0));
	Sim.AddSkeleton(FVector2D(700.0, 0.0));
	TestTrue(TEXT("500 units from him is inside his presence"), Sim.IsInPresence(Sim.GetUnits()[0], CrowdTest::Origin));
	TestFalse(TEXT("700 is outside"), Sim.IsInPresence(Sim.GetUnits()[1], CrowdTest::Origin));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
