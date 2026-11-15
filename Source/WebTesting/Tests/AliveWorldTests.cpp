#include "Sim/LevelLayout.h"
#include "Tests/CrowdTestHelpers.h"
#include "Tests/WizTestFlags.h"

#if WITH_DEV_AUTOMATION_TESTS

// Direction of 2026-11-12 (fifth round of the day), after the director played the Outskirts:
// - bug: a Bolt that kills a small enemy in one hit did not alert the rest of its group;
// - Bolt has a maximum distance, so the wizard cannot snipe a group that has not noticed him;
// - enemies at a post move about a little; some stand;
// - patrols walk the roads, so camping is not safe;
// - more enemies; the level is larger beside the wizard.

namespace AliveTest
{
	void StepFor(FCrowdSim& Sim, float Seconds, FVector2D Wizard)
	{
		for (float Time = 0.0f; Time < Seconds; Time += 0.05f)
		{
			Sim.Step(0.05f, Wizard);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGarrisonOneShotKillWakes, "WebTesting.Garrison.AKillInOneHitWakesTheGroup", WIZ_TEST_FLAGS)
bool FGarrisonOneShotKillWakes::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	// The wizard stands outside the distance they notice him at, and his Bolt reaches them.
	Sim.Tuning.WakeRadius = 200.0f;
	Sim.Tuning.BoltRange = 3000.0f;
	Sim.Tuning.BoltLife = 5.0f;
	Sim.Tuning.PostWanderRadius = 0.0f;
	Sim.AddGarrisonEnemy(ECrowdKind::Grunt, FVector2D(1000.0, 0.0), 1);
	Sim.AddGarrisonEnemy(ECrowdKind::Grunt, FVector2D(1000.0, 150.0), 1);
	Sim.AddGarrisonEnemy(ECrowdKind::Grunt, FVector2D(1000.0, -150.0), 1);
	Sim.AddGarrisonEnemy(ECrowdKind::Grunt, FVector2D(1000.0, 3000.0), 2);
	// One Bolt does a grunt's whole health.
	Sim.FireBolt(CrowdTest::Origin, FVector2D(1.0, 0.0));
	int32 Woken = 0;
	for (int32 Index = 0; Index < 20; ++Index)
	{
		Woken += Sim.Step(0.05f, CrowdTest::Origin).SquadsWoken;
	}
	TestEqual(TEXT("the Bolt killed one grunt outright"), Sim.CountTeam(ECrowdTeam::Enemy), 3);
	TestEqual(TEXT("the two beside it left their post; the other group stays"), Sim.CountAsleep(), 1);
	TestEqual(TEXT("the step reports the group"), Woken, 1);
	AliveTest::StepFor(Sim, 1.0f, CrowdTest::Origin);
	for (const FCrowdUnit& Unit : Sim.GetUnits())
	{
		TestEqual(TEXT("the group that lost one charges; the other has not moved"), Unit.Position.X < 950.0, Unit.Squad == 1);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBoltHasARange, "WebTesting.Bolt.HasAMaximumDistance", WIZ_TEST_FLAGS)
bool FBoltHasARange::RunTest(const FString& Parameters)
{
	const FCrowdTuning Defaults;
	TestTrue(TEXT("Bolt has a range"), Defaults.BoltRange > 0.0f);
	TestTrue(TEXT("shorter than the distance a post notices the wizard at"), Defaults.BoltRange < Defaults.WakeRadius);

	for (int32 Case = 0; Case < 2; ++Case)
	{
		const bool bInRange = Case == 0;
		FCrowdSim Sim;
		Sim.Tuning.BruteSpeed = 0.0f;
		Sim.AddEnemy(ECrowdKind::Brute, FVector2D(Sim.Tuning.BoltRange + (bInRange ? -100.0 : 300.0), 0.0));
		Sim.FireBolt(CrowdTest::Origin, FVector2D(1.0, 0.0));
		AliveTest::StepFor(Sim, 3.0f, FVector2D(0.0, 50000.0));
		const FCrowdUnit* Brute = CrowdTest::First(Sim, ECrowdTeam::Enemy);
		if (!TestNotNull(TEXT("the brute"), Brute))
		{
			return false;
		}
		TestEqual(bInRange ? TEXT("inside the range: hit") : TEXT("300 past the range: not hit"), Brute->Health < Brute->MaxHealth, bInRange);
		TestEqual(TEXT("the Bolt is gone"), Sim.GetBolts().Num(), 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGarrisonMovesAbout, "WebTesting.Garrison.SomeMoveAboutAtTheirPost", WIZ_TEST_FLAGS)
bool FGarrisonMovesAbout::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	const int32 Count = 12;
	TArray<FVector2D> Starts;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		// Far enough apart that nobody pushes anybody.
		Starts.Add(FVector2D(5000.0, Index * 600.0));
		Sim.AddGarrisonEnemy(ECrowdKind::Grunt, Starts.Last(), 1);
	}
	// The order of units does not change while nobody dies.
	int32 Moved = 0;
	double Farthest = 0.0;
	TArray<bool> EverMoved;
	EverMoved.SetNumZeroed(Count);
	for (float Time = 0.0f; Time < 15.0f; Time += 0.05f)
	{
		Sim.Step(0.05f, CrowdTest::Origin);
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const double Distance = FVector2D::Distance(Sim.GetUnits()[Index].Position, Starts[Index]);
			Farthest = FMath::Max(Farthest, Distance);
			EverMoved[Index] |= Distance > 25.0;
		}
	}
	for (bool bMoved : EverMoved)
	{
		Moved += bMoved ? 1 : 0;
	}
	TestTrue(TEXT("several walk about"), Moved >= 4);
	TestTrue(TEXT("some stand"), Moved <= Count - 1);
	TestTrue(TEXT("nobody strays from the post"), Farthest <= Sim.Tuning.PostWanderRadius + 5.0);
	TestEqual(TEXT("and nobody has left it to charge"), Sim.CountAsleep(), Count);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPatrolWalksItsRoute, "WebTesting.Patrol.WalksItsRoadUntilItNoticesTheWizard", WIZ_TEST_FLAGS)
bool FPatrolWalksItsRoute::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	const FVector2D FarAway(0.0, 50000.0);
	TestFalse(TEXT("no road, no patrol"), Sim.AddPatrolEnemy(ECrowdKind::Grunt, CrowdTest::Origin, 5, 0));
	const int32 Route = Sim.AddRoute({ FVector2D(0.0, 0.0), FVector2D(1500.0, 0.0) });
	for (int32 Index = 0; Index < 3; ++Index)
	{
		TestTrue(TEXT("a patrol on the road"), Sim.AddPatrolEnemy(ECrowdKind::Grunt, FVector2D(0.0, Index * 60.0 - 60.0), 5, Route, 1));
	}
	TestEqual(TEXT("three in the group"), Sim.CountSquad(5), 3);
	if (Sim.GetUnits().Num() != 3)
	{
		return false;
	}

