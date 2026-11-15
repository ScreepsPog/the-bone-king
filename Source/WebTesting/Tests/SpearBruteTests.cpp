#include "Sim/CrowdSim.h"
#include "Tests/CrowdTestHelpers.h"
#include "Tests/WizTestFlags.h"

#if WITH_DEV_AUTOMATION_TESTS

// Direction of 2026-11-13 (17:21 PST): the Spear of Sacrifice is paid for with bones when he has them, and only
// otherwise with plain skeletons (never another kind); enemies at a post do not stand in each other now that they are
// larger; a brute is armored at the front and weak at the back.

namespace SpearBruteTest
{
	void Run(FCrowdSim& Sim, float Seconds, FVector2D Wizard)
	{
		for (float Time = 0.0f; Time < Seconds; Time += 0.05f)
		{
			Sim.Step(0.05f, Wizard);
		}
	}

	int32 CountKind(const FCrowdSim& Sim, ECrowdKind Kind)
	{
		int32 Count = 0;
		for (const FCrowdUnit& Unit : Sim.GetUnits())
		{
			Count += Unit.Kind == Kind ? 1 : 0;
		}
		return Count;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpearBonesFirst, "WebTesting.Spear.BonesFirstThenPlainSkeletonsOnly", WIZ_TEST_FLAGS)
bool FSpearBonesFirst::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	// Changed 2026-11-14: the spear now costs one skeleton (Spear.CostsWhatOneSkeletonCosts). This test is about bones
	// paying first and only plain skeletons being taken, so it sets the old price of three.
	Sim.Tuning.SpearSkeletonCost = 3.0f;
	Sim.Tuning.SpearBoneCost = 3.0f;
	CrowdTest::FreezeArmy(Sim);
	Sim.Tuning.GuardSpeed = 0.0f;
	Sim.SetSpearUnlocked(true);
	for (int32 Index = 0; Index < 7; ++Index)
	{
		Sim.AddSkeleton(FVector2D(-100.0 - Index * 30.0, 0.0), Index < 5 ? ECrowdKind::Skeleton : ECrowdKind::Guard);
	}
	const int32 Cost = FMath::RoundToInt32(Sim.Tuning.SpearBoneCost);
	TestTrue(TEXT("the spear has a price in bones"), Cost >= 1);
	Sim.DropBone(CrowdTest::Origin, Cost + 2);
	Sim.Step(0.05f, CrowdTest::Origin);
	TestEqual(TEXT("he holds the price and 2 more"), Sim.GetBonesHeld(), Cost + 2);

	// With the bones for it, the bones pay and nobody is taken.
	TestTrue(TEXT("thrown"), Sim.ThrowSpear(CrowdTest::Origin, FVector2D(600.0, 0.0)));
	TestEqual(TEXT("the bones paid"), Sim.GetBonesHeld(), 2);
	TestEqual(TEXT("the army is whole"), Sim.CountTeam(ECrowdTeam::Army), 7);
	TestEqual(TEXT("nobody was cut down"), Sim.GetSlashes().Num(), 0);

	// Without them, three plain skeletons are cut down for it and the bones stay.
	SpearBruteTest::Run(Sim, Sim.Tuning.SpearCooldown + 0.2f, CrowdTest::Origin);
	TestTrue(TEXT("it can be thrown with 2 bones and five skeletons"), Sim.CanThrowSpear());
	TestTrue(TEXT("thrown"), Sim.ThrowSpear(CrowdTest::Origin, FVector2D(600.0, 0.0)));
	TestEqual(TEXT("the 2 bones stay"), Sim.GetBonesHeld(), 2);
	TestEqual(TEXT("three plain skeletons were taken"), SpearBruteTest::CountKind(Sim, ECrowdKind::Skeleton), 2);
	TestEqual(TEXT("both Guards are untouched"), SpearBruteTest::CountKind(Sim, ECrowdKind::Guard), 2);
	TestEqual(TEXT("three cuts are shown"), Sim.GetSlashes().Num(), 3);

