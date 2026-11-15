#include "Sim/CrowdSim.h"
#include "Tests/CrowdTestHelpers.h"
#include "Tests/WizTestFlags.h"

#if WITH_DEV_AUTOMATION_TESTS

// Direction of 2026-11-13 (21:53 PST):
// - the brute's attack is a slam in front of it: a red circle grows from tiny to the size of the area, and 0.3 seconds
//   after it is full the slam lands and does big damage to everything of the wizard's inside it;
// - every melee unit stands still while it attacks, and does not turn;
// - a skeleton costs 2 bones to summon and the spear 3 bones to throw.

namespace SlamTest
{
	const float StepSeconds = 0.05f;

	struct FTotals
	{
		int32 Landed = 0;
		int32 HitWizard = 0;
	};

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

	void Run(FCrowdSim& Sim, float Seconds, FVector2D Wizard, FTotals& Totals)
	{
		for (float Time = 0.0f; Time < Seconds - 0.001f; Time += StepSeconds)
		{
			const FCrowdStepResult Result = Sim.Step(StepSeconds, Wizard);
			Totals.Landed += Result.SlamsLanded;
			Totals.HitWizard += Result.SlamsHitWizard;
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBruteSlamGrowsThenLands, "WebTesting.Brute.TheSlamGrowsThenLands", WIZ_TEST_FLAGS)
bool FBruteSlamGrowsThenLands::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	CrowdTest::MakeEnemiesPlain(Sim);
	CrowdTest::FreezeArmy(Sim);
	Sim.Tuning.BruteSpeed = 0.0f;
	Sim.Tuning.SkeletonHealth = 100.0f;
	Sim.Tuning.SlamFlashSeconds = 5.0f;
	const FVector2D Wizard(200.0, 0.0);
	Sim.AddEnemy(ECrowdKind::Brute, CrowdTest::Origin);
	// One skeleton in front of the brute, beside the wizard; one behind the brute.
	Sim.AddSkeleton(FVector2D(250.0, 60.0));
	Sim.AddSkeleton(FVector2D(-150.0, 0.0));
	SlamTest::FTotals Totals;

	TestTrue(TEXT("the slam does big damage: at least twice what a brute's blow did"),
		Sim.Tuning.BruteSlamDamage >= 2.0f * Sim.Tuning.BruteDamagePerSecond * Sim.Tuning.MeleeInterval);
	TestEqual(TEXT("it lands 0.3 seconds after the circle is full"), Sim.Tuning.BruteSlamHold, 0.3f);

	// It winds up for the first half second, as every melee unit does, then the circle starts.
	SlamTest::Run(Sim, Sim.Tuning.MeleeInterval * Sim.Tuning.MeleeFirstShare + 0.2f, Wizard, Totals);
	const FCrowdUnit* Brute = SlamTest::FirstOfKind(Sim, ECrowdKind::Brute);
	if (!TestNotNull(TEXT("the brute"), Brute))
	{
		return false;
	}
	TestTrue(TEXT("with the wizard in front of it, the brute starts a slam"), Sim.IsSlamming(*Brute));
	const float Early = Sim.GetSlamShare(*Brute);
	TestTrue(TEXT("the circle starts tiny"), Early > 0.0f && Early < 0.4f);
	TestTrue(TEXT("the circle is in front of the brute, where the wizard stands"),
		FVector2D::Distance(Brute->SlamCenter, Wizard) < Sim.Tuning.BruteSlamRadius && Brute->SlamCenter.X > 50.0);

	SlamTest::Run(Sim, Sim.Tuning.BruteSlamGrow * 0.4f, Wizard, Totals);
	Brute = SlamTest::FirstOfKind(Sim, ECrowdKind::Brute);
	const float Later = Sim.GetSlamShare(*Brute);
	TestTrue(TEXT("the circle grows"), Later > Early + 0.2f && Later < 1.0f);
	TestEqual(TEXT("nothing has landed while it grows"), Totals.Landed, 0);

	// Full size, and the 0.3 seconds have not passed.
	SlamTest::Run(Sim, Sim.Tuning.BruteSlamGrow * 0.6f - 0.15f + 0.1f, Wizard, Totals);
	Brute = SlamTest::FirstOfKind(Sim, ECrowdKind::Brute);
	TestEqual(TEXT("the circle is at the size of the area"), Sim.GetSlamShare(*Brute), 1.0f);
	TestTrue(TEXT("and the brute is still slamming"), Sim.IsSlamming(*Brute));
	TestEqual(TEXT("nothing has landed before the 0.3 seconds are up"), Totals.Landed, 0);
	TestEqual(TEXT("nobody is hurt yet"), Sim.GetUnits()[1].Health, 100.0f);

	SlamTest::Run(Sim, Sim.Tuning.BruteSlamHold + 0.15f, Wizard, Totals);
	TestEqual(TEXT("the slam lands once"), Totals.Landed, 1);
	TestEqual(TEXT("it hits the wizard, who stood in the circle"), Totals.HitWizard, 1);
	const FCrowdUnit* Front = nullptr;
	const FCrowdUnit* Behind = nullptr;
	for (const FCrowdUnit& Unit : Sim.GetUnits())
	{
		if (Unit.Team == ECrowdTeam::Army)
		{
			(Unit.Position.X > 0.0 ? Front : Behind) = &Unit;
		}
	}
	if (TestNotNull(TEXT("the skeleton in the circle"), Front) && TestNotNull(TEXT("the skeleton behind the brute"), Behind))
	{
		TestEqual(TEXT("the skeleton in the circle takes the slam's damage"), Front->Health, 100.0f - Sim.Tuning.BruteSlamDamage);
		TestEqual(TEXT("the skeleton behind the brute is untouched"), Behind->Health, 100.0f);
	}
	Brute = SlamTest::FirstOfKind(Sim, ECrowdKind::Brute);
	TestFalse(TEXT("the brute rests before its next slam"), Sim.IsSlamming(*Brute));
	TestEqual(TEXT("the flash of the slam is kept for the picture"), Sim.GetSlamHits().Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBruteStandsThroughSlam, "WebTesting.Brute.StandsAndDoesNotTurnThroughASlam", WIZ_TEST_FLAGS)
bool FBruteStandsThroughSlam::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	CrowdTest::MakeEnemiesPlain(Sim);
	Sim.AddEnemy(ECrowdKind::Brute, CrowdTest::Origin);
	FVector2D Wizard(600.0, 0.0);
	SlamTest::FTotals Totals;

	// It walks at the wizard and starts its slam when he is in front of it.
	const FCrowdUnit* Brute = SlamTest::FirstOfKind(Sim, ECrowdKind::Brute);
	for (int32 Step = 0; Step < 100 && !Sim.IsSlamming(*Brute); ++Step)
	{
		SlamTest::Run(Sim, SlamTest::StepSeconds, Wizard, Totals);
		Brute = SlamTest::FirstOfKind(Sim, ECrowdKind::Brute);
	}
	if (!TestTrue(TEXT("the brute walks up and starts a slam"), Sim.IsSlamming(*Brute)))
	{
		return false;
	}
	const FVector2D Stood = Brute->Position;
	const FVector2D Faced = Brute->Facing;
	TestTrue(TEXT("it walked toward him first"), Stood.X > 100.0);
	TestTrue(TEXT("it faces him"), Faced.X > 0.99);

	// The wizard gets behind it. Through the whole slam the brute neither walks nor turns.
	Wizard = Stood + FVector2D(-300.0, 0.0);
	bool bStood = true;
	bool bHeld = true;
	const int32 Steps = FMath::FloorToInt32((Sim.Tuning.BruteSlamGrow + Sim.Tuning.BruteSlamHold) / SlamTest::StepSeconds) - 3;
	for (int32 Step = 0; Step < Steps; ++Step)
	{
		SlamTest::Run(Sim, SlamTest::StepSeconds, Wizard, Totals);
		Brute = SlamTest::FirstOfKind(Sim, ECrowdKind::Brute);
		bStood &= FVector2D::Distance(Brute->Position, Stood) < 0.01;
		bHeld &= FVector2D::Distance(Brute->Facing, Faced) < 0.0001;
	}
	TestTrue(TEXT("it stands still through the slam"), bStood);
	TestTrue(TEXT("it does not turn through the slam"), bHeld);
	TestTrue(TEXT("it is still slamming"), Sim.IsSlamming(*Brute));

	SlamTest::Run(Sim, 0.4f, Wizard, Totals);
	TestEqual(TEXT("the slam lands where it was aimed"), Totals.Landed, 1);
	TestEqual(TEXT("the wizard, who left the circle, is not hit"), Totals.HitWizard, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMeleeStrikerStands, "WebTesting.Melee.AStrikerStandsAndDoesNotTurn", WIZ_TEST_FLAGS)
bool FMeleeStrikerStands::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	CrowdTest::MakeEnemiesPlain(Sim);
	// A slow grunt walks past a skeleton toward the wizard. The skeleton is in reach of it for two steps.
	Sim.Tuning.GruntSpeed = 60.0f;
	const FVector2D Wizard(450.0, 50.0);
	Sim.AddSkeleton(CrowdTest::Origin);
	Sim.AddEnemy(ECrowdKind::Grunt, FVector2D(60.0, 50.0));

	Sim.Step(SlamTest::StepSeconds, Wizard);
	const FCrowdUnit* Skeleton = CrowdTest::First(Sim, ECrowdTeam::Army);
	if (!TestNotNull(TEXT("the skeleton"), Skeleton) || !TestTrue(TEXT("the skeleton starts a blow at the grunt"), Skeleton->bStriking))
	{
		return false;
	}
	const FVector2D Stood = Skeleton->Position;
	const FVector2D Faced = Skeleton->Facing;
	TestTrue(TEXT("it faces the grunt as the blow starts"), Faced.X > 0.5 && Faced.Y > 0.5);

	Sim.Step(SlamTest::StepSeconds, Wizard);
	Skeleton = CrowdTest::First(Sim, ECrowdTeam::Army);
	TestTrue(TEXT("still in its blow"), Skeleton->bStriking);
	TestTrue(TEXT("it does not follow the grunt: it stands where the blow started"), FVector2D::Distance(Skeleton->Position, Stood) < 0.01);
	TestTrue(TEXT("it does not turn after the grunt"), FVector2D::Distance(Skeleton->Facing, Faced) < 0.0001);

	// Once the grunt is out of reach the blow is off, and the skeleton goes after it again.
	for (int32 Step = 0; Step < 6; ++Step)
	{
		Sim.Step(SlamTest::StepSeconds, Wizard);
	}
	Skeleton = CrowdTest::First(Sim, ECrowdTeam::Army);
	TestTrue(TEXT("with the grunt out of reach it walks again"), FVector2D::Distance(Skeleton->Position, Stood) > 1.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSummonAndSpearCosts, "WebTesting.Summon.ASkeletonCostsTwoBonesAndTheSpearTheSame", WIZ_TEST_FLAGS)
bool FSummonAndSpearCosts::RunTest(const FString& Parameters)
{
	{
		FCrowdSim Sim;
		Sim.DropBone(CrowdTest::Origin, 2);
		Sim.Step(0.05f, CrowdTest::Origin);
		TestEqual(TEXT("he holds 2 bones"), Sim.GetBonesHeld(), 2);
		TestEqual(TEXT("2 bones raise one skeleton"), Sim.Summon(CrowdTest::Origin), 1);
		TestEqual(TEXT("and they are spent"), Sim.GetBonesHeld(), 0);
	}
	{
		FCrowdSim Sim;
		Sim.SetSpearUnlocked(true);
		// Changed 2026-11-14 (director: the spear costs the same as one base army member): it was 3 bones, now 2.
		Sim.DropBone(CrowdTest::Origin, 2);
		Sim.Step(0.05f, CrowdTest::Origin);
		TestTrue(TEXT("2 bones and no army pay for a spear"), Sim.CanThrowSpear());
		TestTrue(TEXT("thrown"), Sim.ThrowSpear(CrowdTest::Origin, FVector2D(600.0, 0.0)));
		TestEqual(TEXT("the 2 bones are spent"), Sim.GetBonesHeld(), 0);
	}
	return true;
}

// Direction of 2026-11-14 (00:11 PST): with bone armor researched a skeleton is no longer killed by one slam, it is
// left with a third of its health; a guard is never killed by one slam, it is left with half.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSlamSparesArmorAndGuards, "WebTesting.Brute.TheSlamLeavesArmoredSkeletonsAThirdAndGuardsHalf", WIZ_TEST_FLAGS)
bool FSlamSparesArmorAndGuards::RunTest(const FString& Parameters)
{
	const FVector2D Wizard(200.0, 0.0);
	for (const bool bArmored : { false, true })
	{
		FCrowdSim Sim;
		CrowdTest::MakeEnemiesPlain(Sim);
		CrowdTest::FreezeArmy(Sim);
		Sim.Tuning.BruteSpeed = 0.0f;
		Sim.AddEnemy(ECrowdKind::Brute, CrowdTest::Origin);
		// Both stand in the circle in front of the brute, beside the wizard.
		Sim.AddSkeleton(FVector2D(250.0, 60.0));
		Sim.AddSkeleton(FVector2D(250.0, -60.0), ECrowdKind::Guard);
		if (bArmored)
		{
			Sim.DropBone(Wizard, Sim.GetShopCost(ECrowdShopItem::Armor));
			Sim.Step(SlamTest::StepSeconds, Wizard);
			TestEqual(TEXT("bone armor is bought"), Sim.Buy(ECrowdShopItem::Armor, 1, Wizard), 1);
		}
		const float SkeletonFull = Sim.GetStats(ECrowdKind::Skeleton).Health;
		const float GuardFull = Sim.GetStats(ECrowdKind::Guard).Health;

		SlamTest::FTotals Totals;
		for (int32 Step = 0; Step < 100 && Totals.Landed == 0; ++Step)
		{
			SlamTest::Run(Sim, SlamTest::StepSeconds, Wizard, Totals);
		}
		TestEqual(TEXT("one slam lands"), Totals.Landed, 1);

		const FCrowdUnit* Skeleton = SlamTest::FirstOfKind(Sim, ECrowdKind::Skeleton);
		const FCrowdUnit* Guard = SlamTest::FirstOfKind(Sim, ECrowdKind::Guard);
		if (bArmored)
		{
			if (TestNotNull(TEXT("the armored skeleton lives through the slam"), Skeleton))
			{
				TestTrue(TEXT("and is left with a third of its health"), FMath::IsNearlyEqual(Skeleton->Health, SkeletonFull / 3.0f, 0.01f));
			}
		}
		else
		{
			TestTrue(TEXT("without armor the slam kills a skeleton"), Skeleton == nullptr || Skeleton->Health <= 0.0f);
		}
		if (TestNotNull(TEXT("the guard lives through the slam"), Guard))
		{
			TestTrue(TEXT("and is left with half its health"), FMath::IsNearlyEqual(Guard->Health, GuardFull / 2.0f, 0.01f));
		}
	}
	return true;
}

#endif
