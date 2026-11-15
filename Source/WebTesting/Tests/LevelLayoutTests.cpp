#include "Sim/LevelLayout.h"
#include "Tests/WizTestFlags.h"

#if WITH_DEV_AUTOMATION_TESTS

// The Outskirts are blocked out from the director's drawing of 2026-11-12. These keep the layout playable while it
// is edited: the wizard must be able to walk the whole way, and nothing important may stand inside a house.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLayoutOutskirtsHasTheDrawing, "WebTesting.Layout.OutskirtsHasWhatTheDrawingShows", WIZ_TEST_FLAGS)
bool FLayoutOutskirtsHasTheDrawing::RunTest(const FString& Parameters)
{
	const FLevelLayout Layout = FLevelLayout::MakeOutskirts();
	TestTrue(TEXT("a river"), Layout.CountPieces(ELevelPiece::River) >= 20);
	TestEqual(TEXT("two bridges"), Layout.CountPieces(ELevelPiece::Bridge), 2);
	TestTrue(TEXT("walkways"), Layout.CountPieces(ELevelPiece::Path) >= 20);
	TestEqual(TEXT("one farmland"), Layout.CountPieces(ELevelPiece::Farmland), 1);
	TestEqual(TEXT("five graveyards"), Layout.CountPieces(ELevelPiece::Graveyard), 5);
	TestTrue(TEXT("over a hundred houses"), Layout.CountPieces(ELevelPiece::House) >= 100);
	TestTrue(TEXT("medical buildings"), Layout.CountPieces(ELevelPiece::Medical) >= 5);
	TestEqual(TEXT("one boat dock"), Layout.CountPieces(ELevelPiece::Dock), 1);
	TestTrue(TEXT("trees"), Layout.CountPieces(ELevelPiece::Tree) >= 40);
	TestEqual(TEXT("twenty lamp posts"), Layout.CountPieces(ELevelPiece::Lamp), 20);
	TestTrue(TEXT("a commander"), Layout.bHasCommander);
	TestTrue(TEXT("posts along the way"), Layout.Posts.Num() >= 8);

	for (const FCrowdObstacle& Obstacle : Layout.MakeObstacles())
	{
		TestTrue(TEXT("an obstacle has a size"), Obstacle.HalfSize.X > 0.0 && Obstacle.HalfSize.Y > 0.0);
	}
	for (const FLevelPiece& Piece : Layout.Pieces)
	{
		TestTrue(TEXT("a piece that blocks is not turned, because the simulation's boxes cannot turn"), !FLevelLayout::Blocks(Piece.Kind) || Piece.Yaw == 0.0f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLayoutOutskirtsWalkable, "WebTesting.Layout.OutskirtsCanBeWalked", WIZ_TEST_FLAGS)
bool FLayoutOutskirtsWalkable::RunTest(const FString& Parameters)
{
	const FLevelLayout Layout = FLevelLayout::MakeOutskirts();
	const TArray<FCrowdObstacle> Obstacles = Layout.MakeObstacles();
	const auto IsClear = [&Obstacles](FVector2D Point, double Margin)
	{
		for (const FCrowdObstacle& Obstacle : Obstacles)
		{
			if (Obstacle.Contains(Point, Margin))
			{
				return false;
			}
		}
		return true;
	};

	TestTrue(TEXT("the start is in the open"), IsClear(Layout.Start, 150.0));
	TestTrue(TEXT("the shrine is in the open"), IsClear(Layout.Shrine, 150.0));
	TestTrue(TEXT("the commander is in the open"), IsClear(Layout.Commander, 100.0));
	TestTrue(TEXT("the wizard can walk from the start to the shrine"), Layout.CanWalk(Layout.Start, Layout.Shrine));
	TestTrue(TEXT("and on to the commander"), Layout.CanWalk(Layout.Shrine, Layout.Commander));
	for (int32 Index = 0; Index < Layout.Posts.Num(); ++Index)
	{
		const FLevelPost& Post = Layout.Posts[Index];
		TestTrue(FString::Printf(TEXT("post %d is in the open"), Index), IsClear(Post.Position, 60.0));
		TestTrue(FString::Printf(TEXT("post %d can be reached from the start"), Index), Layout.CanWalk(Layout.Start, Post.Position));
	}

	// The river cuts the level in two, and only the bridges join the halves.
	FLevelLayout NoBridges = Layout;
	for (FLevelPiece& Piece : NoBridges.Pieces)
	{
		if (Piece.Kind == ELevelPiece::Bridge)
		{
			// Fill the gap with water.
			Piece.Kind = ELevelPiece::River;
		}
	}
	const FVector2D WestBank = Layout.Posts.Last().Position;
	TestTrue(TEXT("the west bank can be reached"), Layout.CanWalk(Layout.Start, WestBank));
	TestFalse(TEXT("but not with the bridges under water: the river really blocks"), NoBridges.CanWalk(Layout.Start, WestBank));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLayoutByName, "WebTesting.Layout.MadeByName", WIZ_TEST_FLAGS)
bool FLayoutByName::RunTest(const FString& Parameters)
{
	FLevelLayout Layout;
	TestTrue(TEXT("Outskirts"), FLevelLayout::Make(TEXT("outskirts"), Layout));
	TestEqual(TEXT("its name"), Layout.Name, FString(TEXT("Outskirts")));
	TestFalse(TEXT("the outskirts have no ring waves"), Layout.bWaves);
	TestTrue(TEXT("Practice"), FLevelLayout::Make(TEXT("Practice"), Layout));
	TestEqual(TEXT("the practice layout keeps its ten walls"), Layout.Walls.Num(), 10);
	TestTrue(TEXT("and its waves"), Layout.bWaves);
	TestEqual(TEXT("and has no posts"), Layout.Posts.Num(), 0);
	TestFalse(TEXT("an unknown name is refused"), FLevelLayout::Make(TEXT("Moon"), Layout));
	for (const FString& Name : FLevelLayout::GetNames())
	{
		TestTrue(TEXT("every listed name makes a layout"), FLevelLayout::Make(Name, Layout));
	}
	return true;
}

// Direction of 2026-11-14 (01:05 PST): the level's pieces are actors that can be moved, turned and deleted.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLayoutTurnedPieceBlocks, "WebTesting.Layout.ATurnedPieceBlocksTheBoxAroundIt", WIZ_TEST_FLAGS)
bool FLayoutTurnedPieceBlocks::RunTest(const FString& Parameters)
{
	FLevelLayout Layout;
	FLevelPiece House;
	House.Kind = ELevelPiece::House;
	House.Center = FVector2D(1000.0, 0.0);
	House.HalfSize = FVector2D(300.0, 100.0);
	Layout.Pieces.Add(House);
	House.Center = FVector2D(3000.0, 0.0);
	House.Yaw = 90.0f;
	Layout.Pieces.Add(House);
	FLevelPiece Road;
	Road.Kind = ELevelPiece::Path;
	Road.Center = FVector2D(5000.0, 0.0);
	Layout.Pieces.Add(Road);

	const TArray<FCrowdObstacle> Obstacles = Layout.MakeObstacles();
	if (!TestEqual(TEXT("the two houses block, the road does not"), Obstacles.Num(), 2))
	{
		return false;
	}
	TestTrue(TEXT("a house that is not turned blocks its own box"), Obstacles[0].HalfSize.Equals(FVector2D(300.0, 100.0), 0.01));
	TestTrue(TEXT("a house turned a quarter blocks the long way across"), Obstacles[1].HalfSize.Equals(FVector2D(100.0, 300.0), 0.01));
	TestTrue(TEXT("its far end blocks"), Obstacles[1].Contains(FVector2D(3000.0, 280.0), 0.0));
	TestFalse(TEXT("where its long side would have been does not"), Obstacles[1].Contains(FVector2D(3280.0, 0.0), 0.0));
	return true;
}

#endif
