#include "Sim/CrowdSim.h"
#include "Sim/OverwhelmMeter.h"
#include "Tests/CrowdTestHelpers.h"
#include "Tests/WizTestFlags.h"

#if WITH_DEV_AUTOMATION_TESTS

// Direction of 2026-11-13: archers shoot from a distance, hit harder than a low-tier enemy, are weaker, and run when the
// wizard or his army come at them. Assassins are fast and frail, ignore the army, go around it for the wizard, and are
// cloaked until they are close.

namespace ArcherTest
{
	/** Steps the crowd for Seconds and adds up what happened. */
	FCrowdStepResult Run(FCrowdSim& Sim, float Seconds, FVector2D Wizard = CrowdTest::Origin)
	{
		FCrowdStepResult Total;
		for (float Time = 0.0f; Time < Seconds; Time += 0.05f)
		{
			const FCrowdStepResult Result = Sim.Step(0.05f, Wizard);
			Total.ArrowsFired += Result.ArrowsFired;
			Total.ArrowsHitWizard += Result.ArrowsHitWizard;
			Total.EnemiesKilled += Result.EnemiesKilled;
			Total.Rejoined += Result.Rejoined;
		}
		return Total;
	}

	const FCrowdUnit* FirstOf(const FCrowdSim& Sim, ECrowdKind Kind)
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArcherShoots, "WebTesting.Archer.ShootsTheWizardFromADistance", WIZ_TEST_FLAGS)
bool FArcherShoots::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	CrowdTest::MakeEnemiesPlain(Sim);
	Sim.AddEnemy(ECrowdKind::Archer, FVector2D(600.0, 0.0));
	const FCrowdStepResult Total = ArcherTest::Run(Sim, 4.0f);
	TestTrue(TEXT("it loosed arrows"), Total.ArrowsFired >= 2);
	TestTrue(TEXT("arrows hit the wizard"), Total.ArrowsHitWizard >= 1);
	const FCrowdUnit* Archer = ArcherTest::FirstOf(Sim, ECrowdKind::Archer);
	TestTrue(TEXT("it kept its distance"), Archer && Archer->Position.Size() > 400.0);
	TestEqual(TEXT("it is not on the wizard"), CrowdTest::CountTouching(Sim, CrowdTest::Origin), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArcherStats, "WebTesting.Archer.WeakerThanAGruntButHitsHarder", WIZ_TEST_FLAGS)
bool FArcherStats::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	CrowdTest::MakeEnemiesPlain(Sim);
	CrowdTest::FreezeArmy(Sim);
	TestTrue(TEXT("less health than a grunt"), Sim.GetStats(ECrowdKind::Archer).Health < Sim.GetStats(ECrowdKind::Grunt).Health);
	TestTrue(TEXT("an arrow does more to a skeleton than a grunt does in a second"), Sim.Tuning.ArrowDamage > Sim.Tuning.GruntDamagePerSecond);
	TestTrue(TEXT("an arrow fills more of the meter than a grunt does in a second"), Sim.Tuning.ArrowOverwhelm > FOverwhelmTuning().FillPerEnemyPerSecond);
	TestEqual(TEXT("an archer leaves bones"), Sim.GetBoneValue(ECrowdKind::Archer), 1);

	// The wizard is far away; a skeleton stands in range of the archer and outside the distance it runs at.
	Sim.AddEnemy(ECrowdKind::Archer, FVector2D(0.0, 0.0));
	Sim.AddSkeleton(FVector2D(0.0, -500.0));
	ArcherTest::Run(Sim, 1.2f, FVector2D(0.0, 6000.0));
	const FCrowdUnit* Skeleton = CrowdTest::First(Sim, ECrowdTeam::Army);
	TestTrue(TEXT("its arrows hurt the skeleton"), Skeleton && Skeleton->Health <= Skeleton->MaxHealth - Sim.Tuning.ArrowDamage + 0.01f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArcherFlees, "WebTesting.Archer.RunsWhenChargedAndDoesNotShoot", WIZ_TEST_FLAGS)
