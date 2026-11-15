#include "Game/WizardHUD.h"
#include "Tests/CrowdTestHelpers.h"
#include "Tests/WizTestFlags.h"

#if WITH_DEV_AUTOMATION_TESTS

// Direction of 2026-11-12 12:15 PST:
// - the army stands in 3 to 6 groups, by its size, and no group is larger than 15;
// - a kill leaves bones, the wizard collects them by walking near, and enough bones assemble a skeleton;
// - an enemy shows a health bar only for a few seconds after it takes damage.

namespace CrowdGroupTest
{
	struct FGroupReport
	{
		int32 Groups = 0;
		int32 Largest = 0;
		int32 Smallest = MAX_int32;
		int32 Ungrouped = 0;
		double FarthestFromOwnMiddle = 0.0;
		double ClosestMiddles = TNumericLimits<double>::Max();
	};

	/** Puts Count skeletons in a block beside the wizard and lets them sort themselves for 15 seconds. */
	FGroupReport Settle(FCrowdSim& Sim, int32 Count)
	{
		for (int32 Index = 0; Index < Count; ++Index)
		{
			Sim.AddSkeleton(FVector2D(120.0 + (Index % 8) * 30.0, -100.0 + (Index / 8) * 30.0));
		}
		for (int32 Index = 0; Index < 300; ++Index)
		{
			Sim.Step(0.05f, CrowdTest::Origin);
		}

		TMap<int32, TArray<FVector2D>> Members;
		FGroupReport Report;
		for (const FCrowdUnit& Unit : Sim.GetUnits())
		{
			if (Unit.Group == INDEX_NONE)
			{
				++Report.Ungrouped;
				continue;
			}
			Members.FindOrAdd(Unit.Group).Add(Unit.Position);
		}

		TArray<FVector2D> Middles;
		for (const TPair<int32, TArray<FVector2D>>& Pair : Members)
		{
			FVector2D Middle = FVector2D::ZeroVector;
			for (const FVector2D& Position : Pair.Value)
			{
				Middle += Position;
			}
			Middle /= Pair.Value.Num();
			for (const FVector2D& Position : Pair.Value)
			{
				Report.FarthestFromOwnMiddle = FMath::Max(Report.FarthestFromOwnMiddle, FVector2D::Distance(Position, Middle));
			}
			Report.Largest = FMath::Max(Report.Largest, Pair.Value.Num());
			Report.Smallest = FMath::Min(Report.Smallest, Pair.Value.Num());
			Middles.Add(Middle);
		}
		Report.Groups = Middles.Num();
		for (int32 Index = 0; Index < Middles.Num(); ++Index)
		{
			for (int32 Other = Index + 1; Other < Middles.Num(); ++Other)
			{
				Report.ClosestMiddles = FMath::Min(Report.ClosestMiddles, FVector2D::Distance(Middles[Index], Middles[Other]));
			}
		}
		return Report;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrowdGroupsFullArmy, "WebTesting.Groups.FullArmyStandsInSix", WIZ_TEST_FLAGS)
bool FCrowdGroupsFullArmy::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	const CrowdGroupTest::FGroupReport Report = CrowdGroupTest::Settle(Sim, 40);
	TestEqual(TEXT("every skeleton is in a group"), Report.Ungrouped, 0);
	TestEqual(TEXT("an army of 40 stands in 6 groups"), Report.Groups, 6);
	TestTrue(TEXT("no group is larger than 15"), Report.Largest <= 15);
	TestTrue(TEXT("the groups are even: the largest has at most 2 more than the smallest"), Report.Largest - Report.Smallest <= 2);
	// Changed 2026-11-13 17:21 PST: the figures are over twice as large (the director's table), so a group needs more
	// room. The limit was 140 when a group shuffled within 60 units; it is the shuffle distance plus 80 now.
	TestTrue(TEXT("each group is tight: nobody is more than its shuffle distance plus 80 from the middle of its group"),
		Report.FarthestFromOwnMiddle <= Sim.Tuning.GroupShuffle + 80.0);
	TestTrue(TEXT("the groups stand apart: middles at least 150 from each other"), Report.ClosestMiddles >= 150.0);

