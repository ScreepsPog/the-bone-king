#include "Sim/CrowdSim.h"
#include "Tests/WizTestFlags.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace CrowdSimTest
{
	const FVector2D Origin(0.0, 0.0);

	FCrowdStepResult StepMany(FCrowdSim& Sim, float DeltaSeconds, int32 Count, FVector2D Wizard = Origin)
	{
		FCrowdStepResult Total;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FCrowdStepResult Result = Sim.Step(DeltaSeconds, Wizard);
			Total.EnemiesKilled += Result.EnemiesKilled;
			Total.SkeletonsRaised += Result.SkeletonsRaised;
			Total.SkeletonsLost += Result.SkeletonsLost;
		}
		return Total;
	}

	const FCrowdUnit* FindFirst(const FCrowdSim& Sim, ECrowdTeam Team)
	{
		for (const FCrowdUnit& Unit : Sim.GetUnits())
		{
			if (Unit.Team == Team)
			{
				return &Unit;
			}
		}
		return nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrowdSimBoltKillDropsBone, "WebTesting.Sim.BoltKillDropsBone", WIZ_TEST_FLAGS)
bool FCrowdSimBoltKillDropsBone::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	Sim.Tuning.GruntSpeed = 0.0f; // the grunt stands still so the kill position is exact
	Sim.AddEnemy(ECrowdKind::Grunt, FVector2D(500.0, 0.0));
	Sim.FireBolt(CrowdSimTest::Origin, FVector2D(1.0, 0.0));
	TestEqual(TEXT("one bolt in flight"), Sim.GetBolts().Num(), 1);

	FCrowdStepResult Total;
	for (int32 Index = 0; Index < 40 && Sim.GetBolts().Num() > 0; ++Index)
	{
		const FCrowdStepResult Result = Sim.Step(0.05f, CrowdSimTest::Origin);
		Total.EnemiesKilled += Result.EnemiesKilled;
		Total.SkeletonsRaised += Result.SkeletonsRaised;
	}

	TestEqual(TEXT("enemies left"), Sim.CountTeam(ECrowdTeam::Enemy), 0);
	// Direction of 2026-11-12 12:15 PST: a kill no longer raises a skeleton by itself. It leaves bones to collect.
	TestEqual(TEXT("no skeleton rises from the kill"), Sim.CountTeam(ECrowdTeam::Army), 0);
	TestEqual(TEXT("enemies killed"), Total.EnemiesKilled, 1);
	TestEqual(TEXT("skeletons raised"), Total.SkeletonsRaised, 0);
	if (TestEqual(TEXT("one pile of bones on the ground"), Sim.GetBones().Num(), 1))
	{
		TestTrue(TEXT("the bones lie where the grunt fell"), FVector2D::Distance(Sim.GetBones()[0].Position, FVector2D(500.0, 0.0)) <= 1.0);
		TestEqual(TEXT("a grunt leaves one bone"), Sim.GetBones()[0].Value, 1);
	}
	TestEqual(TEXT("the wizard, far away, holds none"), Sim.GetBonesHeld(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrowdSimSkeletonKillsGrunt, "WebTesting.Sim.SkeletonKillsGrunt", WIZ_TEST_FLAGS)
bool FCrowdSimSkeletonKillsGrunt::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	Sim.AddSkeleton(FVector2D(0.0, 0.0));
	Sim.AddEnemy(ECrowdKind::Grunt, FVector2D(60.0, 0.0));
	CrowdSimTest::StepMany(Sim, 0.1f, 20);
	TestEqual(TEXT("enemies left"), Sim.CountTeam(ECrowdTeam::Enemy), 0);
	TestEqual(TEXT("army size"), Sim.CountTeam(ECrowdTeam::Army), 1);
	TestEqual(TEXT("the grunt left bones, on the ground or collected"), Sim.GetBones().Num() + Sim.GetBonesHeld(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrowdSimGruntWalksToWizard, "WebTesting.Sim.GruntWalksToWizard", WIZ_TEST_FLAGS)
bool FCrowdSimGruntWalksToWizard::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	// No bend and no speed share, so the line and the distance are exact.
	Sim.Tuning.EnemyFlankDegrees = 0.0f;
	Sim.Tuning.EnemySpeedVariation = 0.0f;
	Sim.AddEnemy(ECrowdKind::Grunt, FVector2D(1000.0, 0.0));
	CrowdSimTest::StepMany(Sim, 0.1f, 10);
	const FCrowdUnit* Grunt = CrowdSimTest::FindFirst(Sim, ECrowdTeam::Enemy);
	if (TestNotNull(TEXT("grunt exists"), Grunt))
	{
		TestTrue(TEXT("grunt walked 300 units toward the wizard"), FMath::Abs(Grunt->Position.X - 700.0) <= 5.0);
		TestTrue(TEXT("grunt stayed on the line"), FMath::Abs(Grunt->Position.Y) <= 1.0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrowdSimSkeletonReturnsToWizard, "WebTesting.Sim.SkeletonReturnsToWizard", WIZ_TEST_FLAGS)
bool FCrowdSimSkeletonReturnsToWizard::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	Sim.AddSkeleton(FVector2D(2000.0, 0.0));
	CrowdSimTest::StepMany(Sim, 0.1f, 60);
	const FCrowdUnit* Skeleton = CrowdSimTest::FindFirst(Sim, ECrowdTeam::Army);
	if (TestNotNull(TEXT("skeleton exists"), Skeleton))
	{
		TestTrue(TEXT("skeleton is back inside the roam circle"), Skeleton->Position.Size() <= Sim.Tuning.RoamRadius + 10.0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrowdSimEnemyCap, "WebTesting.Sim.EnemyCap", WIZ_TEST_FLAGS)
bool FCrowdSimEnemyCap::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	int32 Accepted = 0;
	// The cap was 120; it is 320 since the Outskirts were given more enemies (2026-11-12).
	const int32 Cap = Sim.Tuning.MaxEnemies;
	TestEqual(TEXT("the cap"), Cap, 320);
	for (int32 Index = 0; Index < Cap; ++Index)
	{
		Accepted += Sim.AddEnemy(ECrowdKind::Grunt, FVector2D(1000.0 + Index * 60.0, 0.0)) ? 1 : 0;
	}
	TestEqual(TEXT("every enemy up to the cap is accepted"), Accepted, Cap);
	TestFalse(TEXT("one more is refused"), Sim.AddEnemy(ECrowdKind::Grunt, FVector2D(0.0, 5000.0)));
	TestEqual(TEXT("enemy count"), Sim.CountTeam(ECrowdTeam::Enemy), Cap);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrowdSimSkeletonCapOnRaise, "WebTesting.Sim.SkeletonCapOnRaise", WIZ_TEST_FLAGS)
bool FCrowdSimSkeletonCapOnRaise::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	for (int32 Index = 0; Index < 40; ++Index)
	{
		TestTrue(TEXT("skeleton accepted"), Sim.AddSkeleton(FVector2D(-200.0, 0.0)));
	}
	TestFalse(TEXT("the 41st skeleton is refused"), Sim.AddSkeleton(FVector2D(-200.0, 0.0)));
	Sim.AddEnemy(ECrowdKind::Grunt, FVector2D(500.0, 0.0));
	Sim.FireBolt(CrowdSimTest::Origin, FVector2D(1.0, 0.0));

	FCrowdStepResult Total;
	for (int32 Index = 0; Index < 40 && Sim.CountTeam(ECrowdTeam::Enemy) > 0; ++Index)
	{
		const FCrowdStepResult Result = Sim.Step(0.05f, CrowdSimTest::Origin);
		Total.EnemiesKilled += Result.EnemiesKilled;
		Total.SkeletonsRaised += Result.SkeletonsRaised;
	}
	TestEqual(TEXT("the grunt died"), Total.EnemiesKilled, 1);
	TestEqual(TEXT("no skeleton raised past the cap"), Total.SkeletonsRaised, 0);
	TestEqual(TEXT("army stays at the cap"), Sim.CountTeam(ECrowdTeam::Army), 40);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrowdSimClampsLongFrame, "WebTesting.Sim.ClampsLongFrame", WIZ_TEST_FLAGS)
bool FCrowdSimClampsLongFrame::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	// No bend and no speed share, so one clamped step is exactly 30 units.
	Sim.Tuning.EnemyFlankDegrees = 0.0f;
	Sim.Tuning.EnemySpeedVariation = 0.0f;
	Sim.AddEnemy(ECrowdKind::Grunt, FVector2D(1000.0, 0.0));
	Sim.Step(5.0f, CrowdSimTest::Origin);
	const FCrowdUnit* Grunt = CrowdSimTest::FindFirst(Sim, ECrowdTeam::Enemy);
	if (TestNotNull(TEXT("grunt exists"), Grunt))
	{
		TestTrue(TEXT("a 5 second frame moves the grunt at most one clamped step"), Grunt->Position.X >= 969.0);
		TestTrue(TEXT("the grunt still moved"), Grunt->Position.X < 1000.0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrowdSimEmptyStep, "WebTesting.Sim.EmptyStep", WIZ_TEST_FLAGS)
bool FCrowdSimEmptyStep::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	const FCrowdStepResult Result = Sim.Step(0.016f, CrowdSimTest::Origin);
	TestEqual(TEXT("killed"), Result.EnemiesKilled, 0);
	TestEqual(TEXT("raised"), Result.SkeletonsRaised, 0);
	TestEqual(TEXT("lost"), Result.SkeletonsLost, 0);
	TestEqual(TEXT("units"), Sim.GetUnits().Num(), 0);
	TArray<FVector2D> Offsets;
	Sim.GetEnemiesTouching(CrowdSimTest::Origin, Offsets);
	TestEqual(TEXT("touching"), Offsets.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrowdSimTouchingOffsets, "WebTesting.Sim.TouchingOffsets", WIZ_TEST_FLAGS)
bool FCrowdSimTouchingOffsets::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	Sim.AddEnemy(ECrowdKind::Grunt, FVector2D(50.0, 0.0));
	Sim.AddEnemy(ECrowdKind::Grunt, FVector2D(500.0, 0.0));
	TArray<FVector2D> Offsets;
	Sim.GetEnemiesTouching(CrowdSimTest::Origin, Offsets);
	if (TestEqual(TEXT("one enemy touching"), Offsets.Num(), 1))
	{
		TestTrue(TEXT("offset is from the wizard to the enemy"), Offsets[0].Equals(FVector2D(50.0, 0.0), 0.01));
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