	// Two skeletons and two Guards are not three skeletons: no other kind is ever taken.
	SpearBruteTest::Run(Sim, Sim.Tuning.SpearCooldown + 0.2f, CrowdTest::Origin);
	TestFalse(TEXT("two skeletons and two Guards cannot pay"), Sim.CanThrowSpear());
	TestFalse(TEXT("a throw is refused"), Sim.ThrowSpear(CrowdTest::Origin, FVector2D(600.0, 0.0)));
	TestEqual(TEXT("and nobody is taken"), Sim.CountTeam(ECrowdTeam::Army), 4);
	TestEqual(TEXT("the cuts have faded"), Sim.GetSlashes().Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPostSpacing, "WebTesting.Post.EnemiesDoNotStandInEachOther", WIZ_TEST_FLAGS)
bool FPostSpacing::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	const FVector2D Post(5000.0, 0.0);
	for (int32 Index = 0; Index < 14; ++Index)
	{
		// All put down within 60 units of the post, as a post does.
		const double Angle = Index * 2.4;
		Sim.AddGarrisonEnemy(Index < 12 ? ECrowdKind::Grunt : ECrowdKind::Brute, Post + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * (10.0 + Index * 3.5), 0);
	}
	TestTrue(TEXT("a grunt's body is wider than the old spacing allowed"), Sim.GetBodyRadius(ECrowdKind::Grunt) * 2.0f > 75.0f);
	TestTrue(TEXT("a brute takes more room than a grunt"), Sim.GetBodyRadius(ECrowdKind::Brute) > Sim.GetBodyRadius(ECrowdKind::Grunt) * 1.3f);
	SpearBruteTest::Run(Sim, 6.0f, CrowdTest::Origin);
	TestEqual(TEXT("the post is still at rest"), Sim.CountAsleep(), 14);
	// While they walk about the post for the next while: no two that stand are inside each other (a tenth of slack),
	// and two that brush past each other on the move never overlap by more than a quarter.
	int32 Overlaps = 0;
	int32 DeepOverlaps = 0;
	for (int32 Step = 0; Step < 120; ++Step)
	{
		Sim.Step(0.05f, CrowdTest::Origin);
		const TArray<FCrowdUnit>& Units = Sim.GetUnits();
		for (int32 Index = 0; Index < Units.Num(); ++Index)
		{
			for (int32 Other = Index + 1; Other < Units.Num(); ++Other)
			{
				const double Room = Sim.GetBodyRadius(Units[Index].Kind) + Sim.GetBodyRadius(Units[Other].Kind);
				const double Gap = FVector2D::Distance(Units[Index].Position, Units[Other].Position);
				Overlaps += Gap < Room * 0.9 && !Units[Index].bMoving && !Units[Other].bMoving ? 1 : 0;
				DeepOverlaps += Gap < Room * 0.75 ? 1 : 0;
			}
		}
	}
	TestEqual(TEXT("no two stood in each other in 6 seconds"), Overlaps, 0);
	TestEqual(TEXT("and none walked through another"), DeepOverlaps, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBruteWeakBack, "WebTesting.Brute.ArmoredInFrontWeakBehind", WIZ_TEST_FLAGS)
bool FBruteWeakBack::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	CrowdTest::MakeEnemiesPlain(Sim);
	// It walks slowly at the wizard, who stands at the origin: it faces him.
	Sim.Tuning.BruteSpeed = 20.0f;
	Sim.AddEnemy(ECrowdKind::Brute, FVector2D(600.0, 0.0));
	SpearBruteTest::Run(Sim, 0.5f, CrowdTest::Origin);
	const auto Health = [&Sim]() { return Sim.GetUnits()[0].Health; };
	const FVector2D Where = Sim.GetUnits()[0].Position;
	TestTrue(TEXT("it faces the wizard"), Sim.GetUnits()[0].Facing.X < -0.9);
	TestEqual(TEXT("it starts whole"), Health(), 80.0f, 0.01f);

