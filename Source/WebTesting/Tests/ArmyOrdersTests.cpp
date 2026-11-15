#include "Sim/CrowdSim.h"
#include "Sim/LevelLayout.h"
#include "Tests/CrowdTestHelpers.h"
#include "Tests/WizTestFlags.h"

#if WITH_DEV_AUTOMATION_TESTS

// Direction of 2026-11-13 (14:30 PST): nothing walks on water; sent units spread out and move about their place, go
// around walls to it, and send some of themselves after an enemy that shoots at them; enemies fight sent units when
// the wizard is far away; the army counts toward the wizard's bone worth; Bolt fires slower.

namespace OrdersTest
{
	FCrowdStepResult Run(FCrowdSim& Sim, float Seconds, FVector2D Wizard, float Step = 0.05f)
	{
		FCrowdStepResult Total;
		for (float Time = 0.0f; Time < Seconds; Time += Step)
		{
			const FCrowdStepResult Result = Sim.Step(Step, Wizard);
			Total.EnemiesKilled += Result.EnemiesKilled;
			Total.SkeletonsLost += Result.SkeletonsLost;
		}
		return Total;
	}

	/** Units whose middle is over water or inside a wall. */
	int32 CountInside(const FCrowdSim& Sim)
	{
		int32 Count = 0;
		for (const FCrowdUnit& Unit : Sim.GetUnits())
		{
			for (const FCrowdObstacle& Obstacle : Sim.GetObstacles())
			{
				if (Obstacle.Contains(Unit.Position, 0.0))
				{
					++Count;
					break;
				}
			}
		}
		return Count;
	}

	/** Steps the crowd and returns the most units that were over water or in a wall after any one step. */
	int32 RunAndCountInside(FCrowdSim& Sim, float Seconds, FVector2D Wizard, float Step)
	{
		int32 Worst = 0;
		for (float Time = 0.0f; Time < Seconds; Time += Step)
		{
			Sim.Step(Step, Wizard);
			Worst = FMath::Max(Worst, CountInside(Sim));
		}
		return Worst;
	}

	double MeanDistance(const FCrowdSim& Sim, FVector2D Point)
	{
		double Sum = 0.0;
		int32 Count = 0;
		for (const FCrowdUnit& Unit : Sim.GetUnits())
		{
			if (Unit.Team == ECrowdTeam::Army)
			{
				Sum += FVector2D::Distance(Unit.Position, Point);
				++Count;
			}
		}
		return Count > 0 ? Sum / Count : 0.0;
	}

