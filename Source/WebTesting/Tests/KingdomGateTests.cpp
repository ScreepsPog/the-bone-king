#include "Sim/KingdomGate.h"
#include "Sim/LevelLayout.h"
#include "Tests/WizTestFlags.h"

#if WITH_DEV_AUTOMATION_TESTS

// Direction of 2026-11-14 (10:42 PST): the game is one level, and it finishes when the wizard enters the kingdom
// gates. The gate opens only once the commander is dead.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGateStaysShut, "WebTesting.Gate.ItIsShutWhileTheCommanderLives", WIZ_TEST_FLAGS)
bool FGateStaysShut::RunTest(const FString& Parameters)
{
	FKingdomGate Gate;
	Gate.Place(FVector2D(1000.0, 0.0), FVector2D(150.0, 300.0), 0.0f);
	TestTrue(TEXT("the gate is in the level"), Gate.IsPlaced());
	for (int32 Step = 0; Step < 40; ++Step)
	{
		TestFalse(TEXT("standing in a shut gate ends nothing"), Gate.Update(false, FVector2D(1000.0, 0.0), true, 0.05f));
	}
	TestFalse(TEXT("it is shut"), Gate.IsOpen());
	TestFalse(TEXT("and nobody has entered"), Gate.IsEntered());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGateEndsTheGame, "WebTesting.Gate.EnteringItOnceTheCommanderIsDeadEndsTheGame", WIZ_TEST_FLAGS)
bool FGateEndsTheGame::RunTest(const FString& Parameters)
{
	FKingdomGate Gate;
	Gate.Place(FVector2D(1000.0, 0.0), FVector2D(150.0, 300.0), 0.0f);
	TestFalse(TEXT("the commander dies with the wizard far off: nothing ends"), Gate.Update(true, FVector2D(0.0, 0.0), true, 0.05f));
	TestTrue(TEXT("but the gate is open"), Gate.IsOpen());
	TestFalse(TEXT("a wizard who is falling does not enter"), Gate.Update(true, FVector2D(1000.0, 0.0), false, 0.05f));
	TestFalse(TEXT("just outside it is not in it"), Gate.Update(true, FVector2D(840.0, 0.0), true, 0.05f));
	TestTrue(TEXT("walking in ends the game, said once"), Gate.Update(true, FVector2D(900.0, 250.0), true, 0.05f));
	TestTrue(TEXT("he has entered"), Gate.IsEntered());
	TestFalse(TEXT("it is not said again"), Gate.Update(true, FVector2D(900.0, 250.0), true, 0.5f));
	TestEqual(TEXT("the time since is counted"), Gate.GetEnteredSeconds(), 0.5f);
	TestTrue(TEXT("walking out does not undo it"), Gate.IsEntered() && !Gate.Update(true, FVector2D(0.0, 0.0), true, 0.5f));

	Gate.Reset();
	TestTrue(TEXT("a new run: the gate still stands"), Gate.IsPlaced());
	TestFalse(TEXT("shut again"), Gate.IsOpen());
	TestFalse(TEXT("and not entered"), Gate.IsEntered());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGateTurned, "WebTesting.Gate.ATurnedGateIsEnteredWhereItStands", WIZ_TEST_FLAGS)
bool FGateTurned::RunTest(const FString& Parameters)
{
	// 100 deep and 600 wide, turned a quarter: now 600 along X and 100 along Y.
	FKingdomGate Gate;
	Gate.Place(FVector2D(0.0, 0.0), FVector2D(50.0, 300.0), 90.0f);
	TestTrue(TEXT("far along its width"), Gate.Contains(FVector2D(280.0, 0.0)));
	TestFalse(TEXT("not past its depth"), Gate.Contains(FVector2D(0.0, 280.0)));

	FKingdomGate None;
	TestFalse(TEXT("a level with no gate never ends by one"), None.Update(true, FVector2D(0.0, 0.0), true, 0.05f));
	TestFalse(TEXT("and it is not open"), None.IsOpen());

	FKingdomGate Forced;
	Forced.Place(FVector2D(0.0, 0.0), FVector2D(100.0, 100.0), 0.0f);
	Forced.Open();
	TestTrue(TEXT("a gate opened by a tester is entered with the commander alive"), Forced.Update(false, FVector2D(0.0, 0.0), true, 0.05f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutskirtsHasGate, "WebTesting.Level.TheOutskirtsEndsAtAGateBeyondTheCommander", WIZ_TEST_FLAGS)
bool FOutskirtsHasGate::RunTest(const FString& Parameters)
{
	const FLevelLayout Layout = FLevelLayout::MakeOutskirts();
	if (!TestTrue(TEXT("the Outskirts has a gate"), Layout.bHasGate))
	{
		return false;
	}
	for (const FCrowdObstacle& Obstacle : Layout.MakeObstacles())
	{
		TestFalse(TEXT("its middle is in the open"), Obstacle.Contains(Layout.Gate, 60.0));
	}
	TestTrue(TEXT("the wizard can walk from the commander to it"), Layout.CanWalk(Layout.Commander, Layout.Gate));
	const double Apart = FVector2D::Distance(Layout.Commander, Layout.Gate);
	TestTrue(TEXT("it is past the commander's ground, not on it"), Apart > 500.0);
	TestTrue(TEXT("and farther from the start than he is"), FVector2D::Distance(Layout.Start, Layout.Gate) > FVector2D::Distance(Layout.Start, Layout.Commander));
	TestFalse(TEXT("the wizard does not start in it"), FVector2D::Distance(Layout.Start, Layout.Gate) < 2000.0);
	return true;
}

#endif