	// A Bolt does 10.
	Sim.FireBolt(Where + FVector2D(-300.0, 0.0), FVector2D(1.0, 0.0));
	SpearBruteTest::Run(Sim, 0.4f, CrowdTest::Origin);
	TestEqual(TEXT("a Bolt in its front does half: 5"), Health(), 75.0f, 0.01f);
	Sim.FireBolt(Where + FVector2D(0.0, 300.0), FVector2D(0.0, -1.0));
	SpearBruteTest::Run(Sim, 0.4f, CrowdTest::Origin);
	TestEqual(TEXT("a Bolt in its side does its 10"), Health(), 65.0f, 0.01f);
	Sim.FireBolt(Where + FVector2D(300.0, 0.0), FVector2D(-1.0, 0.0));
	SpearBruteTest::Run(Sim, 0.4f, CrowdTest::Origin);
	TestEqual(TEXT("a Bolt in its back does double: 20"), Health(), 45.0f, 0.01f);

	TestEqual(TEXT("a grunt takes the same from every side"), Sim.GetDamageTaken(FCrowdUnit(), FVector2D(100.0, 0.0)), 1.0f);

	// It does not spin on the spot: a wizard who gets behind it has a moment at its back.
	FCrowdSim Turn;
	CrowdTest::MakeEnemiesPlain(Turn);
	Turn.Tuning.BruteSpeed = 20.0f;
	Turn.AddEnemy(ECrowdKind::Brute, FVector2D(600.0, 0.0));
	SpearBruteTest::Run(Turn, 0.5f, CrowdTest::Origin);
	const FVector2D Behind(1200.0, 0.0);
	SpearBruteTest::Run(Turn, 0.3f, Behind);
	TestTrue(TEXT("0.3 seconds after he is behind it, it still mostly faces the old way"), Turn.GetUnits()[0].Facing.X < 0.0);
	SpearBruteTest::Run(Turn, 2.0f, Behind);
	TestTrue(TEXT("two seconds later it has turned to him"), Turn.GetUnits()[0].Facing.X > 0.9);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrikeClock, "WebTesting.Melee.TheSwingCanBeTimedToTheBlow", WIZ_TEST_FLAGS)
bool FStrikeClock::RunTest(const FString& Parameters)
{
	// What the picture needs to line a swing up with its blow: who is striking, when its next blow lands, and a clock.
	FCrowdSim Sim;
	CrowdTest::MakeEnemiesPlain(Sim);
	CrowdTest::FreezeArmy(Sim);
	Sim.Tuning.GruntSpeed = 0.0f;
	Sim.AddSkeleton(FVector2D(3000.0, 0.0));
	Sim.AddEnemy(ECrowdKind::Grunt, FVector2D(3040.0, 0.0));
	Sim.AddEnemy(ECrowdKind::Grunt, FVector2D(6000.0, 0.0));
	Sim.Command(FVector2D(3000.0, 0.0), 1, ECrowdCommandKind::Any);
	SpearBruteTest::Run(Sim, 0.2f, FVector2D(2700.0, 0.0));
	TestEqual(TEXT("the clock has run 0.2 seconds"), Sim.GetClock(), 0.2, 0.001);
	TestTrue(TEXT("the skeleton in reach of a grunt is striking"), Sim.GetUnits()[0].bStriking);
	TestTrue(TEXT("so is the grunt"), Sim.GetUnits()[1].bStriking);
	TestFalse(TEXT("a grunt with nobody in reach is not"), Sim.GetUnits()[2].bStriking);
	TestEqual(TEXT("its first blow is 0.3 seconds off"), Sim.GetUnits()[0].AttackTimer, 0.3f, 0.01f);
	TestTrue(TEXT("the skeleton faces the grunt"), Sim.GetUnits()[0].Facing.X > 0.9);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
