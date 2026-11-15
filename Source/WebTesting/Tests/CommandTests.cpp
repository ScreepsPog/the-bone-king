#include "Sim/CommandState.h"
#include "Sim/CrowdSim.h"
#include "Tests/CrowdTestHelpers.h"
#include "Tests/WizTestFlags.h"

#if WITH_DEV_AUTOMATION_TESTS

// Direction of 2026-11-13: the player sends part of the army to a place. The mouse wheel sets how many (1 to all),
// right mouse the kind, left mouse sends. Sent units hold the place, rejoin when the wizard comes near it, and can all
// be recalled.

namespace CommandTest
{
	void Run(FCrowdSim& Sim, float Seconds, FVector2D Wizard)
	{
		for (float Time = 0.0f; Time < Seconds; Time += 0.05f)
		{
			Sim.Step(0.05f, Wizard);
		}
	}

	int32 CountNear(const FCrowdSim& Sim, FVector2D Point, double Radius)
	{
		int32 Count = 0;
		for (const FCrowdUnit& Unit : Sim.GetUnits())
		{
			Count += Unit.Team == ECrowdTeam::Army && FVector2D::Distance(Unit.Position, Point) <= Radius ? 1 : 0;
		}
		return Count;
	}

	void AddArmy(FCrowdSim& Sim, int32 Skeletons, int32 Guards)
	{
		for (int32 Index = 0; Index < Skeletons + Guards; ++Index)
		{
			Sim.AddSkeleton(FVector2D(100.0 + Index * 10.0, 100.0), Index < Skeletons ? ECrowdKind::Skeleton : ECrowdKind::Guard);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommandSends, "WebTesting.Command.SendsTheNumberToThePlace", WIZ_TEST_FLAGS)
bool FCommandSends::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	CommandTest::AddArmy(Sim, 6, 0);
	const FVector2D Place(1500.0, 0.0);
	TestEqual(TEXT("all six can be sent"), Sim.CountCommandable(ECrowdCommandKind::Any), 6);
	TestEqual(TEXT("four are sent"), Sim.Command(Place, 4, ECrowdCommandKind::Any), 4);
	TestEqual(TEXT("four are out"), Sim.CountCommanded(), 4);
	TestEqual(TEXT("two can still be sent"), Sim.CountCommandable(ECrowdCommandKind::Any), 2);
	CommandTest::Run(Sim, 5.0f, CrowdTest::Origin);
	TestEqual(TEXT("four stand at the place"), CommandTest::CountNear(Sim, Place, 160.0), 4);
	TestEqual(TEXT("two stayed with the wizard"), CommandTest::CountNear(Sim, CrowdTest::Origin, 600.0), 2);
	TArray<FVector2D> Points;
	Sim.GetCommandPoints(Points);
	TestEqual(TEXT("one place is held"), Points.Num(), 1);
	TestEqual(TEXT("asking for more than there are sends what is left"), Sim.Command(Place, 10, ECrowdCommandKind::Any), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommandKind, "WebTesting.Command.SendsOnlyTheChosenKind", WIZ_TEST_FLAGS)
bool FCommandKind::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	CommandTest::AddArmy(Sim, 3, 2);
	TestEqual(TEXT("three skeletons can be sent"), Sim.CountCommandable(ECrowdCommandKind::Skeleton), 3);
	TestEqual(TEXT("two guards can be sent"), Sim.CountCommandable(ECrowdCommandKind::Guard), 2);
	TestEqual(TEXT("asking for five guards sends two"), Sim.Command(FVector2D(1500.0, 0.0), 5, ECrowdCommandKind::Guard), 2);
	for (const FCrowdUnit& Unit : Sim.GetUnits())
	{
		TestEqual(TEXT("only guards went"), Unit.bCommanded, Unit.Kind == ECrowdKind::Guard);
	}
	TestTrue(TEXT("the kinds have names"), FString(FCrowdSim::GetCommandKindName(ECrowdCommandKind::Guard)) == TEXT("Guard")
		&& FString(FCrowdSim::GetCommandKindName(ECrowdCommandKind::Any)) == TEXT("Any"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommandRejoin, "WebTesting.Command.RejoinWhenTheWizardComesNear", WIZ_TEST_FLAGS)
bool FCommandRejoin::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	CommandTest::AddArmy(Sim, 4, 0);
	const FVector2D Place(1500.0, 0.0);
	Sim.Command(Place, 4, ECrowdCommandKind::Any);
	CommandTest::Run(Sim, 4.0f, CrowdTest::Origin);
	TestEqual(TEXT("they hold while he is away"), Sim.CountCommanded(), 4);
	CommandTest::Run(Sim, 0.2f, FVector2D(1300.0, 0.0));
	TestEqual(TEXT("they rejoin when he is within the distance"), Sim.CountCommanded(), 0);

	// Sent to a place right beside him, they hold it until he has gone away and come back.
	FCrowdSim Near;
	CommandTest::AddArmy(Near, 2, 0);
	Near.Command(FVector2D(150.0, 0.0), 2, ECrowdCommandKind::Any);
	CommandTest::Run(Near, 1.0f, CrowdTest::Origin);
	TestEqual(TEXT("they hold a place beside him"), Near.CountCommanded(), 2);
	CommandTest::Run(Near, 0.2f, FVector2D(3000.0, 0.0));
	TestEqual(TEXT("they hold it while he walks off"), Near.CountCommanded(), 2);
	CommandTest::Run(Near, 0.2f, CrowdTest::Origin);
	TestEqual(TEXT("they rejoin when he comes back"), Near.CountCommanded(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommandRecall, "WebTesting.Command.RecallBringsEveryoneBack", WIZ_TEST_FLAGS)
bool FCommandRecall::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	CommandTest::AddArmy(Sim, 6, 0);
	Sim.Command(FVector2D(1500.0, 0.0), 2, ECrowdCommandKind::Any);
	Sim.Command(FVector2D(0.0, 1500.0), 3, ECrowdCommandKind::Any);
	TestEqual(TEXT("five are out at two places"), Sim.CountCommanded(), 5);
	TestEqual(TEXT("recall calls five"), Sim.Recall(), 5);
	TestEqual(TEXT("none is out"), Sim.CountCommanded(), 0);
	CommandTest::Run(Sim, 6.0f, CrowdTest::Origin);
	TestEqual(TEXT("all six are back with the wizard"), CommandTest::CountNear(Sim, CrowdTest::Origin, 600.0), 6);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommandFights, "WebTesting.Command.SentUnitsFightWhereTheyStand", WIZ_TEST_FLAGS)
bool FCommandFights::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	CrowdTest::MakeEnemiesPlain(Sim);
	Sim.Tuning.GruntSpeed = 0.0f;
	CommandTest::AddArmy(Sim, 4, 0);
	const FVector2D Place(3000.0, 0.0);
	Sim.AddEnemy(ECrowdKind::Grunt, Place + FVector2D(150.0, 0.0));
	Sim.Command(Place, 4, ECrowdCommandKind::Any);
	// The wizard stays far away: the enemy is well outside his leash.
	CommandTest::Run(Sim, 12.0f, CrowdTest::Origin);
	TestEqual(TEXT("the enemy beside the place is dead"), Sim.CountTeam(ECrowdTeam::Enemy), 0);
	TestEqual(TEXT("they still hold the place"), Sim.CountCommanded(), 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommandModeState, "WebTesting.Command.WheelSetsTheNumberRightMouseTheKind", WIZ_TEST_FLAGS)
bool FCommandModeState::RunTest(const FString& Parameters)
{
	FCommandState State;
	TestFalse(TEXT("no command mode with no army"), State.Enter(0));
	TestFalse(TEXT("it stays out"), State.IsActive());
	TestTrue(TEXT("it enters with an army"), State.Enter(9));
	TestTrue(TEXT("it is on"), State.IsActive());
	TestEqual(TEXT("it starts at half, rounded up"), State.GetCount(), 5);
	TestTrue(TEXT("it starts on any kind"), State.GetKind() == ECrowdCommandKind::Any);
	State.Scroll(2, 9);
	TestEqual(TEXT("wheel up adds"), State.GetCount(), 7);
	State.Scroll(50, 9);
	TestEqual(TEXT("never more than the army"), State.GetCount(), 9);
	State.Scroll(-50, 9);
	TestEqual(TEXT("never fewer than one"), State.GetCount(), 1);
	TestTrue(TEXT("right mouse goes to skeletons"), State.CycleKind() == ECrowdCommandKind::Skeleton);
	TestTrue(TEXT("then guards"), State.CycleKind() == ECrowdCommandKind::Guard);
	TestTrue(TEXT("then any again"), State.CycleKind() == ECrowdCommandKind::Any);
	// Direction of 2026-11-13 (14:30 PST): a kind the army has none of cannot be chosen.
	const int32 NoGuards[] = { 9, 9, 0 };
	TestTrue(TEXT("with no guards: skeletons"), State.CycleKind(NoGuards) == ECrowdCommandKind::Skeleton);
	TestTrue(TEXT("then straight back to any, past the guards"), State.CycleKind(NoGuards) == ECrowdCommandKind::Any);
	const int32 OnlyGuards[] = { 4, 0, 4 };
	TestTrue(TEXT("with no plain skeletons: guards"), State.CycleKind(OnlyGuards) == ECrowdCommandKind::Guard);
	TestTrue(TEXT("then any"), State.CycleKind(OnlyGuards) == ECrowdCommandKind::Any);
	State.Scroll(8, 9);
	State.Clamp(3);
	TestEqual(TEXT("the number is held to what the kind has"), State.GetCount(), 3);
	State.Clamp(0);
	TestEqual(TEXT("none when the kind has none"), State.GetCount(), 0);
	State.Clamp(4);
	TestEqual(TEXT("back to one when there are some again"), State.GetCount(), 1);
	State.Exit();
	TestFalse(TEXT("it leaves"), State.IsActive());
	return true;
}

#endif
