#include "Game/CrowdSubsystem.h"
#include "Tests/CrowdTestHelpers.h"
#include "Tests/WizTestFlags.h"

#if WITH_DEV_AUTOMATION_TESTS

// Direction of 2026-11-12 14:55 PST:
// - the bone spear costs skeletons, flies to where the wizard aims, and explodes there in an area. It wipes out weak
//   enemies and is weak against tough ones;
// - the Brute is the first tough enemy, and its bones assemble a stronger skeleton, the Guard.

namespace CrowdSpearTest
{
	/** Steps until no spear is in flight (at most 3 seconds). Returns how many spears exploded. */
	int32 FlyOut(FCrowdSim& Sim, FVector2D Wizard = CrowdTest::Origin)
	{
		int32 Exploded = 0;
		for (int32 Index = 0; Index < 60; ++Index)
		{
			Exploded += Sim.Step(0.05f, Wizard).SpearsExploded;
			if (Sim.GetSpears().Num() == 0)
			{
				break;
			}
		}
		return Exploded;
	}

	// Changed 2026-11-12: the spear is bought at the shrine now, so these tests unlock it first.
	void AddArmy(FCrowdSim& Sim, int32 Count)
	{
		Sim.SetSpearUnlocked(true);
		for (int32 Index = 0; Index < Count; ++Index)
		{
			Sim.AddSkeleton(FVector2D(-100.0 - Index * 40.0, 0.0));
		}
	}

