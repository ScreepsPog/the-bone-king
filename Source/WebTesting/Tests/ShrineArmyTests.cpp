#include "Sim/CrowdSim.h"
#include "Tests/CrowdTestHelpers.h"
#include "Tests/WizTestFlags.h"

#if WITH_DEV_AUTOMATION_TESTS

// Direction of 2026-11-13 (15:55 PST): at the shrine, bones are spent first and the army pays the rest and is
// sacrificed, leaving no bones; and a melee attack is one blow, not a steady drain.

namespace ShrineArmyTest
{
	/** A crowd with this many bones held and this army standing with the wizard at the origin. */
	void Make(FCrowdSim& Sim, int32 Bones, int32 Skeletons, int32 Guards)
	{
		CrowdTest::FreezeArmy(Sim);
		Sim.Tuning.GuardSpeed = 0.0f;
		if (Bones > 0)
		{
			Sim.DropBone(CrowdTest::Origin, Bones);
		}
		for (int32 Index = 0; Index < Skeletons + Guards; ++Index)
		{
			Sim.AddSkeleton(FVector2D(120.0 + Index * 20.0, 100.0), Index < Skeletons ? ECrowdKind::Skeleton : ECrowdKind::Guard);
		}
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShrineArmyPays, "WebTesting.Shrine.BonesFirstThenTheArmyPays", WIZ_TEST_FLAGS)
bool FShrineArmyPays::RunTest(const FString& Parameters)
{
	// Bone armor costs 15. He holds 4 bones and has 20 skeletons, each worth 1.
	FCrowdSim Sim;
	ShrineArmyTest::Make(Sim, 4, 20, 0);
	TestEqual(TEXT("4 bones held"), Sim.GetBonesHeld(), 4);
	TestEqual(TEXT("a skeleton is worth 1"), Sim.GetUnitWorth(ECrowdKind::Skeleton), 1);
	TestEqual(TEXT("a Guard is worth 3"), Sim.GetUnitWorth(ECrowdKind::Guard), 3);
	TestEqual(TEXT("the armor can be bought"), Sim.GetShopMax(ECrowdShopItem::Armor), 1);
	const FCrowdShopPrice Price = Sim.GetShopPrice(ECrowdShopItem::Armor, 1);
	TestTrue(TEXT("it can be paid"), Price.bCanPay);
	TestEqual(TEXT("all 4 bones go first"), Price.Bones, 4);
	TestEqual(TEXT("11 skeletons pay the rest"), Price.Skeletons, 11);
	TestEqual(TEXT("no Guards"), Price.Guards, 0);
	TestEqual(TEXT("bought"), Sim.Buy(ECrowdShopItem::Armor, 1, CrowdTest::Origin), 1);
	TestTrue(TEXT("the armor is researched"), Sim.IsResearched(ECrowdShopItem::Armor));
	TestEqual(TEXT("no bones left"), Sim.GetBonesHeld(), 0);
	TestEqual(TEXT("9 skeletons left"), Sim.CountTeam(ECrowdTeam::Army), 9);
	TestEqual(TEXT("the 11 left no bones on the ground"), Sim.GetBones().Num(), 0);
	TestEqual(TEXT("11 were sacrificed"), Sim.GetSacrificed(), 11);

	// With bones enough, nobody is sacrificed.
	FCrowdSim Rich;
	ShrineArmyTest::Make(Rich, 20, 10, 0);
	TestEqual(TEXT("with 20 bones the price is 15 bones and nobody"), Rich.GetShopPrice(ECrowdShopItem::Armor, 1).Skeletons, 0);
	Rich.Buy(ECrowdShopItem::Armor, 1, CrowdTest::Origin);
	TestEqual(TEXT("5 bones left"), Rich.GetBonesHeld(), 5);
	TestEqual(TEXT("the army is whole"), Rich.CountTeam(ECrowdTeam::Army), 10);

	// No bones at all and a large army: that is the player's choice to make.
	FCrowdSim Poor;
	ShrineArmyTest::Make(Poor, 0, 15, 0);
	TestEqual(TEXT("an army alone can buy"), Poor.Buy(ECrowdShopItem::Blades, 1, CrowdTest::Origin), 1);
	TestEqual(TEXT("and all 15 are gone"), Poor.CountTeam(ECrowdTeam::Army), 0);

	// Not enough of both: nothing is bought and nothing is taken.
	FCrowdSim Short;
	ShrineArmyTest::Make(Short, 2, 3, 0);
	TestEqual(TEXT("2 bones and 3 skeletons cannot buy the armor"), Short.GetShopMax(ECrowdShopItem::Armor), 0);
	TestFalse(TEXT("the price says so"), Short.GetShopPrice(ECrowdShopItem::Armor, 1).bCanPay);
	TestEqual(TEXT("nothing bought"), Short.Buy(ECrowdShopItem::Armor, 1, CrowdTest::Origin), 0);
	TestEqual(TEXT("bones kept"), Short.GetBonesHeld(), 2);
	TestEqual(TEXT("army kept"), Short.CountTeam(ECrowdTeam::Army), 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShrineArmyOrder, "WebTesting.Shrine.SkeletonsPayBeforeGuardsAndSentUnitsDoNot", WIZ_TEST_FLAGS)
bool FShrineArmyOrder::RunTest(const FString& Parameters)
{
	// Changed 2026-11-14 19:40 PST (director: "don't allow for guards/marksman to be converted in the army"). It used
	// to be: dark focus costs 15, two skeletons pay 2 and five Guards pay the rest. Now Guards never pay, so two
	// skeletons and six Guards cannot buy it, and nobody is taken.
	FCrowdSim Sim;
	ShrineArmyTest::Make(Sim, 0, 2, 6);
	const FCrowdShopPrice Price = Sim.GetShopPrice(ECrowdShopItem::Focus, 1);
	TestFalse(TEXT("it cannot be paid"), Price.bCanPay);
	TestEqual(TEXT("both skeletons are offered"), Price.Skeletons, 2);
	TestEqual(TEXT("no Guard is"), Price.Guards, 0);
	Sim.Buy(ECrowdShopItem::Focus, 1, CrowdTest::Origin);
	TestEqual(TEXT("both skeletons are still there"), ShrineArmyTest::CountKind(Sim, ECrowdKind::Skeleton), 2);
	TestEqual(TEXT("and all six Guards"), ShrineArmyTest::CountKind(Sim, ECrowdKind::Guard), 6);

	// Units that were sent away do not pay.
	FCrowdSim Sent;
	ShrineArmyTest::Make(Sent, 0, 20, 0);
	Sent.Command(FVector2D(3000.0, 0.0), 15, ECrowdCommandKind::Any);
	TestEqual(TEXT("5 with him cannot pay 15"), Sent.GetShopMax(ECrowdShopItem::Armor), 0);

	// A Guard (8 bones) can be paid for with skeletons, never with Guards.
	FCrowdSim Guards;
	ShrineArmyTest::Make(Guards, 0, 8, 2);
	TestEqual(TEXT("8 skeletons buy one Guard; the two Guards do not count"), Guards.GetShopMax(ECrowdShopItem::Guard), 1);
	TestEqual(TEXT("bought"), Guards.Buy(ECrowdShopItem::Guard, 5, CrowdTest::Origin), 1);
	TestEqual(TEXT("the skeletons are gone"), ShrineArmyTest::CountKind(Guards, ECrowdKind::Skeleton), 0);
	TestEqual(TEXT("three Guards now"), ShrineArmyTest::CountKind(Guards, ECrowdKind::Guard), 3);

	// Buying several at once never spends the army: 28 bones buy three Guards, and "as many as possible" stops there.
	FCrowdSim Bulk;
	ShrineArmyTest::Make(Bulk, 28, 4, 0);
	TestEqual(TEXT("the most that can be bought at once is what the bones pay for"), Bulk.GetShopMax(ECrowdShopItem::Guard), 3);
	TestEqual(TEXT("asking for 40 buys 3"), Bulk.Buy(ECrowdShopItem::Guard, 40, CrowdTest::Origin), 3);
	TestEqual(TEXT("the four skeletons are untouched"), ShrineArmyTest::CountKind(Bulk, ECrowdKind::Skeleton), 4);
	TestEqual(TEXT("4 bones left"), Bulk.GetBonesHeld(), 4);
	// One more, on purpose: 4 bones and the four skeletons.
	TestEqual(TEXT("then one more can be bought with the army"), Bulk.Buy(ECrowdShopItem::Guard, 1, CrowdTest::Origin), 1);
	TestEqual(TEXT("the skeletons paid"), ShrineArmyTest::CountKind(Bulk, ECrowdKind::Skeleton), 0);
	TestEqual(TEXT("four Guards"), ShrineArmyTest::CountKind(Bulk, ECrowdKind::Guard), 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMeleeOneBlow, "WebTesting.Melee.IsOneBlowAtATime", WIZ_TEST_FLAGS)
bool FMeleeOneBlow::RunTest(const FString& Parameters)
{
	// A skeleton (8 damage a second, 20 health) and a grunt (5 a second, 10 health) stand in reach of each other,
	// beside the wizard.
	FCrowdSim Sim;
	CrowdTest::MakeEnemiesPlain(Sim);
	Sim.Tuning.SkeletonSpeed = 0.0f;
	Sim.Tuning.SkeletonRoamSpeed = 0.0f;
	Sim.Tuning.SkeletonCatchUpSpeed = 0.0f;
	Sim.Tuning.GruntSpeed = 0.0f;
	Sim.AddSkeleton(FVector2D(3000.0, 0.0));
	Sim.AddEnemy(ECrowdKind::Grunt, FVector2D(3040.0, 0.0));
	Sim.Command(FVector2D(3000.0, 0.0), 1, ECrowdCommandKind::Any);
	const FVector2D Wizard(2700.0, 0.0);
	const auto Health = [&Sim](ECrowdTeam Team)
	{
		const FCrowdUnit* Unit = CrowdTest::First(Sim, Team);
		return Unit ? Unit->Health : 0.0f;
	};
	const auto Run = [&Sim, Wizard](int32 Steps)
	{
		for (int32 Index = 0; Index < Steps; ++Index)
		{
			Sim.Step(0.05f, Wizard);
		}
	};
	Run(7);
	TestEqual(TEXT("0.35 seconds in, the grunt is unhurt: the blow has not landed"), Health(ECrowdTeam::Enemy), 10.0f, 0.01f);
	TestEqual(TEXT("and so is the skeleton"), Health(ECrowdTeam::Army), 20.0f, 0.01f);
	Run(7);
	TestEqual(TEXT("0.7 seconds in, one whole blow of 8 has landed on the grunt"), Health(ECrowdTeam::Enemy), 2.0f, 0.01f);
	TestEqual(TEXT("and one of 5 on the skeleton"), Health(ECrowdTeam::Army), 15.0f, 0.01f);
	Run(12);
	TestEqual(TEXT("1.3 seconds in, nothing more: the next blow is a second after the first"), Health(ECrowdTeam::Enemy), 2.0f, 0.01f);
	Run(8);
	TestEqual(TEXT("1.7 seconds in, the second blow has killed the grunt"), Sim.CountTeam(ECrowdTeam::Enemy), 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