bool FArcherFlees::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	CrowdTest::MakeEnemiesPlain(Sim);
	Sim.AddEnemy(ECrowdKind::Archer, FVector2D(300.0, 0.0));
	const FCrowdStepResult Close = ArcherTest::Run(Sim, 0.25f);
	TestEqual(TEXT("it does not shoot with the wizard on it"), Close.ArrowsFired, 0);
	ArcherTest::Run(Sim, 0.75f);
	const FCrowdUnit* Archer = ArcherTest::FirstOf(Sim, ECrowdKind::Archer);
	TestTrue(TEXT("it ran from the wizard"), Archer && Archer->Position.X > 400.0);

	FCrowdSim FromArmy;
	CrowdTest::MakeEnemiesPlain(FromArmy);
	CrowdTest::FreezeArmy(FromArmy);
	FromArmy.AddEnemy(ECrowdKind::Archer, FVector2D(0.0, 0.0));
	FromArmy.AddSkeleton(FVector2D(-200.0, 0.0));
	ArcherTest::Run(FromArmy, 1.0f, FVector2D(0.0, 6000.0));
	const FCrowdUnit* Chased = ArcherTest::FirstOf(FromArmy, ECrowdKind::Archer);
	TestTrue(TEXT("it ran from the skeleton"), Chased && Chased->Position.X > 100.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArcherBlocked, "WebTesting.Archer.ArrowsNeedAClearLineAndMissABlink", WIZ_TEST_FLAGS)
bool FArcherBlocked::RunTest(const FString& Parameters)
{
	FCrowdSim Walled;
	CrowdTest::MakeEnemiesPlain(Walled);
	Walled.Tuning.ArcherSpeed = 0.0f;
	Walled.SetObstacles({ CrowdTest::Wall(300.0, 0.0, 30.0, 600.0) });
	Walled.AddEnemy(ECrowdKind::Archer, FVector2D(600.0, 0.0));
	TestEqual(TEXT("no arrow is loosed at a wall"), ArcherTest::Run(Walled, 3.0f).ArrowsFired, 0);

	FCrowdSim Blinking;
	CrowdTest::MakeEnemiesPlain(Blinking);
	Blinking.bWizardInvulnerable = true;
	Blinking.AddEnemy(ECrowdKind::Archer, FVector2D(600.0, 0.0));
	const FCrowdStepResult Total = ArcherTest::Run(Blinking, 4.0f);
	TestTrue(TEXT("arrows are loosed"), Total.ArrowsFired >= 2);
	TestEqual(TEXT("none hits him during the blink"), Total.ArrowsHitWizard, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAssassinIgnoresArmy, "WebTesting.Assassin.GoesAroundTheArmyForTheWizard", WIZ_TEST_FLAGS)
bool FAssassinIgnoresArmy::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	CrowdTest::MakeEnemiesPlain(Sim);
	CrowdTest::FreezeArmy(Sim);
	Sim.AddEnemy(ECrowdKind::Assassin, FVector2D(700.0, 0.0));
	// A skeleton right in its way: a grunt would stop here and fight.
	Sim.AddSkeleton(FVector2D(350.0, 0.0));
	ArcherTest::Run(Sim, 2.5f);
	const FCrowdUnit* Skeleton = CrowdTest::First(Sim, ECrowdTeam::Army);
	TestTrue(TEXT("the skeleton was not touched"), Skeleton && Skeleton->Health >= Skeleton->MaxHealth);
	TestTrue(TEXT("the assassin is on the wizard"), CrowdTest::CountTouching(Sim, CrowdTest::Origin) >= 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAssassinStats, "WebTesting.Assassin.FastFrailAndDeadly", WIZ_TEST_FLAGS)
bool FAssassinStats::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	CrowdTest::MakeEnemiesPlain(Sim);
	const FCrowdKindStats Assassin = Sim.GetStats(ECrowdKind::Assassin);
	TestTrue(TEXT("faster than a grunt"), Assassin.Speed > Sim.GetStats(ECrowdKind::Grunt).Speed * 1.5f);
	TestTrue(TEXT("faster than a skeleton"), Assassin.Speed > Sim.GetStats(ECrowdKind::Skeleton).Speed);
	TestTrue(TEXT("no more health than a grunt"), Assassin.Health <= Sim.GetStats(ECrowdKind::Grunt).Health);
	Sim.AddEnemy(ECrowdKind::Assassin, FVector2D(40.0, 0.0));
	Sim.AddEnemy(ECrowdKind::Grunt, FVector2D(-40.0, 0.0));
	TestEqual(TEXT("on the wizard, one assassin counts as four enemies beside a grunt's one"), CrowdTest::CountTouching(Sim, CrowdTest::Origin), 5);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAssassinCloak, "WebTesting.Assassin.CloakedUntilClose", WIZ_TEST_FLAGS)
