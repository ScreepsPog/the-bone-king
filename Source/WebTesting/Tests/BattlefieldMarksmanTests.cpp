#include "Engine/Level.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Game/CrowdSubsystem.h"
#include "Game/LevelActors.h"
#include "Materials/MaterialParameterCollection.h"
#include "Sim/Battlefield.h"
#include "Sim/OverwhelmMeter.h"
#include "Tests/CrowdTestHelpers.h"
#include "Tests/WizTestFlags.h"

#if WITH_DEV_AUTOMATION_TESTS

// Direction of 2026-11-14 19:40 PST:
// - Marksmen "stand in place" to shoot, "then catch up to the player"; they "try to be far from the enemy. If an
//   enemy get's close they'll try to relocate, but not always."
// - "don't allow for guards/marksman to be converted in the army, nor count towards the bone currency."
// - "change the AI to walk on my collision boxes for the bridge and ensure they still correctly go around houses as
//   I also updated those collision boxes."
// - "Replace the walls with just a soft barrier, that warns the player to return to the battlefield, with a failing
//   of the mission if they wait to long or go to far."

namespace FieldTest
{
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

	bool Blocks(const TArray<FCrowdObstacle>& Boxes, FVector2D Point)
	{
		return Boxes.ContainsByPredicate([Point](const FCrowdObstacle& Box) { return Box.Contains(Point, 0.0); });
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBattlefieldEdge, "WebTesting.Battlefield.ItsEdgeWarnsThenFails", WIZ_TEST_FLAGS)
bool FBattlefieldEdge::RunTest(const FString& Parameters)
{
	FBattlefield None;
	TestFalse(TEXT("a level with no battlefield never fails him for where he stands"), None.Update(FVector2D(1.e6, 0.0), 100.0f, 6.0f, 900.0f));

	FBattlefield Field;
	Field.Place(FVector2D(1000.0, 0.0), FVector2D(2000.0, 1000.0));
	TestEqual(TEXT("inside is not outside"), Field.GetOutside(FVector2D(2900.0, 900.0)), 0.0);
	TestEqual(TEXT("past an edge: how far past it"), Field.GetOutside(FVector2D(3250.0, 0.0)), 250.0, 0.01);
	TestEqual(TEXT("past a corner: how far from the corner"), Field.GetOutside(FVector2D(3300.0, 1400.0)), 500.0, 0.01);

	TestFalse(TEXT("inside: nothing"), Field.Update(FVector2D(0.0, 0.0), 1.0f, 6.0f, 900.0f));
	TestFalse(TEXT("and no warning"), Field.IsWarning());
	TestFalse(TEXT("a step outside: warned, not failed"), Field.Update(FVector2D(3100.0, 0.0), 1.0f, 6.0f, 900.0f));
	TestTrue(TEXT("the warning is up"), Field.IsWarning());
	TestEqual(TEXT("with the time he has left"), Field.GetSecondsLeft(6.0f), 5.0f, 0.001f);
	TestFalse(TEXT("back inside: the warning is gone"), Field.Update(FVector2D(2900.0, 0.0), 1.0f, 6.0f, 900.0f) || Field.IsWarning());
	bool bFailed = false;
	for (int32 Step = 0; Step < 5; ++Step)
	{
		bFailed = Field.Update(FVector2D(3100.0, 0.0), 1.0f, 6.0f, 900.0f);
	}
	TestFalse(TEXT("five seconds outside: not yet"), bFailed);
	TestTrue(TEXT("six seconds outside: the mission fails"), Field.Update(FVector2D(3100.0, 0.0), 1.0f, 6.0f, 900.0f));

	FBattlefield Far;
	Far.Place(FVector2D(1000.0, 0.0), FVector2D(2000.0, 1000.0));
	TestTrue(TEXT("too far outside: the mission fails at once"), Far.Update(FVector2D(3950.0, 0.0), 0.05f, 6.0f, 900.0f));

	// The fall it causes, whatever the meter held; not in god mode.
	FOverwhelmMeter Meter;
	TestTrue(TEXT("leaving the battlefield starts the fall"), Meter.Fall() && Meter.IsFalling());
	FOverwhelmMeter God;
	God.bGodMode = true;
	TestFalse(TEXT("not for a tester in god mode"), God.Fall() || God.IsFalling());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCollisionFootprints, "WebTesting.Level.CollisionShapesAreWhatBlocks", WIZ_TEST_FLAGS)
bool FCollisionFootprints::RunTest(const FString& Parameters)
{
	// A square shape, square to the level: one box, exactly it.
	TArray<FCrowdObstacle> Square;
	const double SquareWidth = UCrowdSubsystem::MakeFootprintObstacles({ FVector2D(0.0, 0.0), FVector2D(400.0, 0.0), FVector2D(400.0, 300.0), FVector2D(0.0, 300.0), FVector2D(200.0, 150.0) }, 0.0, Square);
	TestEqual(TEXT("one box"), Square.Num(), 1);
	TestTrue(TEXT("the shape's own"), Square.Num() == 1 && Square[0].Center.Equals(FVector2D(200.0, 150.0), 0.01) && Square[0].HalfSize.Equals(FVector2D(200.0, 150.0), 0.01));
	TestTrue(TEXT("its width is its narrow way, roughly"), SquareWidth > 200.0 && SquareWidth < 300.0);

	// A house wall piece turned 45 degrees: 1000 long, 600 wide.
	TArray<FVector2D> Turned;
	for (const FVector2D& Corner : { FVector2D(-500.0, -300.0), FVector2D(500.0, -300.0), FVector2D(500.0, 300.0), FVector2D(-500.0, 300.0) })
	{
		Turned.Add(Corner.GetRotated(45.0));
	}
	TArray<FCrowdObstacle> House;
	UCrowdSubsystem::MakeFootprintObstacles(Turned, 0.0, House);
	TestTrue(TEXT("a turned shape is many small boxes"), House.Num() >= 8);
	// (Not the exact middle: with an even number of boxes that is the line between two of them.)
	TestTrue(TEXT("its middle is blocked"), FieldTest::Blocks(House, FVector2D(30.0, 20.0)));
	TestTrue(TEXT("and near its far end"), FieldTest::Blocks(House, FVector2D(420.0, 0.0).GetRotated(45.0)));
	TestFalse(TEXT("the open ground at the corner of the box around it is not"), FieldTest::Blocks(House, FVector2D(520.0, -520.0)));
	TestFalse(TEXT("nor a step outside its long side"), FieldTest::Blocks(House, FVector2D(0.0, 420.0).GetRotated(45.0)));

	// A bridge: a rail 1000 long and 40 thick, turned, blocks along its length; the deck, 600 wide, is walked on.
	TArray<FVector2D> Rail;
	TArray<FVector2D> Deck;
	for (const FVector2D& Corner : { FVector2D(-500.0, -20.0), FVector2D(500.0, -20.0), FVector2D(500.0, 20.0), FVector2D(-500.0, 20.0) })
	{
		Rail.Add(Corner.GetRotated(30.0) + FVector2D(0.0, 320.0).GetRotated(30.0));
	}
	for (const FVector2D& Corner : { FVector2D(-500.0, -300.0), FVector2D(500.0, -300.0), FVector2D(500.0, 300.0), FVector2D(-500.0, 300.0) })
	{
		Deck.Add(Corner.GetRotated(30.0));
	}
	TArray<FCrowdObstacle> Bridge;
	const double DeckWidth = UCrowdSubsystem::MakeFootprintObstacles(Deck, UCrowdSubsystem::WalkOnWidth, Bridge);
	TestTrue(TEXT("the deck is wide"), DeckWidth > UCrowdSubsystem::WalkOnWidth);
	TestEqual(TEXT("so it blocks nothing: it is walked on"), Bridge.Num(), 0);
	const double RailWidth = UCrowdSubsystem::MakeFootprintObstacles(Rail, UCrowdSubsystem::WalkOnWidth, Bridge);
	TestTrue(TEXT("the rail is narrow"), RailWidth < UCrowdSubsystem::WalkOnWidth);
	TestTrue(TEXT("so it blocks"), Bridge.Num() >= 6);
	for (const double Along : { -400.0, -150.0, 100.0, 380.0 })
	{
		TestTrue(TEXT("all along its length"), FieldTest::Blocks(Bridge, FVector2D(Along, 320.0).GetRotated(30.0)));
	}
	TestFalse(TEXT("and the middle of the deck is open"), FieldTest::Blocks(Bridge, FVector2D(0.0, 0.0)));

	// The meshes in the project: the director's collision is there to read.
	const UStaticMesh* BridgeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Props/SM_Bridge.SM_Bridge"));
	if (TestNotNull(TEXT("the bridge"), BridgeMesh))
	{
		TArray<FCrowdObstacle> Rails;
		UCrowdSubsystem::MakeCollisionObstacles(*BridgeMesh, FTransform::Identity, true, Rails);
		AddInfo(FString::Printf(TEXT("the bridge blocks %d small boxes (its rails)"), Rails.Num()));
		TestFalse(TEXT("the middle of the bridge's deck is open to walk on"), FieldTest::Blocks(Rails, FVector2D(0.0, 0.0)));
	}
	const UStaticMesh* Barn = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Buildings/SM_Barn.SM_Barn"));
	if (TestNotNull(TEXT("the barn"), Barn))
	{
		TArray<FCrowdObstacle> Walls;
		UCrowdSubsystem::MakeCollisionObstacles(*Barn, FTransform::Identity, false, Walls);
		TestTrue(TEXT("the barn blocks"), Walls.Num() >= 1);
		TestTrue(TEXT("its middle"), FieldTest::Blocks(Walls, FVector2D(0.0, 0.0)));
		const FBox Bounds = Barn->GetBoundingBox();
		TestFalse(TEXT("and nothing well outside the model"), FieldTest::Blocks(Walls, FVector2D(Bounds.Max.X + 150.0, Bounds.Max.Y + 150.0)));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarksmanStands, "WebTesting.Marksman.StandsToShootThenCatchesUpAndKeepsAway", WIZ_TEST_FLAGS)
bool FMarksmanStands::RunTest(const FString& Parameters)
{
	// It stands to shoot while the wizard walks away, then catches up.
	FCrowdSim Sim;
	CrowdTest::MakeEnemiesPlain(Sim);
	Sim.Tuning.GruntSpeed = 0.0f;
	Sim.Tuning.GruntHealth = 100000.0f;
	Sim.Tuning.MarksmanRelocateChance = 0.0f;
	Sim.AddSkeleton(FVector2D(100.0, 0.0), ECrowdKind::Marksman);
	Sim.AddEnemy(ECrowdKind::Grunt, FVector2D(700.0, 0.0));
	FVector2D Wizard = CrowdTest::Origin;
	int32 Arrows = 0;
	double MovedWhileDrawing = 0.0;
	bool bDrew = false;
	bool bMovedAfter = false;
	FVector2D Last = FieldTest::FirstOf(Sim, ECrowdKind::Marksman)->Position;
	for (int32 Step = 0; Step < 120; ++Step)
	{
		// He walks away from the enemy, slowly enough that it stays in the Marksman's range for a while.
		Wizard.X -= 200.0 * 0.05;
		const FCrowdUnit* Before = FieldTest::FirstOf(Sim, ECrowdKind::Marksman);
		const bool bDrawing = Before && Before->bStriking;
		Arrows += Sim.Step(0.05f, Wizard).ArmyArrowsFired;
		const FCrowdUnit* Marksman = FieldTest::FirstOf(Sim, ECrowdKind::Marksman);
		if (!Marksman)
		{
			break;
		}
		const double Moved = FVector2D::Distance(Marksman->Position, Last);
		if (bDrawing)
		{
			bDrew = true;
			MovedWhileDrawing = FMath::Max(MovedWhileDrawing, Moved);
		}
		else if (Arrows > 0 && Moved > 1.0)
		{
			bMovedAfter = true;
		}
		Last = Marksman->Position;
	}
	TestTrue(TEXT("it shot"), Arrows >= 1);
	TestTrue(TEXT("it stood to draw"), bDrew);
	TestTrue(TEXT("and did not move with the wizard while drawing"), MovedWhileDrawing < 2.0);
	TestTrue(TEXT("after an arrow it went after him"), bMovedAfter);

	// An enemy comes close: with the chance at 1 it backs away from it; at 0 it stands its ground.
	const auto Gap = [](float Chance)
	{
		FCrowdSim Close;
		CrowdTest::MakeEnemiesPlain(Close);
		Close.Tuning.GruntSpeed = 0.0f;
		Close.Tuning.GruntDamagePerSecond = 0.0f;
		Close.Tuning.GruntHealth = 100000.0f;
		Close.Tuning.MarksmanRelocateChance = Chance;
		Close.AddSkeleton(FVector2D(300.0, 0.0), ECrowdKind::Marksman);
		Close.AddEnemy(ECrowdKind::Grunt, FVector2D(420.0, 0.0));
		for (int32 Step = 0; Step < 20; ++Step)
		{
			Close.Step(0.05f, FVector2D(300.0, 0.0));
		}
		const FCrowdUnit* Marksman = FieldTest::FirstOf(Close, ECrowdKind::Marksman);
		const FCrowdUnit* Grunt = FieldTest::FirstOf(Close, ECrowdKind::Grunt);
		return Marksman && Grunt ? FVector2D::Distance(Marksman->Position, Grunt->Position) : -1.0;
	};
	TestTrue(TEXT("when it does relocate it puts ground between them"), Gap(1.0f) > 300.0);
	TestTrue(TEXT("when it does not, it stays where it was"), Gap(0.0f) < 200.0);
	const FCrowdTuning Shipped;
	TestTrue(TEXT("it relocates sometimes, not always"), Shipped.MarksmanRelocateChance > 0.0f && Shipped.MarksmanRelocateChance < 1.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOnlySkeletonsPay, "WebTesting.Shrine.OnlyPlainSkeletonsAreSacrificedOrCounted", WIZ_TEST_FLAGS)
bool FOnlySkeletonsPay::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	CrowdTest::FreezeArmy(Sim);
	for (int32 Index = 0; Index < 3; ++Index)
	{
		Sim.AddSkeleton(FVector2D(100.0 + Index * 40.0, 0.0));
		Sim.AddSkeleton(FVector2D(100.0 + Index * 40.0, 80.0), ECrowdKind::Guard);
		Sim.AddSkeleton(FVector2D(100.0 + Index * 40.0, 160.0), ECrowdKind::Marksman);
	}
	Sim.Step(0.05f, CrowdTest::Origin);
	TestEqual(TEXT("the army's worth in bones is its three plain skeletons"), Sim.GetArmyBoneValue(), 3 * Sim.GetUnitWorth(ECrowdKind::Skeleton));
	// A research too dear for three skeletons: the Guards and Marksmen do not make up the rest.
	const FCrowdShopPrice Price = Sim.GetShopPrice(ECrowdShopItem::Focus, 1);
	TestEqual(TEXT("no Guard is offered"), Price.Guards, 0);
	TestEqual(TEXT("all three skeletons are"), Price.Skeletons, 3);
	TestFalse(TEXT("and it cannot be paid"), Price.bCanPay);
	TestEqual(TEXT("so it cannot be bought"), Sim.GetShopMax(ECrowdShopItem::Focus), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLevelHasBattlefield, "WebTesting.Level.ABattlefieldNotWalls", WIZ_TEST_FLAGS)
bool FLevelHasBattlefield::RunTest(const FString& Parameters)
{
	const UWorld* Map = LoadObject<UWorld>(nullptr, TEXT("/Game/Maps/Outskirts.Outskirts"));
	if (!TestNotNull(TEXT("the Outskirts level"), Map) || !TestNotNull(TEXT("its level"), Map->PersistentLevel.Get()))
	{
		return false;
	}
	int32 Walls = 0;
	int32 Fields = 0;
	int32 Dressing = 0;
	for (const AActor* Actor : Map->PersistentLevel->Actors)
	{
		Walls += Cast<AWallActor>(Actor) ? 1 : 0;
		Fields += Cast<ABattlefieldActor>(Actor) ? 1 : 0;
	}
	FLevelLayout Layout = FLevelLayout::MakeOutskirts();
	TArray<FCrowdObstacle> Buildings;
	UCrowdSubsystem::ReadPlacedLevel(*Map->PersistentLevel, Layout, Buildings);
	TestEqual(TEXT("no walls stand around the level"), Walls, 0);
	TestEqual(TEXT("one battlefield marks its edge"), Fields, 1);
	TestTrue(TEXT("the game reads it"), Layout.bHasBattlefield && Layout.BattlefieldHalfSize.X > 5000.0 && Layout.BattlefieldHalfSize.Y > 3000.0);
	TestEqual(TEXT("and has no walls to stop him at it"), Layout.Walls.Num(), 0);
	TestTrue(TEXT("the wizard starts inside it"), FMath::Abs(Layout.Start.X - Layout.Battlefield.X) < Layout.BattlefieldHalfSize.X && FMath::Abs(Layout.Start.Y - Layout.Battlefield.Y) < Layout.BattlefieldHalfSize.Y);
	if (Layout.bHasGate)
	{
		TestTrue(TEXT("the kingdom gate is inside it"), FMath::Abs(Layout.Gate.X - Layout.Battlefield.X) < Layout.BattlefieldHalfSize.X && FMath::Abs(Layout.Gate.Y - Layout.Battlefield.Y) < Layout.BattlefieldHalfSize.Y);
	}
	// Past the old walls there is something to look at: buildings outside the battlefield, none of them to the east.
	int32 East = 0;
	for (const FCrowdObstacle& Building : Buildings)
	{
		const FVector2D Past = (Building.Center - Layout.Battlefield).GetAbs() - Layout.BattlefieldHalfSize;
		if (Past.X > 0.0 || Past.Y > 0.0)
		{
			++Dressing;
			East += Building.Center.X > Layout.Battlefield.X + Layout.BattlefieldHalfSize.X ? 1 : 0;
		}
	}
	TestTrue(TEXT("buildings stand past the edge"), Dressing >= 10);
	TestEqual(TEXT("none on the kingdom's side"), East, 0);
	TestNotNull(TEXT("the collection houses and trees read the cursor's circle from"), LoadObject<UMaterialParameterCollection>(nullptr, TEXT("/Game/Level/MPC_SeeThrough.MPC_SeeThrough")));
	TestNotNull(TEXT("the dagger"), LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Weapons/SM_Dagger.SM_Dagger")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
