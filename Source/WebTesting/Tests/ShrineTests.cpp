#include "Game/WizardHUD.h"
#include "Sim/ShrineState.h"
#include "Tests/CrowdTestHelpers.h"
#include "Tests/WizTestFlags.h"

#if WITH_DEV_AUTOMATION_TESTS

// Direction of 2026-11-12 15:25 PST:
// - every kind of enemy leaves its own number of bones (grunt 1, brute 3);
// - the shrine opens only when the wizard is near, the fight goes on while it is open, he can walk but cannot use
//   Bolt or the spear, and its menu closes when he walks away, presses Escape, or clicks outside the menu box.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBonesBruteLeavesThree, "WebTesting.Bones.BruteLeavesThree", WIZ_TEST_FLAGS)
bool FBonesBruteLeavesThree::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	// Changed 2026-11-13 22:05 PST: a skeleton now costs 2 bones (Summon.ASkeletonCostsTwoBonesAndTheSpearThree tests
	// that). This test's numbers are for a price of 3, so it sets that price itself.
	Sim.Tuning.BonesPerSkeleton = 3.0f;
	Sim.Tuning.BruteSpeed = 0.0f;
	Sim.Tuning.BruteHealth = 10.0f;
	Sim.AddEnemy(ECrowdKind::Brute, FVector2D(300.0, 0.0));
	Sim.FireBolt(CrowdTest::Origin, FVector2D(1.0, 0.0));
	const FVector2D FarAway(-2000.0, 0.0);
	for (int32 Index = 0; Index < 10; ++Index)
	{
		Sim.Step(0.05f, FarAway);
	}
	TestEqual(TEXT("the brute died"), Sim.CountTeam(ECrowdTeam::Enemy), 0);
	if (TestEqual(TEXT("it left one pile"), Sim.GetBones().Num(), 1))
	{
		TestEqual(TEXT("worth three bones"), Sim.GetBones()[0].Value, 3);
		TestFalse(TEXT("of plain bones, the same kind a grunt leaves"), Sim.GetBones()[0].bGuard);
	}

	const FCrowdStepResult Result = Sim.Step(0.05f, FVector2D(300.0, 0.0));
	TestEqual(TEXT("collecting it counts three bones"), Result.BonesCollected, 3);
	// Changed 2026-11-12: bones wait for the spell Summon the Afterlife.
	TestEqual(TEXT("the bones are held, not assembled"), Sim.CountTeam(ECrowdTeam::Army), 0);
	TestEqual(TEXT("the spell turns three bones into one skeleton"), Sim.Summon(FVector2D(300.0, 0.0)), 1);
	const FCrowdUnit* Raised = CrowdTest::First(Sim, ECrowdTeam::Army);
	if (TestNotNull(TEXT("the unit"), Raised))
	{
		TestTrue(TEXT("is a plain skeleton"), Raised->Kind == ECrowdKind::Skeleton);
	}
	TestEqual(TEXT("and the bones are spent"), Sim.GetBonesHeld(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBonesValueByKind, "WebTesting.Bones.ValueByKind", WIZ_TEST_FLAGS)
bool FBonesValueByKind::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	TestEqual(TEXT("a grunt is worth 1"), Sim.GetBoneValue(ECrowdKind::Grunt), 1);
	TestEqual(TEXT("a brute is worth 3"), Sim.GetBoneValue(ECrowdKind::Brute), 3);
	TestEqual(TEXT("the army leaves none"), Sim.GetBoneValue(ECrowdKind::Skeleton), 0);
	Sim.Tuning.BonesPerBrute = 5.0f;
	TestEqual(TEXT("the brute's value can be tuned"), Sim.GetBoneValue(ECrowdKind::Brute), 5);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShrineOpensOnlyInRange, "WebTesting.Shrine.OpensOnlyInRange", WIZ_TEST_FLAGS)
bool FShrineOpensOnlyInRange::RunTest(const FString& Parameters)
{
	FShrineState Shrine;
	Shrine.Position = FVector2D(400.0, 100.0);
	Shrine.Range = 200.0f;

	TestFalse(TEXT("far away is out of range"), Shrine.IsInRange(FVector2D(0.0, 0.0)));
	TestFalse(TEXT("so it does not open"), Shrine.TryOpen(FVector2D(0.0, 0.0)));
	TestFalse(TEXT("and stays closed"), Shrine.IsOpen());

	TestTrue(TEXT("190 units away is in range"), Shrine.IsInRange(FVector2D(400.0, 290.0)));
	TestFalse(TEXT("210 units away is not"), Shrine.IsInRange(FVector2D(400.0, 310.0)));
	TestTrue(TEXT("in range it opens"), Shrine.TryOpen(FVector2D(400.0, 290.0)));
	TestTrue(TEXT("and is open"), Shrine.IsOpen());

	Shrine.Close();
	TestFalse(TEXT("closing closes it"), Shrine.IsOpen());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShrineClosesWhenWizardLeaves, "WebTesting.Shrine.ClosesWhenTheWizardLeaves", WIZ_TEST_FLAGS)