bool FAssassinCloak::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	CrowdTest::MakeEnemiesPlain(Sim);
	Sim.Tuning.AssassinSpeed = 0.0f;
	Sim.AddEnemy(ECrowdKind::Assassin, FVector2D(250.0, 0.0));
	Sim.AddEnemy(ECrowdKind::Grunt, FVector2D(5000.0, 0.0));
	const FCrowdUnit& Assassin = Sim.GetUnits()[0];
	TestTrue(TEXT("cloaked far from the wizard"), Sim.IsCloaked(Assassin, FVector2D(-2000.0, 0.0)));
	TestFalse(TEXT("seen close to the wizard"), Sim.IsCloaked(Assassin, CrowdTest::Origin));
	TestEqual(TEXT("fully shown when close"), Sim.GetShown(Assassin, CrowdTest::Origin), 1.0f, 0.001f);
	const float Edge = Sim.GetShown(Assassin, FVector2D(250.0 - Sim.Tuning.AssassinRevealRange + 60.0, 0.0));
	TestTrue(TEXT("it fades in at the edge of the range"), Edge > 0.0f && Edge < 1.0f);
	TestFalse(TEXT("a grunt is never cloaked"), Sim.IsCloaked(Sim.GetUnits()[1], CrowdTest::Origin));

	// Direction of 2026-11-13 (14:30 PST): a more gradual fade, to give the player more warning. It starts to show
	// well before the old 450 units, is half shown about there, and is whole only when it is nearly on him.
	const float Far = Sim.GetShown(Assassin, FVector2D(250.0 - 650.0, 0.0));
	TestTrue(TEXT("650 units out it already shows faintly"), Far > 0.05f && Far < 0.5f);
	const float Mid = Sim.GetShown(Assassin, FVector2D(250.0 - 450.0, 0.0));
	TestTrue(TEXT("450 units out it is partly shown, more than at 650"), Mid > Far && Mid < 0.9f);
	TestTrue(TEXT("the fade is spread over at least 400 units"), Sim.Tuning.AssassinRevealFade >= 400.0f);

	// The army cannot see an assassin until it is within AssassinArmyRange of the wizard, whatever the player can see.
	// A skeleton stands beside it. (This part of the test changed with the rule: it used the reveal range before.)
	Sim.AddSkeleton(FVector2D(200.0, 0.0));
	Sim.Tuning.AssassinArmyRange = 100.0f;
	ArcherTest::Run(Sim, 2.0f);
	TestTrue(TEXT("the skeleton leaves an assassin it cannot see alone"), Sim.GetUnits()[0].Health >= Sim.GetUnits()[0].MaxHealth);
	Sim.Tuning.AssassinArmyRange = 450.0f;
	ArcherTest::Run(Sim, 2.0f);
	TestTrue(TEXT("and attacks it once it is seen"), Sim.GetUnits().Num() < 3 || Sim.GetUnits()[0].Health < Sim.GetUnits()[0].MaxHealth);
	return true;
}

#endif
