#include "Engine/Level.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Game/CrowdRenderer.h"
#include "Game/CrowdSubsystem.h"
#include "Game/LevelActors.h"
#include "Game/LevelPieceActor.h"
#include "PhysicsEngine/BodySetup.h"
#include "Sim/CrowdSim.h"
#include "Tests/CrowdTestHelpers.h"
#include "Tests/WizTestFlags.h"

#if WITH_DEV_AUTOMATION_TESTS

// Direction of 2026-11-14 (01:45 PST): everything is in the level as in a normal project. The shrine is an actor with
// its range seen in the editor; posts are actors that say what stands there and how many; house boxes are house
// meshes; lamps are the lamp model; nothing is laid out at run time that cannot be seen before play. A post has room
// for its group, and posts do not stand on one another.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPostSpreadsItsGroup, "WebTesting.Post.AGroupIsSpreadOverItsGround", WIZ_TEST_FLAGS)
bool FPostSpreadsItsGroup::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("a small group gets the least ground"), AEnemyPostActor::GetRoomFor(4), 300.0f);
	TestTrue(TEXT("a large group gets more"), AEnemyPostActor::GetRoomFor(16) > 400.0f);

	const FVector2D Post(1000.0, -500.0);
	const int32 Count = 14;
	const float Radius = AEnemyPostActor::GetRoomFor(Count);
	const TArray<FVector2D> Spots = UCrowdSubsystem::MakePostSpots(Post, Count, Radius);
	if (!TestEqual(TEXT("one spot for each"), Spots.Num(), Count))
	{
		return false;
	}
	double Nearest = TNumericLimits<double>::Max();
	for (int32 First = 0; First < Count; ++First)
	{
		TestTrue(TEXT("every spot is on the post's ground"), FVector2D::Distance(Spots[First], Post) <= Radius);
		for (int32 Second = First + 1; Second < Count; ++Second)
		{
			Nearest = FMath::Min(Nearest, FVector2D::Distance(Spots[First], Spots[Second]));
		}
	}
	// Two bodies side by side are 80 units from middle to middle.
	TestTrue(TEXT("no two start on top of one another: a body's width and more between them"), Nearest >= 85.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPostGroundIsWalked, "WebTesting.Post.ItsGroupWalksItsOwnGround", WIZ_TEST_FLAGS)
