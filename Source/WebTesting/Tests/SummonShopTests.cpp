#include "Game/WizardHUD.h"
#include "Sim/ShrineState.h"
#include "Tests/CrowdTestHelpers.h"
#include "Tests/WizTestFlags.h"

#if WITH_DEV_AUTOMATION_TESTS

// Direction of 2026-11-12 (third round of the day):
// - bones no longer turn into skeletons by themselves: the spell "Summon the Afterlife" spends them, all at once or a
//   set number per cast (SummonMax, to be play-tested);
// - the Spear of Sacrifice is locked until it is bought at the shrine;
// - the shrine sells in bulk: Guards (a better unit, bought, never summoned) and bone armor for plain skeletons;
// - the shrine's circle is half again as big (345).

namespace SummonShopTest
{
	/** Puts bones in the wizard's hands: a pile dropped on him and collected in one step. */
	void GiveBones(FCrowdSim& Sim, int32 Count)
	{
		Sim.DropBone(CrowdTest::Origin, Count);
		Sim.Step(0.05f, CrowdTest::Origin);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSummonBonesWaitForSpell, "WebTesting.Summon.BonesWaitForTheSpell", WIZ_TEST_FLAGS)
bool FSummonBonesWaitForSpell::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	// Changed 2026-11-13 22:05 PST: a skeleton now costs 2 bones (Summon.ASkeletonCostsTwoBonesAndTheSpearThree tests
	// that). This test's numbers are for a price of 3, so it sets that price itself.
	Sim.Tuning.BonesPerSkeleton = 3.0f;
	SummonShopTest::GiveBones(Sim, 7);
	for (int32 Index = 0; Index < 10; ++Index)
	{
		Sim.Step(0.05f, CrowdTest::Origin);
	}
	TestEqual(TEXT("seven bones are held"), Sim.GetBonesHeld(), 7);
	TestEqual(TEXT("and nothing stands up by itself"), Sim.CountTeam(ECrowdTeam::Army), 0);
	TestEqual(TEXT("a cast would raise two"), Sim.GetSummonCount(), 2);

	TestEqual(TEXT("the spell raises two skeletons"), Sim.Summon(CrowdTest::Origin), 2);
	TestEqual(TEXT("they are in the army"), Sim.CountTeam(ECrowdTeam::Army), 2);
	TestEqual(TEXT("both are plain skeletons"), SummonShopTest::CountKind(Sim, ECrowdKind::Skeleton), 2);
	TestEqual(TEXT("one bone is left over"), Sim.GetBonesHeld(), 1);
	for (const FCrowdUnit& Unit : Sim.GetUnits())
	{
		TestTrue(TEXT("they stand up beside the wizard"), Unit.Position.Size() <= 150.0);
	}
	TestEqual(TEXT("a second cast has nothing to raise"), Sim.Summon(CrowdTest::Origin), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSummonOnePerCast, "WebTesting.Summon.SetNumberPerCastWhenTuned", WIZ_TEST_FLAGS)
bool FSummonOnePerCast::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	// Changed 2026-11-13 22:05 PST: a skeleton now costs 2 bones (Summon.ASkeletonCostsTwoBonesAndTheSpearThree tests
	// that). This test's numbers are for a price of 3, so it sets that price itself.
	Sim.Tuning.BonesPerSkeleton = 3.0f;
	TestEqual(TEXT("by default a cast uses all the bones it can"), Sim.Tuning.SummonMax, 0.0f);
	Sim.Tuning.SummonMax = 1.0f;
	SummonShopTest::GiveBones(Sim, 7);
	TestEqual(TEXT("a cast would raise one"), Sim.GetSummonCount(), 1);
	TestEqual(TEXT("one cast raises one"), Sim.Summon(CrowdTest::Origin), 1);
	TestEqual(TEXT("and spends three bones"), Sim.GetBonesHeld(), 4);
	TestEqual(TEXT("the next cast raises another"), Sim.Summon(CrowdTest::Origin), 1);
	TestEqual(TEXT("one bone left"), Sim.GetBonesHeld(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSummonStopsAtCap, "WebTesting.Summon.StopsAtTheArmyCap", WIZ_TEST_FLAGS)
bool FSummonStopsAtCap::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	// Changed 2026-11-13 22:05 PST: a skeleton now costs 2 bones (Summon.ASkeletonCostsTwoBonesAndTheSpearThree tests
	// that). This test's numbers are for a price of 3, so it sets that price itself.
	Sim.Tuning.BonesPerSkeleton = 3.0f;
	for (int32 Index = 0; Index < 39; ++Index)
	{
		Sim.AddSkeleton(FVector2D(200.0, 0.0));
	}
	SummonShopTest::GiveBones(Sim, 9);
	TestEqual(TEXT("only one place is free"), Sim.GetSummonCount(), 1);
	TestEqual(TEXT("so one is raised"), Sim.Summon(CrowdTest::Origin), 1);
	TestEqual(TEXT("the army is at its cap"), Sim.CountTeam(ECrowdTeam::Army), 40);
	TestEqual(TEXT("the other bones are kept"), Sim.GetBonesHeld(), 6);
	TestEqual(TEXT("a full army raises none"), Sim.Summon(CrowdTest::Origin), 0);
	TestEqual(TEXT("and spends nothing"), Sim.GetBonesHeld(), 6);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpearLockedUntilBought, "WebTesting.Shop.SpearIsLockedUntilBought", WIZ_TEST_FLAGS)
bool FSpearLockedUntilBought::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	for (int32 Index = 0; Index < 5; ++Index)
	{
		Sim.AddSkeleton(FVector2D(-100.0, 0.0));
	}
	TestFalse(TEXT("the spear starts locked"), Sim.IsSpearUnlocked());
	TestFalse(TEXT("so it cannot be thrown, even with an army"), Sim.CanThrowSpear());
	TestFalse(TEXT("and a throw is refused"), Sim.ThrowSpear(CrowdTest::Origin, FVector2D(600.0, 0.0)));
	TestEqual(TEXT("with nothing spent"), Sim.CountTeam(ECrowdTeam::Army), 5);