	const FCrowdUnit* FirstOfKind(const FCrowdSim& Sim, ECrowdKind Kind)
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrowdSpearCostsSkeletons, "WebTesting.Spear.CostsSkeletons", WIZ_TEST_FLAGS)
bool FCrowdSpearCostsSkeletons::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	// Changed 2026-11-14: the spear now costs one skeleton (Spear.CostsWhatOneSkeletonCosts). This test is about being
	// refused with too few and paying with enough, so it sets the old price of three.
	Sim.Tuning.SpearSkeletonCost = 3.0f;
	Sim.Tuning.SpearBoneCost = 3.0f;
	CrowdSpearTest::AddArmy(Sim, 2);
	TestFalse(TEXT("two skeletons are not enough"), Sim.CanThrowSpear());
	TestFalse(TEXT("so the throw is refused"), Sim.ThrowSpear(CrowdTest::Origin, FVector2D(600.0, 0.0)));
	TestEqual(TEXT("and nothing is spent"), Sim.CountTeam(ECrowdTeam::Army), 2);
	TestEqual(TEXT("and no spear flies"), Sim.GetSpears().Num(), 0);

	CrowdSpearTest::AddArmy(Sim, 3);
	TestTrue(TEXT("five skeletons are enough"), Sim.CanThrowSpear());
	TestTrue(TEXT("the throw goes"), Sim.ThrowSpear(CrowdTest::Origin, FVector2D(600.0, 0.0)));
	TestEqual(TEXT("three skeletons are spent"), Sim.CountTeam(ECrowdTeam::Army), 2);
	TestEqual(TEXT("one spear flies"), Sim.GetSpears().Num(), 1);

	// The five stood at x = -100, -140, -100, -140, -180. The three nearest the wizard were taken, so -140 and -180 are left.
	for (const FCrowdUnit& Unit : Sim.GetUnits())
	{
		TestTrue(TEXT("the skeletons nearest the wizard were the ones taken"), Unit.Position.X <= -135.0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrowdSpearExplodesInArea, "WebTesting.Spear.ExplodesInAnArea", WIZ_TEST_FLAGS)
bool FCrowdSpearExplodesInArea::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	Sim.Tuning.GruntSpeed = 0.0f;
	Sim.Tuning.SkeletonSpeed = 0.0f; // the army stays out of it
	CrowdSpearTest::AddArmy(Sim, 3);
	for (int32 Index = 0; Index < 6; ++Index)
	{
		const double Angle = UE_DOUBLE_TWO_PI * Index / 6.0;
		Sim.AddEnemy(ECrowdKind::Grunt, FVector2D(700.0 + FMath::Cos(Angle) * 180.0, FMath::Sin(Angle) * 180.0));
	}
	// One grunt outside the blast.
	Sim.AddEnemy(ECrowdKind::Grunt, FVector2D(700.0, Sim.Tuning.SpearRadius + 80.0));

	TestTrue(TEXT("thrown"), Sim.ThrowSpear(CrowdTest::Origin, FVector2D(700.0, 0.0)));
	Sim.Step(0.05f, CrowdTest::Origin);
	TestEqual(TEXT("nothing dies while the spear is in the air"), Sim.CountTeam(ECrowdTeam::Enemy), 7);

	TestEqual(TEXT("the spear explodes once"), CrowdSpearTest::FlyOut(Sim), 1);
	TestEqual(TEXT("the six grunts in the blast die, the one outside lives"), Sim.CountTeam(ECrowdTeam::Enemy), 1);
	const FCrowdUnit* Left = CrowdTest::First(Sim, ECrowdTeam::Enemy);
	if (TestNotNull(TEXT("the grunt outside"), Left))
	{
		TestEqual(TEXT("is unhurt"), Left->Health, Sim.Tuning.GruntHealth);
	}
	TestEqual(TEXT("each dead grunt left a bone"), Sim.GetBones().Num(), 6);
	if (TestEqual(TEXT("the blast is there to be drawn"), Sim.GetBlasts().Num(), 1))
	{
		TestTrue(TEXT("where the wizard aimed"), FVector2D::Distance(Sim.GetBlasts()[0].Position, FVector2D(700.0, 0.0)) <= 5.0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrowdSpearWeakAgainstBrute, "WebTesting.Spear.WeakAgainstABrute", WIZ_TEST_FLAGS)
bool FCrowdSpearWeakAgainstBrute::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	Sim.Tuning.BruteSpeed = 0.0f;
	Sim.Tuning.SkeletonSpeed = 0.0f;
	CrowdSpearTest::AddArmy(Sim, 3);
	Sim.AddEnemy(ECrowdKind::Brute, FVector2D(700.0, 0.0));
	Sim.ThrowSpear(CrowdTest::Origin, FVector2D(700.0, 0.0));
	CrowdSpearTest::FlyOut(Sim);

	const FCrowdUnit* Brute = CrowdSpearTest::FirstOfKind(Sim, ECrowdKind::Brute);
	if (TestNotNull(TEXT("the brute lives through a spear"), Brute))
	{
		TestEqual(TEXT("it lost the spear's damage"), Brute->Health, Sim.Tuning.BruteHealth - Sim.Tuning.SpearDamage);
		TestTrue(TEXT("it keeps more than half its health"), Brute->Health > Sim.Tuning.BruteHealth * 0.5f);
		TestTrue(TEXT("its health bar shows"), Brute->HurtTimer > 0.0f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrowdSpearStopsAtWall, "WebTesting.Spear.StopsAtAWall", WIZ_TEST_FLAGS)
bool FCrowdSpearStopsAtWall::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	Sim.Tuning.GruntSpeed = 0.0f;
	Sim.Tuning.SkeletonSpeed = 0.0f;
	Sim.Tuning.SpearRadius = 150.0f;
	Sim.SetObstacles({ CrowdTest::Wall(400.0, 0.0, 50.0, 300.0) });
	CrowdSpearTest::AddArmy(Sim, 3);
	// Behind the wall, 350 past its near face: out of the blast when the spear stops at the wall.
	Sim.AddEnemy(ECrowdKind::Grunt, FVector2D(700.0, 0.0));
	Sim.ThrowSpear(CrowdTest::Origin, FVector2D(700.0, 0.0));
	TestEqual(TEXT("it explodes"), CrowdSpearTest::FlyOut(Sim), 1);
	if (TestEqual(TEXT("one blast"), Sim.GetBlasts().Num(), 1))
	{
		TestTrue(TEXT("on the near face of the wall"), Sim.GetBlasts()[0].Position.X <= 350.0 && Sim.GetBlasts()[0].Position.X >= 300.0);
	}
	TestEqual(TEXT("the grunt behind the wall lives"), Sim.CountTeam(ECrowdTeam::Enemy), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrowdSpearCooldownAndRange, "WebTesting.Spear.CooldownAndRange", WIZ_TEST_FLAGS)
bool FCrowdSpearCooldownAndRange::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	CrowdSpearTest::AddArmy(Sim, 9);
	TestTrue(TEXT("first throw"), Sim.ThrowSpear(CrowdTest::Origin, FVector2D(5000.0, 0.0)));
	TestFalse(TEXT("a second throw at once is refused"), Sim.ThrowSpear(CrowdTest::Origin, FVector2D(600.0, 0.0)));
	// Changed 2026-11-14: the spear costs one skeleton now (it was three), so 8 of the 9 are left after the first throw.
	TestEqual(TEXT("and costs nothing"), Sim.CountTeam(ECrowdTeam::Army), 9 - FMath::RoundToInt32(Sim.Tuning.SpearSkeletonCost));

	CrowdSpearTest::FlyOut(Sim);
	if (TestEqual(TEXT("one blast"), Sim.GetBlasts().Num(), 1))
	{
		TestTrue(TEXT("a far aim is cut to the spear's range"), FMath::IsNearlyEqual(Sim.GetBlasts()[0].Position.X, static_cast<double>(Sim.Tuning.SpearRange), 5.0));
	}

	for (int32 Index = 0; Index < 30; ++Index)
	{
		Sim.Step(0.05f, CrowdTest::Origin);
	}
	TestEqual(TEXT("the blast is gone a moment later"), Sim.GetBlasts().Num(), 0);
	TestTrue(TEXT("after the cooldown it can be thrown again"), Sim.ThrowSpear(CrowdTest::Origin, FVector2D(600.0, 0.0)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrowdBruteIsTough, "WebTesting.Brute.TakesManyBolts", WIZ_TEST_FLAGS)
bool FCrowdBruteIsTough::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	Sim.Tuning.BruteSpeed = 0.0f;
	Sim.AddEnemy(ECrowdKind::Brute, FVector2D(300.0, 0.0));
	const FCrowdUnit* Brute = CrowdSpearTest::FirstOfKind(Sim, ECrowdKind::Brute);
	if (!TestNotNull(TEXT("a brute"), Brute))
	{
		return true;
	}
	TestEqual(TEXT("with a brute's health"), Brute->Health, Sim.Tuning.BruteHealth);
	TestTrue(TEXT("which is at least five grunts' worth"), Sim.Tuning.BruteHealth >= Sim.Tuning.GruntHealth * 5.0f);

	int32 Bolts = 0;
	while (Sim.CountTeam(ECrowdTeam::Enemy) > 0 && Bolts < 50)
	{
		Sim.FireBolt(CrowdTest::Origin, FVector2D(1.0, 0.0));
		++Bolts;
		for (int32 Index = 0; Index < 5; ++Index)
		{
			Sim.Step(0.05f, FVector2D(-2000.0, 0.0));
		}
	}
	TestEqual(TEXT("bolts to kill a brute"), Bolts, FMath::CeilToInt32(Sim.Tuning.BruteHealth / Sim.Tuning.BoltDamage));
	return true;
}

// Changed 2026-11-12 15:25 PST: a brute now leaves 3 plain bones (see Bones.BruteLeavesThree). How a Guard is made
// is being rethought; until then a Guard pile can only be dropped directly, and this keeps that path working.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrowdGuardPileMakesGuard, "WebTesting.Bones.GuardPileAssemblesAGuard", WIZ_TEST_FLAGS)
bool FCrowdGuardPileMakesGuard::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	TestTrue(TEXT("a guard pile is dropped"), Sim.DropBone(FVector2D(300.0, 0.0), 1, true));
	Sim.Step(0.05f, FVector2D(-2000.0, 0.0));
	TestEqual(TEXT("no guard until the wizard walks there"), Sim.CountTeam(ECrowdTeam::Army), 0);

	const FCrowdStepResult Result = Sim.Step(0.05f, FVector2D(300.0, 0.0));
	TestEqual(TEXT("collecting it assembles one unit"), Sim.CountTeam(ECrowdTeam::Army), 1);
	TestEqual(TEXT("the step reports it"), Result.SkeletonsRaised, 1);
	const FCrowdUnit* Guard = CrowdSpearTest::FirstOfKind(Sim, ECrowdKind::Guard);
	if (TestNotNull(TEXT("and it is a guard"), Guard))
	{
		TestEqual(TEXT("with a guard's health"), Guard->Health, Sim.Tuning.GuardHealth);
		TestTrue(TEXT("tougher than a skeleton"), Sim.Tuning.GuardHealth > Sim.Tuning.SkeletonHealth);
	}
	TestEqual(TEXT("a guard pile does not count as plain bones"), Sim.GetBonesHeld(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrowdSpearSparesGuards, "WebTesting.Spear.TakesPlainSkeletonsFirst", WIZ_TEST_FLAGS)
bool FCrowdSpearSparesGuards::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	// Changed 2026-11-14: the spear costs one skeleton now. This test is about who is taken when three are, so it sets
	// the old price of three.
	Sim.Tuning.SpearSkeletonCost = 3.0f;
	Sim.Tuning.SpearBoneCost = 3.0f;
	// The guard stands nearest the wizard, three plain skeletons farther out.
	Sim.AddSkeleton(FVector2D(-60.0, 0.0), ECrowdKind::Guard);
	CrowdSpearTest::AddArmy(Sim, 3);
	TestTrue(TEXT("thrown"), Sim.ThrowSpear(CrowdTest::Origin, FVector2D(600.0, 0.0)));
	TestEqual(TEXT("one unit is left"), Sim.CountTeam(ECrowdTeam::Army), 1);
	TestNotNull(TEXT("and it is the guard"), CrowdSpearTest::FirstOfKind(Sim, ECrowdKind::Guard));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardWavesBrutesComeLater, "WebTesting.Waves.BrutesComeLater", WIZ_TEST_FLAGS)
bool FWizardWavesBrutesComeLater::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("no brute in the first wave"), UCrowdSubsystem::WaveBrutes(0.0f), 0);
	TestEqual(TEXT("none just before 45 seconds"), UCrowdSubsystem::WaveBrutes(44.0f), 0);
	TestEqual(TEXT("one from 45 seconds"), UCrowdSubsystem::WaveBrutes(45.0f), 1);
	TestEqual(TEXT("two from 105 seconds"), UCrowdSubsystem::WaveBrutes(105.0f), 2);
	TestEqual(TEXT("never more than four a wave"), UCrowdSubsystem::WaveBrutes(3600.0f), 4);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