	// 150 a second: 1200 along after 8 seconds.
	AliveTest::StepFor(Sim, 8.0f, FarAway);
	for (const FCrowdUnit& Unit : Sim.GetUnits())
	{
		TestTrue(TEXT("after 8 seconds it is well along the road"), Unit.Position.X > 900.0 && Unit.Position.X < 1500.0);
		TestTrue(TEXT("and on it"), FMath::Abs(Unit.Position.Y) < 150.0);
	}
	// It reaches the end at 10 seconds and turns back.
	AliveTest::StepFor(Sim, 8.0f, FarAway);
	for (const FCrowdUnit& Unit : Sim.GetUnits())
	{
		TestTrue(TEXT("after 16 seconds it is on its way back"), Unit.Position.X < 900.0);
	}
	TestEqual(TEXT("it has not noticed the wizard"), Sim.CountAsleep(), 3);

	// The wizard stands on the road ahead of it.
	const FVector2D OnTheRoad(Sim.GetUnits()[0].Position.X - 600.0, 0.0);
	Sim.Step(0.05f, OnTheRoad);
	TestEqual(TEXT("600 away: the patrol notices him"), Sim.CountAsleep(), 0);
	AliveTest::StepFor(Sim, 5.0f, OnTheRoad);
	TestTrue(TEXT("and reaches him"), CrowdTest::CountTouching(Sim, OnTheRoad) > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLayoutOutskirtsRoutes, "WebTesting.Layout.OutskirtsHasRoadsForPatrols", WIZ_TEST_FLAGS)
bool FLayoutOutskirtsRoutes::RunTest(const FString& Parameters)
{
	const FLevelLayout Layout = FLevelLayout::MakeOutskirts();
	const TArray<FCrowdObstacle> Obstacles = Layout.MakeObstacles();
	TestTrue(TEXT("at least three roads are patrolled"), Layout.Routes.Num() >= 3);
	for (int32 RouteIndex = 0; RouteIndex < Layout.Routes.Num(); ++RouteIndex)
	{
		const TArray<FVector2D>& Route = Layout.Routes[RouteIndex];
		TestTrue(TEXT("a road has two ends"), Route.Num() >= 2);
		for (int32 Index = 1; Index < Route.Num(); ++Index)
		{
			for (const FCrowdObstacle& Obstacle : Obstacles)
			{
				double Alpha = 0.0;
				TestFalse(FString::Printf(TEXT("road %d, stretch %d: nothing stands in the way (%.0f, %.0f)"), RouteIndex, Index, Obstacle.Center.X, Obstacle.Center.Y),
					Obstacle.SegmentEnters(Route[Index - 1], Route[Index], 30.0, Alpha));
			}
		}
	}
	TestTrue(TEXT("more enemies than the first block-out's 98"), Layout.CountPostEnemies() >= 150);
	TestTrue(TEXT("more posts"), Layout.Posts.Num() >= 13);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLayoutOutskirtsScale, "WebTesting.Layout.OutskirtsScaleCanBeSet", WIZ_TEST_FLAGS)
bool FLayoutOutskirtsScale::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("the level is larger than the first block-out's 10 units a pixel"), FLevelLayout::OutskirtsUnitsPerPixel > 10.0);
	const FLevelLayout Small = FLevelLayout::MakeOutskirts(10.0);
	const FLevelLayout Large = FLevelLayout::MakeOutskirts(20.0);
	const auto FirstHouse = [](const FLevelLayout& Layout)
	{
		for (const FLevelPiece& Piece : Layout.Pieces)
		{
			if (Piece.Kind == ELevelPiece::House)
			{
				return Piece;
			}
		}
		return FLevelPiece();
	};
	TestTrue(TEXT("twice the scale: a house twice as large"), FirstHouse(Large).HalfSize.Equals(FirstHouse(Small).HalfSize * 2.0, 0.01));
	TestTrue(TEXT("and twice as far away"), FirstHouse(Large).Center.Equals(FirstHouse(Small).Center * 2.0, 0.01));
	TestTrue(TEXT("the shrine too"), Large.Shrine.Equals(Small.Shrine * 2.0, 0.01));
	TestTrue(TEXT("the posts too"), Large.Posts.Num() > 0 && Large.Posts[0].Position.Equals(Small.Posts[0].Position * 2.0, 0.01));
	const FLevelLayout Default = FLevelLayout::MakeOutskirts();
	TestTrue(TEXT("with no scale given it is the default"), FirstHouse(Default).HalfSize.Equals(FirstHouse(Small).HalfSize * (FLevelLayout::OutskirtsUnitsPerPixel / 10.0), 0.01));
	// Both scales can be walked from the start to the commander.
	TestTrue(TEXT("the small level can be walked"), Small.CanWalk(Small.Start, Small.Commander));
	TestTrue(TEXT("the large level can be walked"), Large.CanWalk(Large.Start, Large.Commander));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
