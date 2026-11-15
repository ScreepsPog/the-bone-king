#include "Tests/CrowdTestHelpers.h"
#include "Tests/WizTestFlags.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrowdObstacleBoltStopsAtWall, "WebTesting.Obstacles.BoltStopsAtWall", WIZ_TEST_FLAGS)
bool FCrowdObstacleBoltStopsAtWall::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	Sim.Tuning.GruntSpeed = 0.0f;
	Sim.SetObstacles({ CrowdTest::Wall(250.0, 0.0, 25.0, 200.0) });
	Sim.AddEnemy(ECrowdKind::Grunt, FVector2D(500.0, 0.0));
	Sim.FireBolt(CrowdTest::Origin, FVector2D(1.0, 0.0));
	for (int32 Index = 0; Index < 40 && Sim.GetBolts().Num() > 0; ++Index)
	{
		Sim.Step(0.05f, CrowdTest::Origin);
	}
	TestEqual(TEXT("the bolt is gone"), Sim.GetBolts().Num(), 0);
	const FCrowdUnit* Grunt = CrowdTest::First(Sim, ECrowdTeam::Enemy);
	if (TestNotNull(TEXT("the grunt behind the wall lives"), Grunt))
	{
		TestEqual(TEXT("and was not hurt"), Grunt->Health, Sim.Tuning.GruntHealth);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrowdObstacleBoltPassesBesideWall, "WebTesting.Obstacles.BoltPassesBesideWall", WIZ_TEST_FLAGS)
bool FCrowdObstacleBoltPassesBesideWall::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	Sim.Tuning.GruntSpeed = 0.0f;
	Sim.SetObstacles({ CrowdTest::Wall(250.0, 300.0, 25.0, 100.0) });
	Sim.AddEnemy(ECrowdKind::Grunt, FVector2D(500.0, 0.0));
	Sim.FireBolt(CrowdTest::Origin, FVector2D(1.0, 0.0));
	for (int32 Index = 0; Index < 40 && Sim.GetBolts().Num() > 0; ++Index)
	{
		Sim.Step(0.05f, CrowdTest::Origin);
	}
	TestEqual(TEXT("a wall beside the path does not stop the bolt"), Sim.CountTeam(ECrowdTeam::Enemy), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrowdObstacleEnemyGoesAroundWall, "WebTesting.Obstacles.EnemyGoesAroundWall", WIZ_TEST_FLAGS)
bool FCrowdObstacleEnemyGoesAroundWall::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	CrowdTest::MakeEnemiesPlain(Sim);
	Sim.SetObstacles({ CrowdTest::Wall(500.0, 0.0, 50.0, 300.0) });
	Sim.AddEnemy(ECrowdKind::Grunt, FVector2D(1000.0, 0.0));
	bool bEverInWall = false;
	for (int32 Index = 0; Index < 200; ++Index)
	{
		Sim.Step(0.05f, CrowdTest::Origin);
		bEverInWall |= CrowdTest::AnyUnitInWall(Sim);
	}
	TestFalse(TEXT("the grunt never walked into the wall"), bEverInWall);
	TestEqual(TEXT("the grunt reached the wizard within 10 seconds"), CrowdTest::CountTouching(Sim, CrowdTest::Origin), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrowdObstacleEnemyLeavesPocket, "WebTesting.Obstacles.EnemyLeavesPocket", WIZ_TEST_FLAGS)