	double Farthest = 0.0;
	for (const FCrowdUnit& Unit : Sim.GetUnits())
	{
		Farthest = FMath::Max(Farthest, Unit.Position.Size());
	}
	TestTrue(TEXT("the groups stay inside the roam circle"), Farthest <= Sim.Tuning.RoamRadius + 10.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrowdGroupsSmallArmy, "WebTesting.Groups.SmallArmyStandsInThree", WIZ_TEST_FLAGS)
bool FCrowdGroupsSmallArmy::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	const CrowdGroupTest::FGroupReport Report = CrowdGroupTest::Settle(Sim, 12);
	TestEqual(TEXT("every skeleton is in a group"), Report.Ungrouped, 0);
	TestEqual(TEXT("an army of 12 stands in 3 groups"), Report.Groups, 3);
	TestEqual(TEXT("of 4 each"), Report.Largest, 4);
	TestEqual(TEXT("of 4 each (smallest)"), Report.Smallest, 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrowdGroupsCountBySize, "WebTesting.Groups.CountFollowsArmySize", WIZ_TEST_FLAGS)
bool FCrowdGroupsCountBySize::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	TestEqual(TEXT("no army, no groups"), Sim.GetGroupCount(0), 0);
	TestEqual(TEXT("2 skeletons cannot make 3 groups"), Sim.GetGroupCount(2), 2);
	TestEqual(TEXT("3"), Sim.GetGroupCount(3), 3);
	TestEqual(TEXT("21"), Sim.GetGroupCount(21), 3);
	TestEqual(TEXT("22"), Sim.GetGroupCount(22), 4);
	TestEqual(TEXT("35"), Sim.GetGroupCount(35), 5);
	TestEqual(TEXT("40"), Sim.GetGroupCount(40), 6);
	TestEqual(TEXT("never more than 6"), Sim.GetGroupCount(90), 6);
	Sim.Tuning.GroupTargetSize = 0.0f;
	TestEqual(TEXT("groups switched off"), Sim.GetGroupCount(40), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrowdBonesCollectedOnlyWhenNear, "WebTesting.Bones.CollectedOnlyWhenNear", WIZ_TEST_FLAGS)
bool FCrowdBonesCollectedOnlyWhenNear::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	TestTrue(TEXT("the bone is dropped"), Sim.DropBone(FVector2D(500.0, 0.0), 1));
	for (int32 Index = 0; Index < 20; ++Index)
	{
		Sim.Step(0.05f, CrowdTest::Origin);
	}
	TestEqual(TEXT("a far bone stays on the ground"), Sim.GetBones().Num(), 1);
	TestEqual(TEXT("and is not held"), Sim.GetBonesHeld(), 0);

	// Just outside the pickup distance, then just inside it.
	Sim.Step(0.05f, FVector2D(500.0 - Sim.Tuning.BonePickupRadius - 10.0, 0.0));
	TestEqual(TEXT("10 units too far: still on the ground"), Sim.GetBones().Num(), 1);
	const FCrowdStepResult Result = Sim.Step(0.05f, FVector2D(500.0 - Sim.Tuning.BonePickupRadius + 10.0, 0.0));
	TestEqual(TEXT("walking near collects it"), Sim.GetBones().Num(), 0);
	TestEqual(TEXT("the wizard holds one bone"), Sim.GetBonesHeld(), 1);
	TestEqual(TEXT("the step reports it"), Result.BonesCollected, 1);
	TestEqual(TEXT("one bone is not a skeleton"), Sim.CountTeam(ECrowdTeam::Army), 0);
	return true;
}