bool FShrineClosesWhenWizardLeaves::RunTest(const FString& Parameters)
{
	FShrineState Shrine;
	Shrine.Position = FVector2D(400.0, 100.0);
	Shrine.Range = 200.0f;
	Shrine.TryOpen(FVector2D(400.0, 100.0));

	TestFalse(TEXT("walking about inside the range changes nothing"), Shrine.Update(FVector2D(520.0, 180.0)));
	TestTrue(TEXT("it is still open: he can walk while it is open"), Shrine.IsOpen());

	TestTrue(TEXT("stepping out of range closes it"), Shrine.Update(FVector2D(400.0, 320.0)));
	TestFalse(TEXT("and it is closed"), Shrine.IsOpen());
	TestFalse(TEXT("a closed shrine has nothing to close"), Shrine.Update(FVector2D(2000.0, 0.0)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShrineBlocksAbilities, "WebTesting.Shrine.HoldsAbilitiesWhileOpen", WIZ_TEST_FLAGS)
bool FShrineBlocksAbilities::RunTest(const FString& Parameters)
{
	FShrineState Shrine;
	Shrine.Position = FVector2D(0.0, 0.0);
	TestTrue(TEXT("closed: Bolt and the spear work"), Shrine.AllowsAbilities());
	Shrine.TryOpen(FVector2D(50.0, 0.0));
	TestFalse(TEXT("open: no Bolt and no spear"), Shrine.AllowsAbilities());
	Shrine.Update(FVector2D(5000.0, 0.0));
	TestTrue(TEXT("put out of it by walking away: they work again"), Shrine.AllowsAbilities());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHudShrineMenuBox, "WebTesting.Hud.ShrineMenuBox", WIZ_TEST_FLAGS)
bool FHudShrineMenuBox::RunTest(const FString& Parameters)
{
	const FBox2D Box = AWizardHUD::ShrineMenuBox(1920.0f, 1080.0f);
	// Changed after the first screenshot: a box in the middle of the screen covered the wizard, who can still walk
	// and be attacked while it is open. The box now stands at the right, level with the middle.
	TestTrue(TEXT("the box is level with the middle of the screen"), FMath::IsNearlyEqual(Box.GetCenter().Y, 540.0, 1.0));
	TestTrue(TEXT("the box is on the right and does not cover the wizard in the middle"), Box.Min.X >= 960.0 + 150.0);
	TestTrue(TEXT("the box is on the screen"), Box.Max.X <= 1920.0 - 20.0);
	TestTrue(TEXT("a click in the middle of the box is inside"), AWizardHUD::IsInShrineMenu(Box.GetCenter(), 1920.0f, 1080.0f));
	TestFalse(TEXT("a click on the wizard is outside"), AWizardHUD::IsInShrineMenu(FVector2D(960.0, 540.0), 1920.0f, 1080.0f));
	TestFalse(TEXT("a click in the corner is outside"), AWizardHUD::IsInShrineMenu(FVector2D(20.0, 20.0), 1920.0f, 1080.0f));
	TestFalse(TEXT("a click just left of the box is outside"), AWizardHUD::IsInShrineMenu(FVector2D(Box.Min.X - 5.0, 540.0), 1920.0f, 1080.0f));
	// Changed 2026-11-14: the designed panel is 600 wide with rows 96 tall, and six rows make it 800 tall. It stands at
	// the right, so the room to click outside is at its left; the height may now be up to 0.75 of the screen (it was 0.6).
	TestTrue(TEXT("the box leaves room around it to click outside"), Box.GetSize().X <= 1920.0 * 0.6 && Box.GetSize().Y <= 1080.0 * 0.75);

	const FBox2D Small = AWizardHUD::ShrineMenuBox(640.0f, 480.0f);
	TestTrue(TEXT("on a small screen the box still fits with room to click outside"), Small.Min.X >= 20.0 && Small.Min.Y >= 20.0 && Small.Max.X <= 620.0 && Small.Max.Y <= 460.0);
	const FBox2D Large = AWizardHUD::ShrineMenuBox(3840.0f, 2160.0f);
	TestTrue(TEXT("at 4K it is twice the size"), FMath::IsNearlyEqual(Large.GetSize().X, Box.GetSize().X * 2.0, 2.0));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