bool FPostGroundIsWalked::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	Sim.Tuning.PostIdleShare = 0.0f;
	Sim.Tuning.PostWanderRadius = 100.0f;
	Sim.Tuning.WakeRadius = 100.0f;
	const FVector2D Post(5000.0, 0.0);
	const float Radius = 600.0f;
	Sim.AddGarrisonEnemy(ECrowdKind::Grunt, Post + FVector2D(200.0, 0.0), 1, Radius, Post);
	double Farthest = 0.0;
	for (int32 Step = 0; Step < 2400; ++Step)
	{
		Sim.Step(0.05f, CrowdTest::Origin);
		Farthest = FMath::Max(Farthest, FVector2D::Distance(Sim.GetUnits()[0].Position, Post));
	}
	TestTrue(TEXT("it stays on the post's ground"), Farthest <= Radius + 10.0);
	TestTrue(TEXT("and uses more of it than the old few steps around its spot"), Farthest > 320.0);
	TestEqual(TEXT("it is still at its post"), Sim.CountAsleep(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOutskirtsIsPlaced, "WebTesting.Level.TheOutskirtsIsPlacedInTheLevel", WIZ_TEST_FLAGS)
bool FOutskirtsIsPlaced::RunTest(const FString& Parameters)
{
	const UWorld* Map = LoadObject<UWorld>(nullptr, TEXT("/Game/Maps/Outskirts.Outskirts"));
	if (!TestNotNull(TEXT("the Outskirts level"), Map) || !TestNotNull(TEXT("its level"), Map->PersistentLevel.Get()))
	{
		return false;
	}
	const ULevel& Level = *Map->PersistentLevel;

	int32 Shrines = 0;
	int32 Lamps = 0;
	int32 Houses = 0;
	int32 UnsolidHouses = 0;
	int32 Gates = 0;
	const AShrineActor* Shrine = nullptr;
	TArray<const AEnemyPostActor*> Posts;
	TArray<FVector2D> LampSpots;
	for (const AActor* Actor : Level.Actors)
	{
		if (const AShrineActor* Found = Cast<AShrineActor>(Actor))
		{
			++Shrines;
			Shrine = Found;
		}
		else if (const AEnemyPostActor* Post = Cast<AEnemyPostActor>(Actor))
		{
			Posts.Add(Post);
		}
		else if (Actor && Actor->IsA<AKingdomGateActor>())
		{
			++Gates;
		}
		else if (const AStaticMeshActor* MeshActor = Cast<AStaticMeshActor>(Actor))
		{
			const UStaticMesh* Mesh = MeshActor->GetStaticMeshComponent() ? MeshActor->GetStaticMeshComponent()->GetStaticMesh() : nullptr;
			const FString Path = Mesh ? Mesh->GetPathName() : FString();
			if (Path.Contains(TEXT("SM_Lamp_Post")))
			{
				++Lamps;
				const FVector Where = LevelRead::GetPlacedTransform(*MeshActor).GetLocation();
				LampSpots.Add(FVector2D(Where.X, Where.Y));
			}
			else if (Path.StartsWith(TEXT("/Game/Buildings/")))
			{
				++Houses;
				UnsolidHouses += Mesh->GetBodySetup() && Mesh->GetBodySetup()->AggGeom.GetElementCount() > 0 ? 0 : 1;
			}
		}
	}

	FLevelLayout Layout = FLevelLayout::MakeOutskirts();
	const int32 CodePosts = Layout.Posts.Num();
	const int32 CodeRoads = Layout.Routes.Num();
	TArray<FCrowdObstacle> Buildings;
	UCrowdSubsystem::ReadPlacedLevel(Level, Layout, Buildings);

	// The shrine.
	TestEqual(TEXT("one shrine stands in the level"), Shrines, 1);
	if (Shrine)
	{
		TestNotNull(TEXT("it is the shrine model"), Shrine->GetMesh()->GetStaticMesh().Get());
		const FCrowdObstacle Body = UCrowdSubsystem::MakeBuildingObstacle(Shrine->GetBlockingBounds());
		TestTrue(TEXT("the wizard can stand in its range without standing in it"), Shrine->Range > Body.HalfSize.GetMax() + 150.0);
		const FBox Bounds = Shrine->GetBlockingBounds();
		const FBox2D Foot(FVector2D(Bounds.Min.X, Bounds.Min.Y), FVector2D(Bounds.Max.X, Bounds.Max.Y));
		for (const FVector2D& Lamp : LampSpots)
		{
			TestFalse(TEXT("the shrine does not stand on a lamp post"), Foot.ExpandBy(40.0).IsInside(Lamp));
		}
		for (const FLevelPiece& Piece : Layout.Pieces)
		{
			if (Piece.Kind == ELevelPiece::Path || Piece.Kind == ELevelPiece::Bridge)
			{
				const FBox2D Road(Piece.Center - Piece.HalfSize, Piece.Center + Piece.HalfSize);
				TestFalse(TEXT("the shrine does not stand on a road"), Piece.Yaw == 0.0f && Road.Intersect(Foot));
			}
		}
	}

	// Lamps and houses are their models.
	TestEqual(TEXT("no lamp is a plain post any more"), Layout.CountPieces(ELevelPiece::Lamp), 0);
	TestTrue(TEXT("the lamps are the lamp model"), Lamps >= 15);
	TestEqual(TEXT("no house is a plain box any more"), Layout.CountPieces(ELevelPiece::House), 0);
	TestTrue(TEXT("the houses are house meshes"), Houses >= 60);
	TestEqual(TEXT("every house mesh is solid by itself"), UnsolidHouses, 0);

	// Posts.
	// Changed 2026-11-14 17:40 PST: the director added a post in the level (15; the code-made layout has 14), so the
	// level may hold more posts than the layout, never fewer.
	TestTrue(TEXT("every post of the layout is in the level"), Layout.Posts.Num() >= CodePosts);
	TestTrue(TEXT("the commander has his post"), Layout.bHasCommander);
	TestEqual(TEXT("every patrol road is in the level"), Layout.Routes.Num(), CodeRoads);
	TArray<FCrowdObstacle> Blocked = Layout.MakeObstacles();
	Blocked.Append(Buildings);
	for (int32 First = 0; First < Posts.Num(); ++First)
	{
		const AEnemyPostActor& Post = *Posts[First];
		const FVector Where3 = LevelRead::GetPlacedTransform(Post).GetLocation();
		const FVector2D Where(Where3.X, Where3.Y);
		TestTrue(*FString::Printf(TEXT("%s has room for its %d"), *Post.GetName(), Post.GetCount()), Post.Radius >= AEnemyPostActor::GetRoomFor(Post.GetCount()) - 1.0f);
		for (const FCrowdObstacle& Obstacle : Blocked)
		{
			TestFalse(*FString::Printf(TEXT("%s does not stand in a building, a tree or water"), *Post.GetName()), Obstacle.Contains(Where, 60.0));
		}
		for (int32 Second = First + 1; Second < Posts.Num(); ++Second)
		{
			const FVector Other = LevelRead::GetPlacedTransform(*Posts[Second]).GetLocation();
			TestTrue(*FString::Printf(TEXT("%s and %s have ground between them"), *Post.GetName(), *Posts[Second]->GetName()),
				FVector2D::Distance(Where, FVector2D(Other.X, Other.Y)) >= Post.Radius + Posts[Second]->Radius + 100.0);
		}
	}

	// The wizard can still walk the whole way with the buildings where they now stand.
	FLevelLayout Walk = Layout;
	Walk.Walls.Append(Buildings);
	for (const FLevelPost& Post : Layout.Posts)
	{
		TestTrue(TEXT("the wizard can walk from the start to every post"), Walk.CanWalk(Layout.Start, Post.Position));
	}
	TestTrue(TEXT("and to the commander"), Walk.CanWalk(Layout.Start, Layout.Commander));

	// The kingdom gate (direction of 2026-11-14 10:42 PST): one in the level, in the open, past the commander's ground.
	TestEqual(TEXT("one kingdom gate stands in the level"), Gates, 1);
	TestTrue(TEXT("the game reads it"), Layout.bHasGate);
	for (const FCrowdObstacle& Obstacle : Blocked)
	{
		TestFalse(TEXT("the gate's middle is not in a building, a tree or water"), Obstacle.Contains(Layout.Gate, 60.0));
	}
	TestTrue(TEXT("the wizard can walk from the commander to the gate"), Walk.CanWalk(Layout.Commander, Layout.Gate));
	TestTrue(TEXT("the gate is clear of the commander's ground"),
		FVector2D::Distance(Layout.Commander, Layout.Gate) > Layout.CommanderRadius + Layout.GateHalfSize.GetMax());
	for (const TArray<FVector2D>& Road : Layout.Routes)
	{
		for (int32 Index = 1; Index < Road.Num(); ++Index)
		{
			TestTrue(TEXT("a patrol can walk every stretch of its road"), Walk.CanWalk(Road[Index - 1], Road[Index]));
		}
	}
	return true;
}

#endif