// Changed 2026-11-12: bones used to assemble a skeleton by themselves at three. Now they are held until the wizard
// casts Summon the Afterlife (see SummonShopTests.cpp).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrowdBonesAssembleSkeleton, "WebTesting.Bones.ThreeMakeASkeletonWhenSummoned", WIZ_TEST_FLAGS)
bool FCrowdBonesAssembleSkeleton::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	// Changed 2026-11-13 22:05 PST: a skeleton now costs 2 bones (Summon.ASkeletonCostsTwoBonesAndTheSpearThree tests
	// that). This test's numbers are for a price of 3, so it sets that price itself.
	Sim.Tuning.BonesPerSkeleton = 3.0f;
	Sim.DropBone(FVector2D(40.0, 0.0), 1);
	Sim.DropBone(FVector2D(-40.0, 0.0), 1);
	Sim.Step(0.05f, CrowdTest::Origin);
	TestEqual(TEXT("two bones held"), Sim.GetBonesHeld(), 2);
	TestEqual(TEXT("two bones summon nothing"), Sim.Summon(CrowdTest::Origin), 0);

	Sim.DropBone(FVector2D(0.0, 40.0), 1);
	const FCrowdStepResult Result = Sim.Step(0.05f, CrowdTest::Origin);
	TestEqual(TEXT("three bones held"), Sim.GetBonesHeld(), 3);
	TestEqual(TEXT("collecting them raises nothing"), Result.SkeletonsRaised, 0);
	TestEqual(TEXT("no skeleton yet"), Sim.CountTeam(ECrowdTeam::Army), 0);
	TestEqual(TEXT("the spell raises one"), Sim.Summon(CrowdTest::Origin), 1);
	TestEqual(TEXT("the bones are spent"), Sim.GetBonesHeld(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrowdBonesKeptWhenArmyFull, "WebTesting.Bones.KeptWhenArmyIsFull", WIZ_TEST_FLAGS)
bool FCrowdBonesKeptWhenArmyFull::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	for (int32 Index = 0; Index < 40; ++Index)
	{
		Sim.AddSkeleton(FVector2D(200.0, 0.0));
	}
	Sim.DropBone(FVector2D(40.0, 0.0), 3);
	Sim.Step(0.05f, CrowdTest::Origin);
	TestEqual(TEXT("a full army summons none"), Sim.Summon(CrowdTest::Origin), 0);
	TestEqual(TEXT("the army stays at its cap"), Sim.CountTeam(ECrowdTeam::Army), 40);
	TestEqual(TEXT("the bones are kept for later"), Sim.GetBonesHeld(), 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrowdBonesFadeAway, "WebTesting.Bones.FadeWhenLeft", WIZ_TEST_FLAGS)
bool FCrowdBonesFadeAway::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	Sim.Tuning.BoneLife = 2.0f;
	Sim.DropBone(FVector2D(900.0, 0.0), 1);
	for (int32 Index = 0; Index < 30; ++Index)
	{
		Sim.Step(0.05f, CrowdTest::Origin);
	}
	TestEqual(TEXT("still there after 1.5 seconds"), Sim.GetBones().Num(), 1);
	for (int32 Index = 0; Index < 20; ++Index)
	{
		Sim.Step(0.05f, CrowdTest::Origin);
	}
	TestEqual(TEXT("gone after 2.5 seconds"), Sim.GetBones().Num(), 0);
	TestEqual(TEXT("and not collected"), Sim.GetBonesHeld(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrowdHealthBarOnlyAfterDamage, "WebTesting.HealthBar.OnlyAfterDamage", WIZ_TEST_FLAGS)
bool FCrowdHealthBarOnlyAfterDamage::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	Sim.Tuning.GruntSpeed = 0.0f;
	Sim.Tuning.GruntHealth = 100.0f;
	Sim.AddEnemy(ECrowdKind::Grunt, FVector2D(500.0, 0.0));
	Sim.AddEnemy(ECrowdKind::Grunt, FVector2D(-500.0, 0.0));
	Sim.Step(0.05f, CrowdTest::Origin);
	for (const FCrowdUnit& Unit : Sim.GetUnits())
	{
		TestEqual(TEXT("an unhurt enemy shows no bar"), Unit.HurtTimer, 0.0f);
		TestEqual(TEXT("full health is remembered"), Unit.MaxHealth, 100.0f);
	}

	Sim.FireBolt(CrowdTest::Origin, FVector2D(1.0, 0.0));
	for (int32 Index = 0; Index < 10 && Sim.GetBolts().Num() > 0; ++Index)
	{
		Sim.Step(0.05f, CrowdTest::Origin);
	}
	const FCrowdUnit& Hit = Sim.GetUnits()[0];
	const FCrowdUnit& Other = Sim.GetUnits()[1];
	TestEqual(TEXT("the bolt took 10 health"), Hit.Health, 90.0f);
	TestTrue(TEXT("the hit enemy shows its bar"), Hit.HurtTimer > Sim.Tuning.HealthBarSeconds - 0.1f);
	TestEqual(TEXT("the other enemy shows none"), Other.HurtTimer, 0.0f);

	for (int32 Index = 0; Index < 56; ++Index)
	{
		Sim.Step(0.05f, CrowdTest::Origin);
	}
	TestEqual(TEXT("the bar is gone 2.8 seconds later"), Sim.GetUnits()[0].HurtTimer, 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardHudHealthBarAlpha, "WebTesting.HealthBar.FadesOut", WIZ_TEST_FLAGS)
bool FWizardHudHealthBarAlpha::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("no time left: not drawn"), AWizardHUD::HealthBarAlpha(0.0f), 0.0f);
	TestEqual(TEXT("just hit: solid"), AWizardHUD::HealthBarAlpha(2.5f), 1.0f);
	TestEqual(TEXT("solid until the fade starts"), AWizardHUD::HealthBarAlpha(AWizardHUD::HealthBarFadeSeconds), 1.0f);
	TestEqual(TEXT("half way through the fade"), AWizardHUD::HealthBarAlpha(AWizardHUD::HealthBarFadeSeconds * 0.5f), 0.5f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
