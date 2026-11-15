#include "Sim/ShrineState.h"
#include "Tests/CrowdTestHelpers.h"
#include "Tests/WizTestFlags.h"

#if WITH_DEV_AUTOMATION_TESTS

// Direction of 2026-11-12 (fourth round of the day):
// - enemies hold their ground: a garrison stands at its post until the wizard or his army comes near, or one of
//   them is hit, and then the whole group charges;
// - an area ends with its commander;
// - shrine upgrades are research, bought once: better armor, better weapons, better stats;
// - time runs at half speed while the shrine's menu is open.

namespace GarrisonTest
{
	void GiveBones(FCrowdSim& Sim, int32 Count, FVector2D Wizard = CrowdTest::Origin)
	{
		Sim.DropBone(Wizard, Count);
		Sim.Step(0.05f, Wizard);
	}

	void StepFor(FCrowdSim& Sim, float Seconds, FVector2D Wizard)
	{
		for (float Time = 0.0f; Time < Seconds; Time += 0.05f)
		{
			Sim.Step(0.05f, Wizard);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGarrisonStands, "WebTesting.Garrison.StandsUntilTheWizardComes", WIZ_TEST_FLAGS)
bool FGarrisonStands::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	TestTrue(TEXT("the post is manned"), Sim.AddGarrisonEnemy(ECrowdKind::Grunt, FVector2D(2000.0, 0.0), 1));
	GarrisonTest::StepFor(Sim, 3.0f, CrowdTest::Origin);
	const FCrowdUnit* Grunt = CrowdTest::First(Sim, ECrowdTeam::Enemy);
	if (!TestNotNull(TEXT("the grunt"), Grunt))
	{
		return false;
	}
	// It may walk about near its post (2026-11-12), but it does not leave it.
	TestTrue(TEXT("with the wizard 2000 away it has stayed at its post"), FVector2D::Distance(Grunt->Position, FVector2D(2000.0, 0.0)) <= Sim.Tuning.PostWanderRadius + 5.0);
	TestEqual(TEXT("it is still at its post"), Sim.CountAsleep(), 1);

	// 950 away: still outside the 900 it notices him at. 850 away: inside.
	GarrisonTest::StepFor(Sim, 1.0f, FVector2D(1050.0, 0.0));
	TestEqual(TEXT("950 away: still at its post"), Sim.CountAsleep(), 1);
	const FCrowdStepResult Result = Sim.Step(0.05f, FVector2D(1150.0, 0.0));
	TestEqual(TEXT("850 away: it wakes"), Sim.CountAsleep(), 0);
	TestEqual(TEXT("the step reports the group"), Result.SquadsWoken, 1);
	GarrisonTest::StepFor(Sim, 1.0f, FVector2D(1150.0, 0.0));
	Grunt = CrowdTest::First(Sim, ECrowdTeam::Enemy);
	TestTrue(TEXT("and charges him"), Grunt && Grunt->Position.X < 1900.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGarrisonSquadWakesTogether, "WebTesting.Garrison.TheWholeGroupWakesTogether", WIZ_TEST_FLAGS)
bool FGarrisonSquadWakesTogether::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	Sim.AddGarrisonEnemy(ECrowdKind::Grunt, FVector2D(2000.0, 0.0), 1);
	Sim.AddGarrisonEnemy(ECrowdKind::Brute, FVector2D(2700.0, 0.0), 1);
	Sim.AddGarrisonEnemy(ECrowdKind::Grunt, FVector2D(2700.0, 600.0), 2);
	TestEqual(TEXT("three at their posts"), Sim.CountAsleep(), 3);
	Sim.Step(0.05f, FVector2D(1200.0, 0.0));
	TestEqual(TEXT("the near group wakes whole, the other group stays"), Sim.CountAsleep(), 1);
	for (const FCrowdUnit& Unit : Sim.GetUnits())
	{
		TestEqual(TEXT("only group 2 is still at its post"), Unit.bAsleep, Unit.Squad == 2);
	}
	Sim.WakeAll();
	TestEqual(TEXT("everything can be woken"), Sim.CountAsleep(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGarrisonWakesWhenHit, "WebTesting.Garrison.WakesWhenHit", WIZ_TEST_FLAGS)
bool FGarrisonWakesWhenHit::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	Sim.Tuning.BruteSpeed = 0.0f;
	// Bolt has a range of 850 since 2026-11-12; this test is about the hit, so the range is lifted here.
	Sim.Tuning.BoltRange = 0.0f;
	Sim.AddGarrisonEnemy(ECrowdKind::Brute, FVector2D(2000.0, 0.0), 4);
	Sim.AddGarrisonEnemy(ECrowdKind::Brute, FVector2D(2300.0, 0.0), 4);
	Sim.FireBolt(CrowdTest::Origin, FVector2D(1.0, 0.0));
	GarrisonTest::StepFor(Sim, 1.2f, CrowdTest::Origin);
	TestEqual(TEXT("a Bolt from far away wakes the group it hits"), Sim.CountAsleep(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGarrisonWakesForArmy, "WebTesting.Garrison.WakesWhenASkeletonComesNear", WIZ_TEST_FLAGS)
bool FGarrisonWakesForArmy::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	Sim.AddGarrisonEnemy(ECrowdKind::Grunt, FVector2D(1500.0, 0.0), 1);
	Sim.Step(0.05f, CrowdTest::Origin);
	TestEqual(TEXT("the wizard is 1500 away: at its post"), Sim.CountAsleep(), 1);
	Sim.AddSkeleton(FVector2D(700.0, 0.0));
	Sim.Step(0.05f, CrowdTest::Origin);
	TestEqual(TEXT("a skeleton 800 away wakes it"), Sim.CountAsleep(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGarrisonSpawnedEnemiesAreAwake, "WebTesting.Garrison.SpawnedEnemiesStillCharge", WIZ_TEST_FLAGS)
bool FGarrisonSpawnedEnemiesAreAwake::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	Sim.AddEnemy(ECrowdKind::Grunt, FVector2D(2000.0, 0.0));
	TestEqual(TEXT("an enemy added the old way is not at a post"), Sim.CountAsleep(), 0);
	GarrisonTest::StepFor(Sim, 1.0f, CrowdTest::Origin);
	const FCrowdUnit* Grunt = CrowdTest::First(Sim, ECrowdTeam::Enemy);
	TestTrue(TEXT("it charges from any distance: a patrol"), Grunt && Grunt->Position.X < 1900.0);

	FVector2D Post;
	TestFalse(TEXT("no post is manned"), Sim.GetNearestPost(CrowdTest::Origin, Post));
	Sim.AddGarrisonEnemy(ECrowdKind::Grunt, FVector2D(0.0, 5000.0), 1);
	Sim.AddGarrisonEnemy(ECrowdKind::Grunt, FVector2D(0.0, 3000.0), 2);
	TestTrue(TEXT("a post is manned"), Sim.GetNearestPost(CrowdTest::Origin, Post));
	TestTrue(TEXT("the nearest one is found"), Post.Equals(FVector2D(0.0, 3000.0), 1.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommanderCapsArea, "WebTesting.Commander.IsToughAndItsDeathIsReported", WIZ_TEST_FLAGS)
bool FCommanderCapsArea::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	TestTrue(TEXT("far tougher than a brute"), Sim.GetStats(ECrowdKind::Commander).Health >= Sim.Tuning.BruteHealth * 4.0f);
	TestTrue(TEXT("hits harder than a brute"), Sim.GetStats(ECrowdKind::Commander).DamagePerSecond > Sim.Tuning.BruteDamagePerSecond);
	TestEqual(TEXT("leaves 15 bones"), Sim.GetBoneValue(ECrowdKind::Commander), 15);

	Sim.Tuning.CommanderSpeed = 0.0f;
	Sim.Tuning.CommanderHealth = 10.0f;
	Sim.AddGarrisonEnemy(ECrowdKind::Commander, FVector2D(300.0, 0.0), 9);
	Sim.FireBolt(CrowdTest::Origin, FVector2D(1.0, 0.0));
	int32 Commanders = 0;
	for (int32 Index = 0; Index < 10; ++Index)
	{
		Commanders += Sim.Step(0.05f, FVector2D(-3000.0, 0.0)).CommandersKilled;
	}
	TestEqual(TEXT("its death is reported once"), Commanders, 1);
	if (TestEqual(TEXT("it left one pile"), Sim.GetBones().Num(), 1))
	{
		TestEqual(TEXT("of 15 bones"), Sim.GetBones()[0].Value, 15);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRiverBlocksWalkingOnly, "WebTesting.Obstacles.WaterStopsFeetNotBolts", WIZ_TEST_FLAGS)
bool FRiverBlocksWalkingOnly::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	Sim.Tuning.GruntSpeed = 0.0f;
	FCrowdObstacle River;
	River.Center = FVector2D(400.0, 0.0);
	River.HalfSize = FVector2D(100.0, 2000.0);
	River.bLow = true;
	Sim.SetObstacles({ River });
	TestFalse(TEXT("nothing walks across water"), Sim.HasClearPath(CrowdTest::Origin, FVector2D(800.0, 0.0)));

	Sim.AddEnemy(ECrowdKind::Grunt, FVector2D(800.0, 0.0));
	Sim.FireBolt(CrowdTest::Origin, FVector2D(1.0, 0.0));
	GarrisonTest::StepFor(Sim, 0.6f, FVector2D(0.0, -3000.0));
	TestEqual(TEXT("a Bolt flies over water and kills the grunt on the far bank"), Sim.CountTeam(ECrowdTeam::Enemy), 0);

	Sim.SetSpearUnlocked(true);
	for (int32 Index = 0; Index < 3; ++Index)
	{
		Sim.AddSkeleton(FVector2D(-100.0, 0.0));
	}
	TestTrue(TEXT("the spear is thrown"), Sim.ThrowSpear(CrowdTest::Origin, FVector2D(800.0, 0.0)));
	TestTrue(TEXT("and it lands across the water, not at the bank"), Sim.GetSpears().Num() == 1 && Sim.GetSpears()[0].Target.X > 700.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FResearchIsBoughtOnce, "WebTesting.Research.EachIsBoughtOnce", WIZ_TEST_FLAGS)
bool FResearchIsBoughtOnce::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	Sim.AddSkeleton(FVector2D(-100.0, 0.0));
	Sim.AddSkeleton(FVector2D(-140.0, 0.0), ECrowdKind::Guard);
	const float BaseHealth = Sim.GetStats(ECrowdKind::Skeleton).Health;
	const float BaseDamage = Sim.GetStats(ECrowdKind::Skeleton).DamagePerSecond;
	TestFalse(TEXT("armor is not researched at the start"), Sim.IsResearched(ECrowdShopItem::Armor));
	TestEqual(TEXT("armor costs 15 bones"), Sim.GetShopCost(ECrowdShopItem::Armor), 15);

	GarrisonTest::GiveBones(Sim, 100);
	TestEqual(TEXT("asking for five buys one: research is bought once"), Sim.Buy(ECrowdShopItem::Armor, 5, CrowdTest::Origin), 1);
	TestTrue(TEXT("armor is researched"), Sim.IsResearched(ECrowdShopItem::Armor));
	TestEqual(TEXT("15 bones spent"), Sim.GetBonesHeld(), 85);
	TestEqual(TEXT("it cannot be bought again"), Sim.Buy(ECrowdShopItem::Armor, 1, CrowdTest::Origin), 0);
	TestEqual(TEXT("nothing more to buy"), Sim.GetShopMax(ECrowdShopItem::Armor), 0);
	TestEqual(TEXT("a plain skeleton is tougher"), Sim.GetStats(ECrowdKind::Skeleton).Health, BaseHealth + Sim.Tuning.ArmorHealth);
	TestEqual(TEXT("a guard is unchanged"), Sim.GetStats(ECrowdKind::Guard).Health, Sim.Tuning.GuardHealth);
	for (const FCrowdUnit& Unit : Sim.GetUnits())
	{
		TestEqual(TEXT("those already in the army get it too"), Unit.MaxHealth, Sim.GetStats(Unit.Kind).Health);
	}

	TestEqual(TEXT("better weapons are bought once"), Sim.Buy(ECrowdShopItem::Blades, 3, CrowdTest::Origin), 1);
	TestEqual(TEXT("a plain skeleton hits harder"), Sim.GetStats(ECrowdKind::Skeleton).DamagePerSecond, BaseDamage + Sim.Tuning.BladesDamage);
	TestEqual(TEXT("armor did not change the damage"), Sim.GetStats(ECrowdKind::Skeleton).Health, BaseHealth + Sim.Tuning.ArmorHealth);

	Sim.Reset();
	TestFalse(TEXT("a new run starts with nothing researched"), Sim.IsResearched(ECrowdShopItem::Armor) || Sim.IsResearched(ECrowdShopItem::Blades));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FResearchFocus, "WebTesting.Research.DarkFocusStrengthensBolt", WIZ_TEST_FLAGS)
bool FResearchFocus::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	Sim.Tuning.GruntSpeed = 0.0f;
	Sim.Tuning.GruntHealth = 15.0f;
	Sim.AddEnemy(ECrowdKind::Grunt, FVector2D(300.0, 0.0));
	Sim.FireBolt(CrowdTest::Origin, FVector2D(1.0, 0.0));
	GarrisonTest::StepFor(Sim, 0.5f, FVector2D(-3000.0, 0.0));
	TestEqual(TEXT("one plain Bolt does not kill a 15 health grunt"), Sim.CountTeam(ECrowdTeam::Enemy), 1);

	FCrowdSim Focused;
	Focused.Tuning.GruntSpeed = 0.0f;
	Focused.Tuning.GruntHealth = 15.0f;
	GarrisonTest::GiveBones(Focused, 15);
	TestEqual(TEXT("dark focus is bought"), Focused.Buy(ECrowdShopItem::Focus, 1, CrowdTest::Origin), 1);
	Focused.AddEnemy(ECrowdKind::Grunt, FVector2D(300.0, 0.0));
	Focused.FireBolt(CrowdTest::Origin, FVector2D(1.0, 0.0));
	GarrisonTest::StepFor(Focused, 0.5f, FVector2D(-3000.0, 0.0));
	TestEqual(TEXT("with dark focus one Bolt kills it"), Focused.CountTeam(ECrowdTeam::Enemy), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShrineSlowsTime, "WebTesting.Shrine.TimeRunsAtHalfSpeedWhileOpen", WIZ_TEST_FLAGS)
bool FShrineSlowsTime::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	TestEqual(TEXT("half speed by default"), Sim.Tuning.ShrineTimeScale, 0.5f);
	FShrineState Shrine;
	TestEqual(TEXT("closed: full speed"), Shrine.GetTimeScale(0.5f), 1.0f);
	Shrine.TryOpen(Shrine.Position);
	TestEqual(TEXT("open: half speed"), Shrine.GetTimeScale(0.5f), 0.5f);
	TestEqual(TEXT("a silly value cannot stop time"), Shrine.GetTimeScale(0.0f), 0.05f);
	TestEqual(TEXT("or speed it up"), Shrine.GetTimeScale(3.0f), 1.0f);
	Shrine.Update(Shrine.Position + FVector2D(5000.0, 0.0));
	TestEqual(TEXT("walked away: full speed again"), Shrine.GetTimeScale(0.5f), 1.0f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