	TestEqual(TEXT("it costs 12 bones"), Sim.GetShopCost(ECrowdShopItem::Spear), 12);
	// Changed 2026-11-13 15:55 PST: the army now pays what the bones do not, so "11 bones cannot buy it" is no longer
	// true beside five skeletons. 5 bones and five skeletons (worth 1 each) still fall short of 12.
	SummonShopTest::GiveBones(Sim, 5);
	TestEqual(TEXT("5 bones and 5 skeletons cannot buy it"), Sim.Buy(ECrowdShopItem::Spear, 1, CrowdTest::Origin), 0);
	TestEqual(TEXT("and nothing is spent"), Sim.GetBonesHeld(), 5);
	TestEqual(TEXT("nor anyone"), Sim.CountTeam(ECrowdTeam::Army), 5);
	SummonShopTest::GiveBones(Sim, 11);
	TestEqual(TEXT("16 bones buy it"), Sim.Buy(ECrowdShopItem::Spear, 1, CrowdTest::Origin), 1);
	TestEqual(TEXT("for 12"), Sim.GetBonesHeld(), 4);
	TestTrue(TEXT("it is unlocked"), Sim.IsSpearUnlocked());
	TestTrue(TEXT("and can be thrown"), Sim.CanThrowSpear());

	SummonShopTest::GiveBones(Sim, 20);
	TestEqual(TEXT("it cannot be bought twice"), Sim.Buy(ECrowdShopItem::Spear, 1, CrowdTest::Origin), 0);
	TestEqual(TEXT("nothing more to buy"), Sim.GetShopMax(ECrowdShopItem::Spear), 0);
	TestEqual(TEXT("and the bones stay"), Sim.GetBonesHeld(), 24);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopBulkGuards, "WebTesting.Shop.GuardsAreBoughtInBulk", WIZ_TEST_FLAGS)
bool FShopBulkGuards::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	// Changed 2026-11-13 22:05 PST: a skeleton now costs 2 bones (Summon.ASkeletonCostsTwoBonesAndTheSpearThree tests
	// that). This test's numbers are for a price of 3, so it sets that price itself.
	Sim.Tuning.BonesPerSkeleton = 3.0f;
	TestEqual(TEXT("a guard costs 8 bones"), Sim.GetShopCost(ECrowdShopItem::Guard), 8);
	SummonShopTest::GiveBones(Sim, 5);
	TestEqual(TEXT("5 bones buy none"), Sim.GetShopMax(ECrowdShopItem::Guard), 0);
	TestEqual(TEXT("the buy is refused"), Sim.Buy(ECrowdShopItem::Guard, 1, CrowdTest::Origin), 0);
	TestEqual(TEXT("and nothing is spent"), Sim.GetBonesHeld(), 5);

	SummonShopTest::GiveBones(Sim, 25);
	TestEqual(TEXT("30 bones can buy three"), Sim.GetShopMax(ECrowdShopItem::Guard), 3);
	TestEqual(TEXT("asking for five in one click buys the three that can be paid for"), Sim.Buy(ECrowdShopItem::Guard, 5, CrowdTest::Origin), 3);
	TestEqual(TEXT("three guards stand in the army"), SummonShopTest::CountKind(Sim, ECrowdKind::Guard), 3);
	TestEqual(TEXT("six bones are left"), Sim.GetBonesHeld(), 6);
	TestEqual(TEXT("summoning never makes a guard"), Sim.Summon(CrowdTest::Origin), 2);
	TestEqual(TEXT("still three guards"), SummonShopTest::CountKind(Sim, ECrowdKind::Guard), 3);

	// The army cap limits a bulk buy as well.
	FCrowdSim Full;
	for (int32 Index = 0; Index < 39; ++Index)
	{
		Full.AddSkeleton(FVector2D(200.0, 0.0));
	}
	SummonShopTest::GiveBones(Full, 40);
	TestEqual(TEXT("one place free: one guard can be bought"), Full.GetShopMax(ECrowdShopItem::Guard), 1);
	TestEqual(TEXT("and only one is"), Full.Buy(ECrowdShopItem::Guard, 5, CrowdTest::Origin), 1);
	TestEqual(TEXT("for 8 bones"), Full.GetBonesHeld(), 32);
	return true;
}

