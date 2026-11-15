#include "Tests/CrowdTestHelpers.h"
#include "Tests/WizTestFlags.h"

#if WITH_DEV_AUTOMATION_TESTS

// How the army and the enemies choose where to go (direction of 2026-11-12 02:00 PST):
// the army stays with the wizard and roams; enemies charge the wizard and fight only a skeleton in their way.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrowdArmyIgnoresEnemyOutOfReach, "WebTesting.Army.IgnoresEnemyOutOfReach", WIZ_TEST_FLAGS)
bool FCrowdArmyIgnoresEnemyOutOfReach::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	Sim.Tuning.GruntSpeed = 0.0f;
	Sim.AddSkeleton(FVector2D(100.0, 0.0));
	Sim.AddEnemy(ECrowdKind::Grunt, FVector2D(800.0, 0.0));

	double Farthest = 0.0;
	for (int32 Index = 0; Index < 50; ++Index)
	{
		Sim.Step(0.1f, CrowdTest::Origin);
		if (const FCrowdUnit* Skeleton = CrowdTest::First(Sim, ECrowdTeam::Army))
		{
			Farthest = FMath::Max(Farthest, Skeleton->Position.Size());
		}
	}
	TestTrue(TEXT("the skeleton never left the roam circle"), Farthest <= Sim.Tuning.RoamRadius + 5.0);
	const FCrowdUnit* Grunt = CrowdTest::First(Sim, ECrowdTeam::Enemy);
	if (TestNotNull(TEXT("the far grunt is still there"), Grunt))
	{
		TestEqual(TEXT("the far grunt was not attacked"), Grunt->Health, Sim.Tuning.GruntHealth);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrowdArmyAttacksEnemyInReach, "WebTesting.Army.AttacksEnemyInReach", WIZ_TEST_FLAGS)