	int32 CountArmyNear(const FCrowdSim& Sim, FVector2D Point, double Radius)
	{
		int32 Count = 0;
		for (const FCrowdUnit& Unit : Sim.GetUnits())
		{
			Count += Unit.Team == ECrowdTeam::Army && FVector2D::Distance(Unit.Position, Point) <= Radius ? 1 : 0;
		}
		return Count;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWaterRiver, "WebTesting.Water.NothingStandsOnTheRiver", WIZ_TEST_FLAGS)
bool FWaterRiver::RunTest(const FString& Parameters)
{
	// A river the way the Outskirts builds one: slices side by side, each a little off the last, with water on both
	// sides of the wizard's bank. Enemies on the far bank want him, his army crowds the near bank, some are sent across.
	TArray<FCrowdObstacle> River;
	for (int32 Index = -8; Index <= 8; ++Index)
	{
		FCrowdObstacle Slice = CrowdTest::Wall(Index * 120.0, 600.0 + (Index % 3) * 45.0, 60.0, 150.0);
		Slice.bLow = true;
		River.Add(Slice);
	}
	for (const float Step : { 0.05f, 0.1f })
	{
		FCrowdSim Sim;
		Sim.SetObstacles(River);
		const FVector2D Wizard(0.0, 330.0);
		for (int32 Index = 0; Index < 30; ++Index)
		{
			Sim.AddEnemy(Index % 5 == 0 ? ECrowdKind::Assassin : ECrowdKind::Grunt, FVector2D(-600.0 + Index * 40.0, 1000.0 + (Index % 4) * 50.0));
			Sim.AddSkeleton(FVector2D(-300.0 + Index * 20.0, 380.0 - (Index % 3) * 40.0), Index % 6 == 0 ? ECrowdKind::Guard : ECrowdKind::Skeleton);
		}
		Sim.Command(FVector2D(100.0, 620.0), 6, ECrowdCommandKind::Any);
		Sim.Command(FVector2D(-200.0, 1100.0), 6, ECrowdCommandKind::Any);
		TestEqual(FString::Printf(TEXT("steps of %.2f: nobody is ever over the water"), Step), OrdersTest::RunAndCountInside(Sim, 20.0f, Wizard, Step), 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWaterOutskirts, "WebTesting.Water.NothingStandsOnTheOutskirtsRiver", WIZ_TEST_FLAGS)
bool FWaterOutskirts::RunTest(const FString& Parameters)
{
	const FLevelLayout Layout = FLevelLayout::MakeOutskirts();
	const TArray<FCrowdObstacle> Obstacles = Layout.MakeObstacles();
	// A stretch of the river about the middle of the level.
	TArray<int32> Water;
	for (int32 Index = 0; Index < Obstacles.Num(); ++Index)
	{
		if (Obstacles[Index].bLow)
		{
			Water.Add(Index);
		}
	}
	TestTrue(TEXT("the outskirts have a river"), Water.Num() > 10);
	if (Water.Num() <= 10)
	{
		return false;
	}
	for (const int32 Pick : { Water.Num() / 4, Water.Num() / 2, (Water.Num() * 3) / 4 })
	{
		const FCrowdObstacle& Slice = Obstacles[Water[Pick]];
		FCrowdSim Sim;
		Sim.SetObstacles(Obstacles);
		// The wizard on one bank, right at the water; enemies and a sent group on the other.
		const FVector2D Wizard = Slice.Center + FVector2D(0.0, Slice.HalfSize.Y + 120.0);
		const FVector2D Far = Slice.Center - FVector2D(0.0, Slice.HalfSize.Y + 250.0);
		for (int32 Index = 0; Index < 24; ++Index)
		{
			Sim.AddEnemy(Index % 6 == 0 ? ECrowdKind::Assassin : ECrowdKind::Grunt, Far + FVector2D(-300.0 + Index * 25.0, -(Index % 3) * 60.0));
			Sim.AddSkeleton(Wizard + FVector2D(-200.0 + Index * 18.0, 60.0 + (Index % 3) * 40.0));
		}
		Sim.Command(Slice.Center, 8, ECrowdCommandKind::Any);
		Sim.Command(Far, 8, ECrowdCommandKind::Any);
		TestEqual(FString::Printf(TEXT("river slice %d: nobody is ever over the water"), Pick), OrdersTest::RunAndCountInside(Sim, 25.0f, Wizard, 0.1f), 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOrdersSpread, "WebTesting.Command.SentUnitsSpreadOutAndMoveAbout", WIZ_TEST_FLAGS)
bool FOrdersSpread::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	for (int32 Index = 0; Index < 20; ++Index)
	{
		Sim.AddSkeleton(FVector2D(Index * 12.0, 0.0));
	}
	const FVector2D Place(1200.0, 0.0);
	Sim.Command(Place, 20, ECrowdCommandKind::Any);
	OrdersTest::Run(Sim, 8.0f, CrowdTest::Origin);
	TestTrue(TEXT("twenty spread out: on average over 120 units from the middle"), OrdersTest::MeanDistance(Sim, Place) > 120.0);
	TestEqual(TEXT("and all stay inside the rejoin circle"), OrdersTest::CountArmyNear(Sim, Place, Sim.Tuning.CommandRejoinRadius), 20);

	// They do not stand still: over the next while, most of them walk to another spot.
	TArray<FVector2D> Before;
	for (const FCrowdUnit& Unit : Sim.GetUnits())
	{
		Before.Add(Unit.Position);
	}
	OrdersTest::Run(Sim, 8.0f, CrowdTest::Origin);
	int32 Moved = 0;
	for (int32 Index = 0; Index < Sim.GetUnits().Num(); ++Index)
	{
		Moved += FVector2D::Distance(Sim.GetUnits()[Index].Position, Before[Index]) > 30.0 ? 1 : 0;
	}
	TestTrue(TEXT("more than half moved to another spot"), Moved > 10);
	TestEqual(TEXT("still inside the circle"), OrdersTest::CountArmyNear(Sim, Place, Sim.Tuning.CommandRejoinRadius), 20);
	TestEqual(TEXT("still sent"), Sim.CountCommanded(), 20);

	// A small group stays close together.
	FCrowdSim Small;
	for (int32 Index = 0; Index < 3; ++Index)
	{
		Small.AddSkeleton(FVector2D(Index * 12.0, 0.0));
	}
	Small.Command(Place, 3, ECrowdCommandKind::Any);
	OrdersTest::Run(Small, 8.0f, CrowdTest::Origin);
	TestEqual(TEXT("three stand within 130 units of the place"), OrdersTest::CountArmyNear(Small, Place, 130.0), 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOrdersAroundWall, "WebTesting.Command.SentUnitsGoAroundAWall", WIZ_TEST_FLAGS)
bool FOrdersAroundWall::RunTest(const FString& Parameters)
{
	for (const bool bWater : { false, true })
	{
		FCrowdSim Sim;
		FCrowdObstacle Wall = CrowdTest::Wall(600.0, 0.0, 40.0, 700.0);
		Wall.bLow = bWater;
		Sim.SetObstacles({ Wall });
		for (int32 Index = 0; Index < 8; ++Index)
		{
			Sim.AddSkeleton(FVector2D(100.0 + Index * 15.0, -60.0 + (Index % 3) * 60.0));
		}
		const FVector2D Place(1200.0, 0.0);
		Sim.Command(Place, 8, ECrowdCommandKind::Any);
		const int32 Inside = OrdersTest::RunAndCountInside(Sim, 12.0f, CrowdTest::Origin, 0.05f);
		TestEqual(bWater ? TEXT("all eight reach the place around the water") : TEXT("all eight reach the place around the wall"),
			OrdersTest::CountArmyNear(Sim, Place, Sim.Tuning.CommandRejoinRadius), 8);
		TestEqual(TEXT("without going through it"), Inside, 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOrdersRespond, "WebTesting.Command.ShotAtTheySendSomeAfterTheShooter", WIZ_TEST_FLAGS)
bool FOrdersRespond::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	CrowdTest::MakeEnemiesPlain(Sim);
	const FVector2D Place(3000.0, 0.0);
	for (int32 Index = 0; Index < 10; ++Index)
	{
		Sim.AddSkeleton(Place + FVector2D(-50.0 + Index * 10.0, 0.0));
	}
	Sim.Command(Place, 10, ECrowdCommandKind::Any);
	OrdersTest::Run(Sim, 2.0f, CrowdTest::Origin);
	TestEqual(TEXT("nobody leaves the place while nothing shoots"), Sim.CountHunting(), 0);

	// An archer out of the group's reach (300), inside its own range (800), and not so close that it runs (380).
	Sim.AddEnemy(ECrowdKind::Archer, Place + FVector2D(0.0, 700.0));
	int32 MostHunting = 0;
	int32 Killed = 0;
	for (int32 Index = 0; Index < 400 && Killed == 0; ++Index)
	{
		Killed += Sim.Step(0.05f, CrowdTest::Origin).EnemiesKilled;
		MostHunting = FMath::Max(MostHunting, Sim.CountHunting());
	}
	TestTrue(TEXT("some of the group went after the archer"), MostHunting >= 2);
	TestTrue(TEXT("but not most of it: the rest held the place"), MostHunting <= 5);
	TestEqual(TEXT("the archer is dead"), Killed, 1);
	OrdersTest::Run(Sim, 6.0f, CrowdTest::Origin);
	TestEqual(TEXT("the hunt is over"), Sim.CountHunting(), 0);
	TestEqual(TEXT("everyone left alive is back at the place"),
		OrdersTest::CountArmyNear(Sim, Place, Sim.Tuning.CommandRejoinRadius), Sim.CountTeam(ECrowdTeam::Army));
	TestEqual(TEXT("and still sent"), Sim.CountCommanded(), Sim.CountTeam(ECrowdTeam::Army));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOrdersEnemiesFightBack, "WebTesting.Command.EnemiesFightSentUnitsWhenTheWizardIsFar", WIZ_TEST_FLAGS)
bool FOrdersEnemiesFightBack::RunTest(const FString& Parameters)
{
	const FVector2D Place(3000.0, 150.0);
	const auto Make = [&Place](FCrowdSim& Sim)
	{
		CrowdTest::MakeEnemiesPlain(Sim);
		CrowdTest::FreezeArmy(Sim);
		Sim.Tuning.GuardSpeed = 0.0f;
		for (int32 Index = 0; Index < 6; ++Index)
		{
			Sim.AddSkeleton(Place + FVector2D(-50.0 + Index * 20.0, 0.0));
			// Not in the skeletons' way to the wizard: beside them, 250 units off.
			Sim.AddGarrisonEnemy(ECrowdKind::Grunt, FVector2D(Place.X - 50.0 + Index * 20.0, -100.0), 0);
		}
		Sim.Command(Place, 6, ECrowdCommandKind::Any);
	};
	const auto LowestHealth = [](const FCrowdSim& Sim)
	{
		float Lowest = TNumericLimits<float>::Max();
		for (const FCrowdUnit& Unit : Sim.GetUnits())
		{
			if (Unit.Team == ECrowdTeam::Army)
			{
				Lowest = FMath::Min(Lowest, Unit.Health / Unit.MaxHealth);
			}
		}
		return Lowest;
	};

	// The wizard is far away: the post turns on the units that were sent at it.
	FCrowdSim Far;
	Make(Far);
	OrdersTest::Run(Far, 3.0f, CrowdTest::Origin);
	TestEqual(TEXT("the post woke"), Far.CountAsleep(), 0);
	TestTrue(TEXT("the grunts hurt the sent skeletons"), Far.CountTeam(ECrowdTeam::Army) < 6 || LowestHealth(Far) < 1.0f);
	int32 Stayed = 0;
	for (const FCrowdUnit& Unit : Far.GetUnits())
	{
		Stayed += Unit.Team == ECrowdTeam::Enemy && FVector2D::Distance(Unit.Position, Place) < 400.0 ? 1 : 0;
	}
	TestEqual(TEXT("and did not leave for the wizard"), Stayed, 6);

	// The wizard is close: they go for him, as before.
	FCrowdSim Near;
	Make(Near);
	const FVector2D Wizard(Place.X, -450.0);
	OrdersTest::Run(Near, 3.0f, Wizard);
	TestTrue(TEXT("with the wizard close, the grunts are on him"), CrowdTest::CountTouching(Near, Wizard) >= 4);
	TestEqual(TEXT("and no skeleton was hurt"), LowestHealth(Near), 1.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBoneWorth, "WebTesting.Bones.TheArmyCountsTowardBoneWorth", WIZ_TEST_FLAGS)
bool FBoneWorth::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	TestEqual(TEXT("no army, no worth"), Sim.GetArmyBoneValue(), 0);
	for (int32 Index = 0; Index < 28; ++Index)
	{
		Sim.AddSkeleton(FVector2D(Index * 20.0, 0.0));
	}
	// Each cost 3 bones to raise and counts for a third of that.
	TestEqual(TEXT("28 skeletons count for 28 bones"), Sim.GetArmyBoneValue(), 28);
	Sim.DropBone(CrowdTest::Origin, 5);
	Sim.Step(0.05f, CrowdTest::Origin);
	TestEqual(TEXT("5 bones held"), Sim.GetBonesHeld(), 5);
	TestEqual(TEXT("worth 33 with the army"), Sim.GetBoneWorth(), 33);
	// A Guard costs 8 at the shrine and counts for 3 (2.67 rounded; director, 2026-11-13 15:10 PST). This was 8 for
	// three while the total was rounded down instead of each unit.
	for (int32 Index = 0; Index < 3; ++Index)
	{
		Sim.AddSkeleton(FVector2D(Index * 20.0, 100.0), ECrowdKind::Guard);
	}
	// Changed 2026-11-14 19:40 PST (director: Guards and Marksmen do not "count towards the bone currency"): the three
	// Guards used to add 9.
	TestEqual(TEXT("three guards add nothing"), Sim.GetArmyBoneValue(), 28);
	Sim.Command(FVector2D(2000.0, 0.0), 10, ECrowdCommandKind::Any);
	TestEqual(TEXT("sent units still count"), Sim.GetArmyBoneValue(), 28);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBoltSlower, "WebTesting.Bolt.FiresSlower", WIZ_TEST_FLAGS)
bool FBoltSlower::RunTest(const FString& Parameters)
{
	// It was a Bolt every 0.2 seconds, five a second. Direction of 2026-11-13: too quick; shots should still matter.
	const FCrowdTuning Tuning;
	TestTrue(TEXT("at most about two and a half Bolts a second"), Tuning.BoltInterval >= 0.4f);
	TestTrue(TEXT("and no slower than one a second"), Tuning.BoltInterval <= 1.0f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