// Changed 2026-11-12: bone armor had five levels. Upgrades are research now, bought once: see
// Research.EachIsBoughtOnce in GarrisonResearchTests.cpp, which replaces the armor test that stood here.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShrineBiggerCircle, "WebTesting.Shrine.CircleIsHalfAgainAsBig", WIZ_TEST_FLAGS)
bool FShrineBiggerCircle::RunTest(const FString& Parameters)
{
	FShrineState Shrine;
	TestEqual(TEXT("230 grew by half"), Shrine.Range, 345.0f);
	TestTrue(TEXT("340 units away opens it"), Shrine.TryOpen(FVector2D(340.0, 0.0)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHudShrineMenuButtons, "WebTesting.Hud.ShrineMenuButtons", WIZ_TEST_FLAGS)
bool FHudShrineMenuButtons::RunTest(const FString& Parameters)
{
	const float Sizes[][2] = { { 1920.0f, 1080.0f }, { 1280.0f, 720.0f }, { 640.0f, 480.0f }, { 3840.0f, 2160.0f } };
	for (const float* Size : Sizes)
	{
		const float Width = Size[0];
		const float Height = Size[1];
		const FBox2D Menu = AWizardHUD::ShrineMenuBox(Width, Height);
		TArray<FBox2D> Boxes;
		for (int32 Row = 0; Row < AWizardHUD::ShrineMenuRows; ++Row)
		{
			const int32 Buttons = AWizardHUD::ShrineMenuButtonCount(Row);
			TestTrue(TEXT("every row has a button"), Buttons >= 1);
			for (int32 Button = 0; Button < Buttons; ++Button)
			{
				const FBox2D Box = AWizardHUD::ShrineMenuButtonBox(Row, Button, Width, Height);
				TestTrue(TEXT("a button is inside the menu"), Menu.IsInside(Box.Min) && Menu.IsInside(Box.Max));
				TestTrue(TEXT("a button is big enough to click"), Box.GetSize().X >= 30.0 && Box.GetSize().Y >= 18.0);
				for (const FBox2D& Other : Boxes)
				{
					TestFalse(TEXT("buttons do not overlap"), Box.Intersect(Other));
				}
				Boxes.Add(Box);

				int32 HitRow = -1;
				int32 HitButton = -1;
				TestTrue(TEXT("a click in the middle of a button hits it"), AWizardHUD::ShrineMenuButtonAt(Box.GetCenter(), Width, Height, HitRow, HitButton));
				TestEqual(TEXT("the right row"), HitRow, Row);
				TestEqual(TEXT("the right button"), HitButton, Button);
			}
		}
		int32 HitRow = -1;
		int32 HitButton = -1;
		TestFalse(TEXT("a click on the title hits no button"), AWizardHUD::ShrineMenuButtonAt(FVector2D(Menu.GetCenter().X, Menu.Min.Y + 5.0), Width, Height, HitRow, HitButton));
		TestFalse(TEXT("a click outside the menu hits no button"), AWizardHUD::ShrineMenuButtonAt(FVector2D(10.0, 10.0), Width, Height, HitRow, HitButton));
	}
	// Changed 2026-11-12: research rows (spear, armor, blades, focus) have one button; the Guard, a unit, has three.
	TestEqual(TEXT("one row for each thing sold"), AWizardHUD::ShrineMenuRows, CrowdShopItemCount);
	for (int32 Row = 0; Row < AWizardHUD::ShrineMenuRows; ++Row)
	{
		const bool bUnit = static_cast<ECrowdShopItem>(Row) == ECrowdShopItem::Guard || static_cast<ECrowdShopItem>(Row) == ECrowdShopItem::Marksman;
		TestEqual(TEXT("research is bought once: one button. A unit: one, five, as many as can be paid for"), AWizardHUD::ShrineMenuButtonCount(Row), bUnit ? 3 : 1);
	}
	TestEqual(TEXT("the unit's last button buys all that can be paid for"), AWizardHUD::ShrineMenuButtonAmount(static_cast<int32>(ECrowdShopItem::Guard), 2), MAX_int32);
	TestEqual(TEXT("a research button buys one"), AWizardHUD::ShrineMenuButtonAmount(static_cast<int32>(ECrowdShopItem::Armor), 0), 1);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