bool FCrowdArmyAttacksEnemyInReach::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	Sim.Tuning.GruntSpeed = 0.0f;
	Sim.AddSkeleton(FVector2D(100.0, 0.0));
	Sim.AddEnemy(ECrowdKind::Grunt, FVector2D(300.0, 0.0));
	for (int32 Index = 0; Index < 30; ++Index)
	{
		Sim.Step(0.1f, CrowdTest::Origin);
	}
	TestEqual(TEXT("the near grunt was killed"), Sim.CountTeam(ECrowdTeam::Enemy), 0);
	TestEqual(TEXT("and left bones"), Sim.GetBones().Num() + Sim.GetBonesHeld(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrowdArmyRoamsWithWizard, "WebTesting.Army.RoamsWithWizard", WIZ_TEST_FLAGS)
bool FCrowdArmyRoamsWithWizard::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	for (int32 Index = 0; Index < 6; ++Index)
	{
		Sim.AddSkeleton(FVector2D(-150.0 + Index * 60.0, 120.0));
	}

	const auto FarthestFrom = [&Sim](FVector2D Wizard)
	{
		double Farthest = 0.0;
		for (const FCrowdUnit& Unit : Sim.GetUnits())
		{
			Farthest = FMath::Max(Farthest, FVector2D::Distance(Unit.Position, Wizard));
		}
		return Farthest;
	};

	// The wizard runs at full speed for 5 seconds.
	FVector2D Wizard = CrowdTest::Origin;
	double FarthestWhileRunning = 0.0;
	for (int32 Index = 0; Index < 100; ++Index)
	{
		Wizard.X += 600.0 * 0.05;
		Sim.Step(0.05f, Wizard);
		FarthestWhileRunning = FMath::Max(FarthestWhileRunning, FarthestFrom(Wizard));
	}
	TestTrue(TEXT("the army keeps up with a running wizard"), FarthestWhileRunning <= Sim.Tuning.LeashRadius);

	// He stops. The army gathers around him, spread out.
	for (int32 Index = 0; Index < 60; ++Index)
	{
		Sim.Step(0.05f, Wizard);
	}
	TestTrue(TEXT("the army is inside the roam circle"), FarthestFrom(Wizard) <= Sim.Tuning.RoamRadius + 10.0);
	double WidestGap = 0.0;
	for (const FCrowdUnit& Unit : Sim.GetUnits())
	{
		for (const FCrowdUnit& Other : Sim.GetUnits())
		{
			WidestGap = FMath::Max(WidestGap, FVector2D::Distance(Unit.Position, Other.Position));
		}
	}
	TestTrue(TEXT("the army is spread out, not on one spot"), WidestGap >= 100.0);

	// While he stands, the skeletons wander.
	TArray<FVector2D> Before;
	for (const FCrowdUnit& Unit : Sim.GetUnits())
	{
		Before.Add(Unit.Position);
	}
	double FarthestWhileIdle = 0.0;
	for (int32 Index = 0; Index < 160; ++Index)
	{
		Sim.Step(0.05f, Wizard);
		FarthestWhileIdle = FMath::Max(FarthestWhileIdle, FarthestFrom(Wizard));
	}
	int32 Wandered = 0;
	for (int32 Index = 0; Index < Before.Num(); ++Index)
	{
		Wandered += FVector2D::Distance(Before[Index], Sim.GetUnits()[Index].Position) > 60.0 ? 1 : 0;
	}
	TestTrue(TEXT("at least half the army wandered in 8 idle seconds"), Wandered >= 3);
	TestTrue(TEXT("wandering stays inside the roam circle"), FarthestWhileIdle <= Sim.Tuning.RoamRadius + 10.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrowdEnemyIgnoresSkeletonOffItsPath, "WebTesting.Enemy.IgnoresSkeletonOffItsPath", WIZ_TEST_FLAGS)
bool FCrowdEnemyIgnoresSkeletonOffItsPath::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	CrowdTest::MakeEnemiesPlain(Sim);
	CrowdTest::FreezeArmy(Sim);
	Sim.AddSkeleton(FVector2D(400.0, 300.0));
	Sim.AddEnemy(ECrowdKind::Grunt, FVector2D(800.0, 0.0));
	for (int32 Index = 0; Index < 10; ++Index)
	{
		Sim.Step(0.1f, CrowdTest::Origin);
	}
	const FCrowdUnit* Grunt = CrowdTest::First(Sim, ECrowdTeam::Enemy);
	if (TestNotNull(TEXT("grunt exists"), Grunt))
	{
		TestTrue(TEXT("the grunt kept to the wizard's line"), FMath::Abs(Grunt->Position.Y) <= 1.0);
		TestTrue(TEXT("the grunt walked 300 units toward the wizard"), FMath::Abs(Grunt->Position.X - 500.0) <= 5.0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrowdEnemyFightsSkeletonInItsWay, "WebTesting.Enemy.FightsSkeletonInItsWay", WIZ_TEST_FLAGS)
bool FCrowdEnemyFightsSkeletonInItsWay::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	CrowdTest::MakeEnemiesPlain(Sim);
	CrowdTest::FreezeArmy(Sim);
	Sim.AddSkeleton(FVector2D(400.0, 0.0));
	Sim.AddEnemy(ECrowdKind::Grunt, FVector2D(800.0, 0.0));
	for (int32 Index = 0; Index < 40; ++Index)
	{
		Sim.Step(0.1f, CrowdTest::Origin);
	}
	const FCrowdUnit* Grunt = CrowdTest::First(Sim, ECrowdTeam::Enemy);
	const FCrowdUnit* Skeleton = CrowdTest::First(Sim, ECrowdTeam::Army);
	if (TestNotNull(TEXT("grunt exists"), Grunt) && TestNotNull(TEXT("skeleton exists"), Skeleton))
	{
		TestTrue(TEXT("the grunt stopped at the skeleton"), Grunt->Position.X >= 440.0 && Grunt->Position.X <= 500.0);
		TestTrue(TEXT("the grunt is hurting the skeleton"), Skeleton->Health < Sim.Tuning.SkeletonHealth);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrowdEnemyWaveFansOut, "WebTesting.Enemy.WaveFansOut", WIZ_TEST_FLAGS)
bool FCrowdEnemyWaveFansOut::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	Sim.Tuning.SeparationRadius = 0.0f; // any spread must come from the bent approach, not from pushing apart
	Sim.Tuning.EnemySpeedVariation = 0.0f;
	for (int32 Index = 0; Index < 12; ++Index)
	{
		Sim.AddEnemy(ECrowdKind::Grunt, FVector2D(1500.0, 0.0));
	}
	for (int32 Index = 0; Index < 20; ++Index)
	{
		Sim.Step(0.1f, CrowdTest::Origin);
	}
	double Lowest = TNumericLimits<double>::Max();
	double Highest = TNumericLimits<double>::Lowest();
	for (const FCrowdUnit& Unit : Sim.GetUnits())
	{
		Lowest = FMath::Min(Lowest, Unit.Position.Y);
		Highest = FMath::Max(Highest, Unit.Position.Y);
	}
	TestTrue(TEXT("a wave from one point fans out sideways"), Highest - Lowest >= 150.0);

	for (int32 Index = 0; Index < 150; ++Index)
	{
		Sim.Step(0.1f, CrowdTest::Origin);
	}
	TestEqual(TEXT("every grunt still reaches the wizard"), CrowdTest::CountTouching(Sim, CrowdTest::Origin), 12);
	return true;
}

// Playtest of 2026-11-12 11:40 PST: skeletons shook back and forth when an enemy stood on the edge of the leash.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrowdArmyHoldsFightAtLeashEdge, "WebTesting.Army.HoldsFightAtLeashEdge", WIZ_TEST_FLAGS)
bool FCrowdArmyHoldsFightAtLeashEdge::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	Sim.Tuning.GruntSpeed = 0.0f;
	Sim.Tuning.GruntDamagePerSecond = 0.0f;
	Sim.AddSkeleton(FVector2D(340.0, 0.0));
	Sim.AddEnemy(ECrowdKind::Grunt, FVector2D(510.0, 0.0));

	// The wizard shifts a little every step, so the grunt is just inside the leash on one step and just outside on the next.
	int32 StepsBackward = 0;
	double LastX = 340.0;
	for (int32 Index = 0; Index < 80 && Sim.CountTeam(ECrowdTeam::Enemy) > 0; ++Index)
	{
		Sim.Step(0.05f, FVector2D(Index % 2 == 0 ? 15.0 : -15.0, 0.0));
		if (const FCrowdUnit* Skeleton = CrowdTest::First(Sim, ECrowdTeam::Army))
		{
			StepsBackward += Skeleton->Position.X < LastX - 0.5 ? 1 : 0;
			LastX = Skeleton->Position.X;
		}
	}
	TestEqual(TEXT("the skeleton never backed away from the grunt it had started on"), StepsBackward, 0);
	TestEqual(TEXT("the grunt on the edge of the leash was killed"), Sim.CountTeam(ECrowdTeam::Enemy), 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