bool FCrowdObstacleEnemyLeavesPocket::RunTest(const FString& Parameters)
{
	// A pocket of three walls that opens away from the wizard. The grunt starts inside and must first walk away from him.
	FCrowdSim Sim;
	CrowdTest::MakeEnemiesPlain(Sim);
	Sim.SetObstacles({
		CrowdTest::Wall(500.0, 0.0, 25.0, 225.0),
		CrowdTest::Wall(700.0, 250.0, 225.0, 25.0),
		CrowdTest::Wall(700.0, -250.0, 225.0, 25.0) });
	Sim.AddEnemy(ECrowdKind::Grunt, FVector2D(700.0, 0.0));
	bool bEverInWall = false;
	for (int32 Index = 0; Index < 240; ++Index)
	{
		Sim.Step(0.05f, CrowdTest::Origin);
		bEverInWall |= CrowdTest::AnyUnitInWall(Sim);
	}
	TestFalse(TEXT("the grunt never walked into a wall"), bEverInWall);
	TestEqual(TEXT("the grunt found its way out and reached the wizard within 12 seconds"), CrowdTest::CountTouching(Sim, CrowdTest::Origin), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrowdObstacleSpawnInsideIsMovedOut, "WebTesting.Obstacles.SpawnInsideIsMovedOut", WIZ_TEST_FLAGS)
bool FCrowdObstacleSpawnInsideIsMovedOut::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	Sim.SetObstacles({ CrowdTest::Wall(500.0, 0.0, 50.0, 300.0) });
	Sim.AddEnemy(ECrowdKind::Grunt, FVector2D(500.0, 0.0));
	Sim.AddSkeleton(FVector2D(510.0, 10.0));
	TestEqual(TEXT("both were added"), Sim.GetUnits().Num(), 2);
	TestFalse(TEXT("neither stands in the wall"), CrowdTest::AnyUnitInWall(Sim));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrowdObstacleSkeletonComesBackAroundWall, "WebTesting.Obstacles.SkeletonComesBackAroundWall", WIZ_TEST_FLAGS)
bool FCrowdObstacleSkeletonComesBackAroundWall::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	Sim.SetObstacles({ CrowdTest::Wall(500.0, 0.0, 50.0, 300.0) });
	Sim.AddSkeleton(FVector2D(1000.0, 0.0));
	bool bEverInWall = false;
	for (int32 Index = 0; Index < 200; ++Index)
	{
		Sim.Step(0.05f, CrowdTest::Origin);
		bEverInWall |= CrowdTest::AnyUnitInWall(Sim);
	}
	TestFalse(TEXT("the skeleton never walked into the wall"), bEverInWall);
	const FCrowdUnit* Skeleton = CrowdTest::First(Sim, ECrowdTeam::Army);
	if (TestNotNull(TEXT("skeleton exists"), Skeleton))
	{
		TestTrue(TEXT("the skeleton is back with the wizard"), Skeleton->Position.Size() <= Sim.Tuning.RoamRadius + 10.0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrowdFlowFieldLeadsAroundWall, "WebTesting.FlowField.LeadsAroundWall", WIZ_TEST_FLAGS)
bool FCrowdFlowFieldLeadsAroundWall::RunTest(const FString& Parameters)
{
	const FCrowdObstacle Wall = CrowdTest::Wall(500.0, 0.0, 50.0, 300.0);
	FCrowdFlowField Field;
	TestFalse(TEXT("not built yet"), Field.IsBuiltFor(CrowdTest::Origin));
	Field.Build(FVector2D(10.0, 10.0), MakeArrayView(&Wall, 1), 30.0);
	TestTrue(TEXT("built for the target"), Field.IsBuiltFor(FVector2D(10.0, 10.0)));
	TestTrue(TEXT("still good while the target stays in its square"), Field.IsBuiltFor(FVector2D(60.0, 80.0)));
	TestFalse(TEXT("stale once the target leaves its square"), Field.IsBuiltFor(FVector2D(150.0, 10.0)));

	const FVector2D Behind = Field.GetDirection(FVector2D(800.0, 0.0));
	TestTrue(TEXT("behind the wall there is a direction"), FMath::IsNearlyEqual(Behind.Size(), 1.0, 0.01));
	TestTrue(TEXT("and it leads sideways, around the wall"), FMath::Abs(Behind.Y) >= 0.4);

	const FVector2D Open = Field.GetDirection(FVector2D(50.0, 800.0));
	TestTrue(TEXT("on open ground it leads at the target"), FVector2D::DotProduct(Open, FVector2D(0.0, -1.0)) >= 0.9);

	TestTrue(TEXT("outside the grid there is no direction"), Field.GetDirection(FVector2D(10000.0, 0.0)).IsNearlyZero());

	// Following the field from behind the wall arrives without crossing it.
	FVector2D Point(800.0, 0.0);
	bool bCrossedWall = false;
	for (int32 Index = 0; Index < 400 && Point.Size() > 150.0; ++Index)
	{
		const FVector2D Direction = Field.GetDirection(Point);
		if (Direction.IsNearlyZero())
		{
			break;
		}
		Point += Direction * 20.0;
		bCrossedWall |= Wall.Contains(Point, 0.0);
	}
	TestFalse(TEXT("the way does not cross the wall"), bCrossedWall);
	TestTrue(TEXT("the way arrives near the target"), Point.Size() <= 150.0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
