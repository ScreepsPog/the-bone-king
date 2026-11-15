#include "Game/CrowdSubsystem.h"
#include "Components/AudioComponent.h"
#include "PhysicsEngine/BodySetup.h"

#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Game/CrowdRenderer.h"
#include "Engine/Level.h"
#include "Game/LevelActors.h"
#include "Game/LevelPieceActor.h"
#include "GameFramework/PlayerStart.h"
#include "Game/DeathPick.h"
#include "Game/WizardAudioSubsystem.h"
#include "Game/WizardPawn.h"
#include "Engine/GameInstance.h"
#include "Misc/ConfigCacheIni.h"
#include "Kismet/GameplayStatics.h"
#include "WebTesting.h"

int32 UCrowdSubsystem::WaveSize(float RunSeconds, int32 Roll)
{
	return 4 + FMath::Clamp(Roll, 0, 4) + FMath::FloorToInt32(FMath::Max(RunSeconds, 0.0f) / 30.0f);
}

int32 UCrowdSubsystem::WaveBrutes(float RunSeconds)
{
	if (RunSeconds < FirstBruteSeconds)
	{
		return 0;
	}
	return FMath::Min(1 + FMath::FloorToInt32((RunSeconds - FirstBruteSeconds) / 60.0f), 4);
}

bool UCrowdSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId UCrowdSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCrowdSubsystem, STATGROUP_Tickables);
}

namespace
{
	// The level the run starts in. Kept across restarts, which load the map again and make a new manager.
	FString StartMapName = TEXT("Outskirts");
	double StartMapScale = FLevelLayout::OutskirtsUnitsPerPixel;
	// A post's group is spread over this share of its ground, so those at the rim have room to walk outward as well.
	const double PostSpotShare = 0.8;
}

float UCrowdSubsystem::GetMapScale() const
{
	return static_cast<float>(StartMapScale);
}

void UCrowdSubsystem::SetMapScale(float UnitsPerPixel)
{
	StartMapScale = FMath::Clamp(UnitsPerPixel, MapScaleMin, MapScaleMax);
	LoadMap(Layout.Name);
}

int32 UCrowdSubsystem::GetRoadPatrolCount() const
{
	int32 Count = 0;
	for (const FRoadPatrol& Patrol : RoadPatrols)
	{
		Count += Sim.CountSquad(Patrol.Squad) > 0 ? 1 : 0;
	}
	return Count;
}

int32 UCrowdSubsystem::SpawnRoadPatrol(int32 RouteIndex)
{
	const int32 Size = Layout.RouteSizes.IsValidIndex(RouteIndex) && Layout.RouteSizes[RouteIndex] >= 0
		? Layout.RouteSizes[RouteIndex] : FMath::RoundToInt32(Sim.Tuning.RoadPatrolSize);
	if (!Layout.Routes.IsValidIndex(RouteIndex) || !RoadPatrols.IsValidIndex(RouteIndex) || Layout.Routes[RouteIndex].Num() < 2 || Size <= 0)
	{
		return 0;
	}
	const TArray<FVector2D>& Route = Layout.Routes[RouteIndex];
	const FVector2D Center = Wizard.IsValid() ? Wizard->GetPosition2D() : Layout.Start;
	// It starts at the end of the road farther from the wizard and walks toward the other end. Never where he would see it appear.
	const bool bFromLast = FVector2D::DistSquared(Route.Last(), Center) >= FVector2D::DistSquared(Route[0], Center);
	const FVector2D From = bFromLast ? Route.Last() : Route[0];
	if (FVector2D::DistSquared(From, Center) < FMath::Square(static_cast<double>(RoadPatrolSpawnClear)))
	{
		return 0;
	}
	const int32 Squad = NextSquad++;
	int32 Placed = 0;
	for (int32 Index = 0; Index < Size; ++Index)
	{
		const double Angle = FMath::FRandRange(0.0, UE_DOUBLE_TWO_PI);
		const FVector2D Spot = From + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * FMath::FRandRange(0.0, static_cast<double>(RoadPatrolScatter));
		Placed += Sim.AddPatrolEnemy(ECrowdKind::Grunt, Spot, Squad, RouteIndex, bFromLast ? Route.Num() - 2 : 1, bFromLast ? -1 : 1) ? 1 : 0;
	}
	RoadPatrols[RouteIndex].Squad = Squad;
	RoadPatrols[RouteIndex].Timer = Sim.Tuning.RoadPatrolRespawn;
	return Placed;
}

void UCrowdSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	Sim.Random.GenerateNewSeed();
	LoadGroundPaint();
	// A level made for the game has its layout actor placed in it; any other level gets one.
	Renderer = nullptr;
	for (TActorIterator<ACrowdRenderer> It(&InWorld); It; ++It)
	{
		Renderer = *It;
		break;
	}
	if (!Renderer)
	{
		Renderer = InWorld.SpawnActor<ACrowdRenderer>();
	}
	if (!LoadMap(StartMapName))
	{
		LoadMap(TEXT("Outskirts"));
	}
}

bool UCrowdSubsystem::LoadMap(const FString& Name)
{
	FLevelLayout NewLayout;
	if (!FLevelLayout::Make(Name, NewLayout, StartMapScale))
	{
		return false;
	}
	Layout = MoveTemp(NewLayout);
	StartMapName = Layout.Name;

	// A level whose pieces were placed as actors: they are the pieces, wherever they were moved to, and those deleted are gone.
	PlacedPieceCount = 0;
	PlacedPostCount = 0;
	bPiecesPlaced = false;
	PlacedBuildings.Reset();
	UWorld* PlacedWorld = GetWorld();
	if (PlacedWorld && PlacedWorld->PersistentLevel && Renderer && Renderer->UsesPlacedPieces() && Layout.Name == TEXT("Outskirts"))
	{
		// Everything stands in the level as it was placed there: nothing is laid out here.
		bPiecesPlaced = true;
		ReadPlacedLevel(*PlacedWorld->PersistentLevel, Layout, PlacedBuildings);
		PlacedPieceCount = Layout.Pieces.Num();
		PlacedPostCount = Layout.Posts.Num() + (Layout.bHasCommander ? 1 : 0);
	}

	// Buildings placed in the level by hand block like walls. With the house boxes switched off, the boxes are gone.
	// They belong to the Outskirts: the practice layout, loaded in the same level, is the bare test bed it always was.
	if (UWorld* World = GetWorld(); World && !bPiecesPlaced && Layout.Name == TEXT("Outskirts"))
	{
		for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
		{
			const UStaticMeshComponent* Component = It->GetStaticMeshComponent();
			const UStaticMesh* Mesh = Component ? Component->GetStaticMesh() : nullptr;
			if (It->ActorHasTag(TEXT("Blocker")) || (Mesh && Mesh->GetPathName().StartsWith(TEXT("/Game/Buildings/"))))
			{
				PlacedBuildings.Add(MakeBuildingObstacle(It->GetComponentsBoundingBox()));
				PlacedBuildings.Last().bBoltPasses = Mesh && BoltPassesMesh(Mesh->GetPathName());
			}
		}
	}
	if (Renderer && !Renderer->AreHouseBoxesShown())
	{
		Layout.Pieces.RemoveAll([](const FLevelPiece& Piece) { return Piece.Kind == ELevelPiece::House; });
	}
	if (Renderer)
	{
		// Placed pieces and buildings are solid themselves; only buildings dropped into a code-made layout need a wall made for them.
		Renderer->SetBuildingBlockers(bPiecesPlaced ? TArray<FCrowdObstacle>() : PlacedBuildings);
		Renderer->SetShrineShown(!bPiecesPlaced);
	}

	// The run starts over.
	Sim.Reset();
	Meter.Reset();
	Blink.Reset();
	BlinksCast = 0;
	Command.Exit();
	ArrowHits = 0;
	JavelinHits = 0;
	SlashHits = 0;
	SlowTimer = 0.0f;
	bCommanderNear = false;
	SlamsLanded = 0;
	SlamHits = 0;
	AssassinsSent = 0;
	AssassinTimer = Sim.Tuning.AssassinInterval;
	Shrine.Close();
	Shrine.Position = Layout.Shrine;
	Shrine.Range = Layout.ShrineRange > 0.0f ? Layout.ShrineRange : FShrineState().Range;
	RunSeconds = 0.0f;
	WaveTimer = FirstWaveDelay;
	TotalRaised = 0;
	TotalKilled = 0;
	BoltsFired = 0;
	SpearsThrown = 0;
	SpearSounds.Reset();
	PatrolsSent = 0;
	NextSquad = 1;
	bCommanderPlaced = false;
	bAreaTaken = false;
	AreaTakenSeconds = 0.0f;
	// The gate is shut again. Its marking in the level is hidden until it opens.
	Gate = FKingdomGate();
	Battlefield = FBattlefield();
	bLeftBattlefield = false;
	if (Layout.bHasBattlefield)
	{
		Battlefield.Place(Layout.Battlefield, Layout.BattlefieldHalfSize);
	}
	if (Layout.bHasGate)
	{
		Gate.Place(Layout.Gate, Layout.GateHalfSize, Layout.GateYaw);
	}
	GateActor = nullptr;
	if (PlacedWorld)
	{
		for (TActorIterator<AKingdomGateActor> It(PlacedWorld); It; ++It)
		{
			GateActor = *It;
			It->SetOpenShown(false);
			break;
		}
	}
	bWavesEnabled = Layout.bWaves;
	bObstaclesEnabled = true;
	ApplyObstacles();
	Sim.ClearRoutes();
	for (const TArray<FVector2D>& Route : Layout.Routes)
	{
		Sim.AddRoute(Route);
	}
	RoadPatrols.Reset();
	RoadPatrols.SetNum(Layout.Routes.Num());
	if (Renderer)
	{
		Renderer->SetShrine(Shrine.Position);
		// Placed pieces are in the level, as they are in the editor: the layout actor draws none.
		Renderer->SetPieces(bPiecesPlaced ? TArray<FLevelPiece>() : Layout.Pieces);
	}
	PlaceWizardAtStart();

	int32 Posted = 0;
	for (const FLevelPost& Post : Layout.Posts)
	{
		Posted += SpawnGarrison(Post.Position, Post.Grunts, Post.Brutes, Post.Archers, Post.Radius);
	}
	if (Layout.bHasCommander)
	{
		SpawnCommander(Layout.Commander, Layout.EscortGrunts, Layout.EscortBrutes, Layout.EscortArchers, Layout.CommanderRadius);
	}
	int32 OnRoads = 0;
	for (int32 RouteIndex = 0; RouteIndex < Layout.Routes.Num(); ++RouteIndex)
	{
		OnRoads += SpawnRoadPatrol(RouteIndex);
	}
	UE_LOG(LogWizard, Log, TEXT("Map loaded: %s at %.1f units a pixel, %d pieces, %d walls, %d posts, %d enemies at posts, %d on %d roads"),
		*Layout.Name, StartMapScale, Layout.Pieces.Num(), Layout.Walls.Num(), Layout.Posts.Num(), Posted, OnRoads, Layout.Routes.Num());
	return true;
}

void UCrowdSubsystem::ReadPlacedLevel(const ULevel& Level, FLevelLayout& InOutLayout, TArray<FCrowdObstacle>& OutBuildings)
{
	FLevelLayout& Layout = InOutLayout;
	Layout.Pieces.Reset();
	TArray<const AEnemyPostActor*> Posts;
	TArray<const APatrolRouteActor*> Roads;
	TArray<FCrowdObstacle> PlacedWalls;
	for (const AActor* Actor : Level.Actors)
	{
		if (!Actor)
		{
			continue;
		}
		if (const ALevelPieceActor* Piece = Cast<ALevelPieceActor>(Actor))
		{
			Layout.Pieces.Add(Piece->MakePiece());
		}
		else if (const AEnemyPostActor* Post = Cast<AEnemyPostActor>(Actor))
		{
			Posts.Add(Post);
		}
		else if (const AWallActor* PlacedWall = Cast<AWallActor>(Actor))
		{
			FCrowdObstacle Wall;
			PlacedWall->GetBox(Wall.Center, Wall.HalfSize);
			PlacedWalls.Add(Wall);
		}
		else if (const APatrolRouteActor* Road = Cast<APatrolRouteActor>(Actor))
		{
			Roads.Add(Road);
		}
		else if (const AShrineActor* Placed = Cast<AShrineActor>(Actor))
		{
			const FVector Where = LevelRead::GetPlacedTransform(*Placed).GetLocation();
			Layout.Shrine = FVector2D(Where.X, Where.Y);
			Layout.ShrineRange = Placed->Range;
			OutBuildings.Add(MakeBuildingObstacle(Placed->GetBlockingBounds()));
		}
		else if (const ABattlefieldActor* PlacedField = Cast<ABattlefieldActor>(Actor))
		{
			Layout.bHasBattlefield = true;
			PlacedField->GetBox(Layout.Battlefield, Layout.BattlefieldHalfSize);
		}
		else if (const AKingdomGateActor* PlacedGate = Cast<AKingdomGateActor>(Actor))
		{
			const FTransform Where = LevelRead::GetPlacedTransform(*PlacedGate);
			Layout.bHasGate = true;
			Layout.Gate = FVector2D(Where.GetLocation().X, Where.GetLocation().Y);
			Layout.GateHalfSize = PlacedGate->GetHalfSize();
			Layout.GateYaw = static_cast<float>(Where.Rotator().Yaw);
		}
		else if (Actor->IsA<APlayerStart>())
		{
			const FVector Where = LevelRead::GetPlacedTransform(*Actor).GetLocation();
			Layout.Start = FVector2D(Where.X, Where.Y);
		}
		else if (const AStaticMeshActor* MeshActor = Cast<AStaticMeshActor>(Actor))
		{
			const UStaticMeshComponent* Component = MeshActor->GetStaticMeshComponent();
			const UStaticMesh* Mesh = Component ? Component->GetStaticMesh() : nullptr;
			if (Mesh && (MeshActor->ActorHasTag(TEXT("Blocker")) || Mesh->GetPathName().StartsWith(TEXT("/Game/Buildings/"))))
			{
				// What it blocks is the collision made for it in the mesh editor.
				const int32 First = OutBuildings.Num();
				MakeCollisionObstacles(*Mesh, LevelRead::GetPlacedTransform(*MeshActor), false, OutBuildings);
				if (BoltPassesMesh(Mesh->GetPathName()))
				{
					for (int32 Index = First; Index < OutBuildings.Num(); ++Index)
					{
						OutBuildings[Index].bBoltPasses = true;
					}
				}
			}
			else if (Mesh && (MeshActor->ActorHasTag(TEXT("Walkable")) || Mesh->GetPathName().StartsWith(TEXT("/Game/Props/SM_Bridge"))))
			{
				// A bridge: its deck is walked on, its rails are in the way.
				MakeCollisionObstacles(*Mesh, LevelRead::GetPlacedTransform(*MeshActor), true, OutBuildings);
			}
		}
	}

	// A level with walls of its own has only those; one without keeps the layout's. A level with a battlefield has
	// none: its edge is a warning, not a wall.
	if (Layout.bHasBattlefield)
	{
		Layout.Walls.Reset();
	}
	else if (PlacedWalls.Num() > 0)
	{
		Layout.Walls = MoveTemp(PlacedWalls);
	}

	// A level with posts of its own has only those; one without keeps the layout's.
	if (Posts.Num() > 0)
	{
		Layout.Posts.Reset();
		Layout.bHasCommander = false;
		Posts.Sort([](const AEnemyPostActor& First, const AEnemyPostActor& Second) { return First.GetName() < Second.GetName(); });
		for (const AEnemyPostActor* Post : Posts)
		{
			const FVector Where = LevelRead::GetPlacedTransform(*Post).GetLocation();
			if (Post->bCommander && !Layout.bHasCommander)
			{
				Layout.bHasCommander = true;
				Layout.Commander = FVector2D(Where.X, Where.Y);
				Layout.EscortGrunts = Post->Grunts;
				Layout.EscortArchers = Post->Archers;
				Layout.EscortBrutes = Post->Brutes;
				Layout.CommanderRadius = Post->Radius;
				continue;
			}
			FLevelPost NewPost;
			NewPost.Position = FVector2D(Where.X, Where.Y);
			NewPost.Grunts = Post->Grunts;
			NewPost.Archers = Post->Archers;
			NewPost.Brutes = Post->Brutes;
			NewPost.Radius = Post->Radius;
			Layout.Posts.Add(NewPost);
		}
	}
	if (Roads.Num() > 0)
	{
		Layout.Routes.Reset();
		Layout.RouteSizes.Reset();
		Roads.Sort([](const APatrolRouteActor& First, const APatrolRouteActor& Second) { return First.GetName() < Second.GetName(); });
		for (const APatrolRouteActor* Road : Roads)
		{
			TArray<FVector2D> Points = Road->GetPoints();
			if (Points.Num() >= 2)
			{
				Layout.Routes.Add(MoveTemp(Points));
				Layout.RouteSizes.Add(Road->PatrolSize);
			}
		}
	}
}

TArray<FVector2D> UCrowdSubsystem::MakePostSpots(FVector2D Post, int32 Count, double Radius)
{
	// A sunflower: each spot a golden angle on from the last and a little farther out, which fills a circle evenly.
	TArray<FVector2D> Spots;
	const double GoldenAngle = 2.39996322972865332;
	const double Reach = FMath::Max(Radius, 0.0) * PostSpotShare;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const double Distance = Count > 1 ? Reach * FMath::Sqrt(static_cast<double>(Index) / (Count - 1)) : 0.0;
		const double Angle = Index * GoldenAngle;
		Spots.Add(Post + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Distance);
	}
	return Spots;
}

bool UCrowdSubsystem::BoltPassesMesh(const FString& MeshPath)
{
	return MeshPath.StartsWith(TEXT("/Game/Buildings/")) || MeshPath.Contains(TEXT("Lamp_Post"));
}

FCrowdObstacle UCrowdSubsystem::MakeBuildingObstacle(const FBox& Bounds)
{
	FCrowdObstacle Obstacle;
	Obstacle.Center = FVector2D(Bounds.GetCenter().X, Bounds.GetCenter().Y);
	Obstacle.HalfSize = FVector2D(Bounds.GetExtent().X, Bounds.GetExtent().Y) * BuildingBodyShare;
	return Obstacle;
}

void UCrowdSubsystem::MakeBuildingObstacles(const FBox& MeshBox, const FTransform& Placed, TArray<FCrowdObstacle>& OutObstacles)
{
	// The walls on the ground, in the mesh's own space: its box drawn in so eaves do not block.
	const FVector Center = MeshBox.GetCenter();
	const FVector2D Half(MeshBox.GetExtent().X * BuildingBodyShare, MeshBox.GetExtent().Y * BuildingBodyShare);
	const double Yaw = FRotator::NormalizeAxis(Placed.Rotator().Yaw);
	const double FromSquare = FMath::Abs(FMath::Fmod(FMath::Abs(Yaw) + 45.0, 90.0) - 45.0);
	const auto BoxAround = [&Placed, &Center](FVector2D Low, FVector2D High)
	{
		FBox2D Around(ForceInit);
		for (const FVector2D& Corner : { FVector2D(Low.X, Low.Y), FVector2D(High.X, Low.Y), FVector2D(High.X, High.Y), FVector2D(Low.X, High.Y) })
		{
			const FVector World = Placed.TransformPosition(FVector(Center.X + Corner.X, Center.Y + Corner.Y, Center.Z));
			Around += FVector2D(World.X, World.Y);
		}
		FCrowdObstacle Obstacle;
		Obstacle.Center = Around.GetCenter();
		Obstacle.HalfSize = Around.GetExtent();
		return Obstacle;
	};
	if (FromSquare < 3.0)
	{
		OutObstacles.Add(BoxAround(-Half, Half));
		return;
	}
	const FVector Scale = Placed.GetScale3D().GetAbs();
	const int32 CountX = FMath::Max(FMath::CeilToInt32(Half.X * 2.0 * Scale.X / BuildingCellSize), 1);
	const int32 CountY = FMath::Max(FMath::CeilToInt32(Half.Y * 2.0 * Scale.Y / BuildingCellSize), 1);
	for (int32 X = 0; X < CountX; ++X)
	{
		for (int32 Y = 0; Y < CountY; ++Y)
		{
			const FVector2D Low(-Half.X + Half.X * 2.0 * X / CountX, -Half.Y + Half.Y * 2.0 * Y / CountY);
			const FVector2D High(-Half.X + Half.X * 2.0 * (X + 1) / CountX, -Half.Y + Half.Y * 2.0 * (Y + 1) / CountY);
			OutObstacles.Add(BoxAround(Low, High));
		}
	}
}

double UCrowdSubsystem::MakeFootprintObstacles(const TArray<FVector2D>& Points, double SkipWiderThan, TArray<FCrowdObstacle>& OutObstacles)
{
	if (Points.Num() < 3)
	{
		return 0.0;
	}
	// The outline around the points: lowest left first, then round (a monotone chain).
	TArray<FVector2D> Sorted = Points;
	Sorted.Sort([](const FVector2D& First, const FVector2D& Second) { return First.X < Second.X || (First.X == Second.X && First.Y < Second.Y); });
	const auto Cross = [](const FVector2D& Origin, const FVector2D& First, const FVector2D& Second)
	{
		return (First.X - Origin.X) * (Second.Y - Origin.Y) - (First.Y - Origin.Y) * (Second.X - Origin.X);
	};
	TArray<FVector2D> Hull;
	for (int32 Pass = 0; Pass < 2; ++Pass)
	{
		const int32 Start = Hull.Num();
		for (int32 Index = 0; Index < Sorted.Num(); ++Index)
		{
			const FVector2D& Point = Sorted[Pass == 0 ? Index : Sorted.Num() - 1 - Index];
			while (Hull.Num() >= Start + 2 && Cross(Hull[Hull.Num() - 2], Hull.Last(), Point) <= 0.0)
			{
				Hull.Pop(EAllowShrinking::No);
			}
			Hull.Add(Point);
		}
		Hull.Pop(EAllowShrinking::No);
	}
	if (Hull.Num() < 3)
	{
		return 0.0;
	}
	FBox2D Around(ForceInit);
	double Area = 0.0;
	double Longest = 0.0;
	for (int32 Index = 0; Index < Hull.Num(); ++Index)
	{
		Around += Hull[Index];
		const FVector2D& Next = Hull[(Index + 1) % Hull.Num()];
		Area += Hull[Index].X * Next.Y - Next.X * Hull[Index].Y;
		for (int32 Other = Index + 1; Other < Hull.Num(); ++Other)
		{
			Longest = FMath::Max(Longest, FVector2D::Distance(Hull[Index], Hull[Other]));
		}
	}
	Area = FMath::Abs(Area) * 0.5;
	// How wide it is across its narrow way: its area over its length.
	const double Width = Longest > 0.0 ? Area / Longest : 0.0;
	if (SkipWiderThan > 0.0 && Width > SkipWiderThan)
	{
		return Width;
	}
	const FVector2D Size = Around.GetSize();
	if (Area >= Size.X * Size.Y * 0.9)
	{
		// Square to the level, near enough: one box.
		FCrowdObstacle Whole;
		Whole.Center = Around.GetCenter();
		Whole.HalfSize = Around.GetExtent();
		OutObstacles.Add(Whole);
		return Width;
	}
	// A grid of small boxes: those whose middle is inside the outline, or just outside it.
	const double Cell = FMath::Clamp(Width, 50.0, BuildingCellSize);
	const int32 CountX = FMath::Clamp(FMath::CeilToInt32(Size.X / Cell), 1, 64);
	const int32 CountY = FMath::Clamp(FMath::CeilToInt32(Size.Y / Cell), 1, 64);
	const FVector2D Step(Size.X / CountX, Size.Y / CountY);
	const double Slack = FMath::Min(Step.X, Step.Y) * 0.3;
	for (int32 X = 0; X < CountX; ++X)
	{
		for (int32 Y = 0; Y < CountY; ++Y)
		{
			const FVector2D Middle(Around.Min.X + (X + 0.5) * Step.X, Around.Min.Y + (Y + 0.5) * Step.Y);
			bool bInside = true;
			for (int32 Index = 0; Index < Hull.Num() && bInside; ++Index)
			{
				const FVector2D& From = Hull[Index];
				const FVector2D Edge = Hull[(Index + 1) % Hull.Num()] - From;
				const double EdgeLength = Edge.Size();
				// The outline runs counter-clockwise: inside is to the left of every edge.
				bInside = EdgeLength <= 0.0 || (Edge.X * (Middle.Y - From.Y) - Edge.Y * (Middle.X - From.X)) / EdgeLength >= -Slack;
			}
			if (bInside)
			{
				FCrowdObstacle Box;
				Box.Center = Middle;
				Box.HalfSize = Step * 0.5;
				OutObstacles.Add(Box);
			}
		}
	}
	return Width;
}

namespace
{
	/** The outline around points on the ground, counter-clockwise. False with fewer than three corners. */
	bool MakeGroundOutline(const TArray<FVector2D>& Points, TArray<FVector2D>& OutHull)
	{
		OutHull.Reset();
		TArray<FVector2D> Sorted = Points;
		Sorted.Sort([](const FVector2D& First, const FVector2D& Second) { return First.X < Second.X || (First.X == Second.X && First.Y < Second.Y); });
		for (int32 Pass = 0; Pass < 2; ++Pass)
		{
			const int32 Start = OutHull.Num();
			for (int32 Index = 0; Index < Sorted.Num(); ++Index)
			{
				const FVector2D& Point = Sorted[Pass == 0 ? Index : Sorted.Num() - 1 - Index];
				while (OutHull.Num() >= Start + 2)
				{
					const FVector2D& Origin = OutHull[OutHull.Num() - 2];
					const FVector2D& Last = OutHull.Last();
					if ((Last.X - Origin.X) * (Point.Y - Origin.Y) - (Last.Y - Origin.Y) * (Point.X - Origin.X) > 0.0)
					{
						break;
					}
					OutHull.Pop(EAllowShrinking::No);
				}
				OutHull.Add(Point);
			}
			OutHull.Pop(EAllowShrinking::No);
		}
		return OutHull.Num() >= 3;
	}

	/** True when a point is inside a counter-clockwise outline, or within Slack outside it. */
	bool IsInGroundOutline(const TArray<FVector2D>& Hull, FVector2D Point, double Slack)
	{
		for (int32 Index = 0; Index < Hull.Num(); ++Index)
		{
			const FVector2D& From = Hull[Index];
			const FVector2D Edge = Hull[(Index + 1) % Hull.Num()] - From;
			const double Length = Edge.Size();
			if (Length > 0.0 && (Edge.X * (Point.Y - From.Y) - Edge.Y * (Point.X - From.X)) / Length < -Slack)
			{
				return false;
			}
		}
		return true;
	}
}

void UCrowdSubsystem::MakeCollisionObstacles(const UStaticMesh& Mesh, const FTransform& Placed, bool bWalkOnWide, TArray<FCrowdObstacle>& OutObstacles)
{
	// Every collision shape made for the mesh, as the corners it has on the ground where the mesh stands.
	TArray<TArray<FVector2D>> Shapes;
	if (const UBodySetup* Body = Mesh.GetBodySetup())
	{
		const auto Add = [&](const FTransform& ShapeTransform, TConstArrayView<FVector> Corners)
		{
			TArray<FVector2D>& Points = Shapes.AddDefaulted_GetRef();
			for (const FVector& Corner : Corners)
			{
				const FVector World = Placed.TransformPosition(ShapeTransform.TransformPosition(Corner));
				Points.Emplace(World.X, World.Y);
			}
		};
		for (const FKConvexElem& Convex : Body->AggGeom.ConvexElems)
		{
			if (bWalkOnWide && Convex.GetName() == FName(TEXT("Deck")))
			{
				// Made to be walked on (the ramps of the bridge): never in the way, however narrow one piece of it is.
				continue;
			}
			Add(Convex.GetTransform(), Convex.VertexData);
		}
		for (const FKBoxElem& Box : Body->AggGeom.BoxElems)
		{
			if (bWalkOnWide && Box.GetName() == FName(TEXT("Deck")))
			{
				// Made to be walked on (the ramps of the bridge): never in the way, however narrow one piece of it is.
				continue;
			}
			const FVector Half(Box.X * 0.5, Box.Y * 0.5, Box.Z * 0.5);
			const FVector Corners[] = { FVector(-Half.X, -Half.Y, 0.0), FVector(Half.X, -Half.Y, 0.0), FVector(Half.X, Half.Y, 0.0), FVector(-Half.X, Half.Y, 0.0) };
			Add(Box.GetTransform(), Corners);
		}
	}
	if (bWalkOnWide)
	{
		// A bridge: each narrow piece (a rail, a post) blocks by itself; the wide ones are the deck.
		for (const TArray<FVector2D>& Points : Shapes)
		{
			MakeFootprintObstacles(Points, WalkOnWidth, OutObstacles);
		}
		return;
	}
	if (Shapes.Num() == 0)
	{
		// No collision was made for it: its bounds, drawn in.
		MakeBuildingObstacles(Mesh.GetBoundingBox(), Placed, OutObstacles);
		return;
	}
	// A building: all its shapes together on one grid, so a place covered by its walls, its roof and its chimney is
	// blocked once. Each row of the grid becomes as few long boxes as cover it.
	TArray<TArray<FVector2D>> Outlines;
	FBox2D Around(ForceInit);
	for (const TArray<FVector2D>& Points : Shapes)
	{
		TArray<FVector2D> Outline;
		if (MakeGroundOutline(Points, Outline))
		{
			for (const FVector2D& Corner : Outline)
			{
				Around += Corner;
			}
			Outlines.Add(MoveTemp(Outline));
		}
	}
	if (Outlines.Num() == 0)
	{
		return;
	}
	const FVector2D Size = Around.GetSize();
	const int32 CountX = FMath::Clamp(FMath::CeilToInt32(Size.X / BuildingCellSize), 1, 64);
	const int32 CountY = FMath::Clamp(FMath::CeilToInt32(Size.Y / BuildingCellSize), 1, 64);
	const FVector2D Step(Size.X / CountX, Size.Y / CountY);
	const double Slack = FMath::Min(Step.X, Step.Y) * 0.3;
	for (int32 Y = 0; Y < CountY; ++Y)
	{
		int32 RunStart = INDEX_NONE;
		for (int32 X = 0; X <= CountX; ++X)
		{
			bool bBlocked = false;
			if (X < CountX)
			{
				const FVector2D Middle(Around.Min.X + (X + 0.5) * Step.X, Around.Min.Y + (Y + 0.5) * Step.Y);
				for (int32 Index = 0; Index < Outlines.Num() && !bBlocked; ++Index)
				{
					bBlocked = IsInGroundOutline(Outlines[Index], Middle, Slack);
				}
			}
			if (bBlocked && RunStart == INDEX_NONE)
			{
				RunStart = X;
			}
			else if (!bBlocked && RunStart != INDEX_NONE)
			{
				FCrowdObstacle Box;
				Box.Center = FVector2D(Around.Min.X + (RunStart + X) * 0.5 * Step.X, Around.Min.Y + (Y + 0.5) * Step.Y);
				Box.HalfSize = FVector2D((X - RunStart) * 0.5 * Step.X, Step.Y * 0.5);
				OutObstacles.Add(Box);
				RunStart = INDEX_NONE;
			}
		}
	}
}

void UCrowdSubsystem::SetHouseBoxesShown(bool bShown)
{
	if (Renderer && Renderer->AreHouseBoxesShown() != bShown)
	{
		Renderer->SetHouseBoxesShown(bShown);
		LoadMap(Layout.Name);
	}
}

bool UCrowdSubsystem::AreHouseBoxesShown() const
{
	return !Renderer || Renderer->AreHouseBoxesShown();
}

TArray<FCrowdObstacle> UCrowdSubsystem::MakePracticeObstacles()
{
	return FLevelLayout::MakePractice().Walls;
}

void UCrowdSubsystem::SetObstaclesEnabled(bool bEnabled)
{
	bObstaclesEnabled = bEnabled;
	ApplyObstacles();
}

void UCrowdSubsystem::ApplyObstacles()
{
	TArray<FCrowdObstacle> Obstacles;
	if (bObstaclesEnabled)
	{
		Obstacles = Layout.MakeObstacles();
		Obstacles.Append(PlacedBuildings);
	}
	Sim.SetObstacles(Obstacles);
	if (Renderer)
	{
		// Plain walls are drawn as gray boxes. The level's pieces are drawn by kind.
		Renderer->SetObstacles(bObstaclesEnabled ? Layout.Walls : TArray<FCrowdObstacle>());
	}
}

void UCrowdSubsystem::PlaceWizardAtStart()
{
	if (AWizardPawn* WizardPawn = Wizard.Get())
	{
		const FVector Location = WizardPawn->GetActorLocation();
		WizardPawn->SetActorLocation(FVector(Layout.Start.X, Layout.Start.Y, Location.Z));
	}
}

void UCrowdSubsystem::RegisterWizard(AWizardPawn* InWizard)
{
	Wizard = InWizard;
	PlaceWizardAtStart();
}

bool UCrowdSubsystem::ToggleShrine()
{
	if (Shrine.IsOpen())
	{
		Shrine.Close();
	}
	else if (Wizard.IsValid() && !Meter.IsFalling())
	{
		Shrine.TryOpen(Wizard->GetPosition2D());
	}
	return Shrine.IsOpen();
}

int32 UCrowdSubsystem::GetMarksmanCount() const
{
	int32 Count = 0;
	for (const FCrowdUnit& Unit : Sim.GetUnits())
	{
		Count += Unit.Team == ECrowdTeam::Army && Unit.Kind == ECrowdKind::Marksman ? 1 : 0;
	}
	return Count;
}

FString UCrowdSubsystem::GetGateStateName() const
{
	return !Gate.IsPlaced() ? TEXT("none") : Gate.IsEntered() ? TEXT("entered") : Gate.IsOpen() ? TEXT("open") : TEXT("shut");
}

void UCrowdSubsystem::TakeArea()
{
	bAreaTaken = true;
	AreaTakenSeconds = 0.0f;
}

bool UCrowdSubsystem::OpenGate()
{
	const bool bWasOpen = Gate.IsOpen();
	Gate.Open();
	if (Gate.IsOpen() && GateActor.IsValid())
	{
		GateActor->SetOpenShown(true);
	}
	if (Gate.IsOpen() && !bWasOpen)
	{
		// Heard wherever the wizard stands: it tells him the way is open, however far the gate is.
		PlayCue(EWizardCue::GateOpen, Wizard.IsValid() ? Wizard->GetPosition2D() : Gate.GetCenter());
	}
	return Gate.IsOpen();
}

bool UCrowdSubsystem::IsWizardAtShrine() const
{
	return Wizard.IsValid() && Shrine.IsInRange(Wizard->GetPosition2D());
}

void UCrowdSubsystem::RequestRestart()
{
	if (!bRestarting)
	{
		RestartRun();
	}
}

void UCrowdSubsystem::FireBolt(FVector From, FVector Direction)
{
	// No Bolt while the shrine's menu is open.
	if (!Shrine.AllowsAbilities())
	{
		return;
	}
	Sim.FireBolt(FVector2D(From.X, From.Y), FVector2D(Direction.X, Direction.Y));
	++BoltsFired;
	PlayCue(EWizardCue::FireThrow, FVector2D(From.X, From.Y));
}

int32 UCrowdSubsystem::SpawnEnemies(int32 Count, ECrowdKind Kind)
{
	const FVector2D Center = Wizard.IsValid() ? Wizard->GetPosition2D() : FVector2D::ZeroVector;
	int32 Spawned = 0;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const double Angle = FMath::FRandRange(0.0, UE_DOUBLE_TWO_PI);
		const double Distance = FMath::FRandRange(static_cast<double>(SpawnRingInner), static_cast<double>(SpawnRingOuter));
		if (!Sim.AddEnemy(Kind, Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Distance))
		{
			break;
		}
		++Spawned;
	}
	return Spawned;
}

int32 UCrowdSubsystem::SpawnEnemiesAt(int32 Count, ECrowdKind Kind, FVector2D Point, float Scatter)
{
	int32 Spawned = 0;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const double Angle = FMath::FRandRange(0.0, UE_DOUBLE_TWO_PI);
		if (!Sim.AddEnemy(Kind, Point + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * FMath::FRandRange(0.0, static_cast<double>(Scatter))))
		{
			break;
		}
		++Spawned;
	}
	return Spawned;
}

int32 UCrowdSubsystem::AddSkeletons(int32 Count, ECrowdKind Kind)
{
	const FVector2D Center = Wizard.IsValid() ? Wizard->GetPosition2D() : FVector2D::ZeroVector;
	int32 Added = 0;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const double Angle = FMath::FRandRange(0.0, UE_DOUBLE_TWO_PI);
		const double Distance = FMath::FRandRange(80.0, static_cast<double>(Sim.Tuning.RoamRadius));
		if (!Sim.AddSkeleton(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Distance, Kind))
		{
			break;
		}
		++Added;
	}
	return Added;
}

bool UCrowdSubsystem::ThrowSpear(FVector2D Target)
{
	if (!Wizard.IsValid() || Meter.IsFalling() || !Shrine.AllowsAbilities())
	{
		return false;
	}
	if (!Sim.ThrowSpear(Wizard->GetPosition2D(), Target))
	{
		PlayCue(EWizardCue::Denied, Wizard->GetPosition2D());
		return false;
	}
	++SpearsThrown;
	return true;
}

bool UCrowdSubsystem::StartBlink(FVector2D Direction)
{
	if (!Wizard.IsValid() || Meter.IsFalling() || !Shrine.AllowsAbilities())
	{
		return false;
	}
	if (!Blink.Start(Wizard->GetPosition2D(), Direction, Sim.Tuning.BlinkRange))
	{
		PlayCue(EWizardCue::Denied, Wizard->GetPosition2D());
		return false;
	}
	++BlinksCast;
	PlayCue(EWizardCue::Blink, Wizard->GetPosition2D());
	return true;
}

FString UCrowdSubsystem::GetBlinkStateName() const
{
	return Blink.IsActive() ? TEXT("active") : Blink.GetCooldown() > 0.0f ? TEXT("cooldown") : TEXT("ready");
}

float UCrowdSubsystem::GetUnitHeight(ECrowdKind Kind) const
{
	return Sim.GetUnitHeight(Kind, Wizard.IsValid() ? Wizard->GetModelHeight() : AWizardPawn::ModelHeightDefault);
}

int32 UCrowdSubsystem::Summon()
{
	if (!Wizard.IsValid() || Meter.IsFalling() || !Shrine.AllowsAbilities())
	{
		return 0;
	}
	const int32 Raised = Sim.Summon(Wizard->GetPosition2D());
	TotalRaised += Raised;
	PlayCue(Raised > 0 ? EWizardCue::Summoned : EWizardCue::Denied, Wizard->GetPosition2D());
	return Raised;
}

int32 UCrowdSubsystem::Buy(ECrowdShopItem Item, int32 Count)
{
	if (!Wizard.IsValid() || Meter.IsFalling())
	{
		return 0;
	}
	const int32 Bought = Sim.Buy(Item, Count, Wizard->GetPosition2D());
	if (Bought <= 0)
	{
		PlayCue(EWizardCue::Denied, Wizard->GetPosition2D());
	}
	if (Item == ECrowdShopItem::Guard || Item == ECrowdShopItem::Marksman)
	{
		TotalRaised += Bought;
	}
	return Bought;
}

int32 UCrowdSubsystem::GetResearchCount() const
{
	int32 Count = 0;
	for (int32 Item = 0; Item < CrowdShopItemCount; ++Item)
	{
		Count += Sim.IsResearched(static_cast<ECrowdShopItem>(Item)) ? 1 : 0;
	}
	return Count;
}

int32 UCrowdSubsystem::SpawnGarrison(FVector2D Post, int32 Grunts, int32 Brutes, int32 InArchers, float Radius)
{
	const int32 Squad = NextSquad++;
	int32 Placed = 0;
	// A post with ground of its own spreads its group over it; the brutes, who need most room, take the first spots
	// only after the middle, which a commander may stand on.
	const int32 Archers = InArchers >= 0 ? InArchers : FMath::Clamp(FMath::RoundToInt32(Grunts * Sim.Tuning.PostArcherShare), 0, Grunts);
	const int32 PlainGrunts = InArchers >= 0 ? Grunts : Grunts - Archers;
	const TArray<FVector2D> Spots = Radius > 0.0f ? MakePostSpots(Post, PlainGrunts + Archers + Brutes + 1, Radius) : TArray<FVector2D>();
	int32 NextSpot = 1;
	const auto Place = [this, Post, Squad, Radius, &Placed, &Spots, &NextSpot](ECrowdKind Kind, int32 Count)
	{
		for (int32 Index = 0; Index < Count; ++Index)
		{
			if (Spots.IsValidIndex(NextSpot))
			{
				Placed += Sim.AddGarrisonEnemy(Kind, Spots[NextSpot++], Squad, Radius, Post) ? 1 : 0;
				continue;
			}
			const double Angle = FMath::FRandRange(0.0, UE_DOUBLE_TWO_PI);
			const double Distance = FMath::FRandRange(0.0, static_cast<double>(PostScatter));
			Placed += Sim.AddGarrisonEnemy(Kind, Post + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Distance, Squad) ? 1 : 0;
		}
	};
	Place(ECrowdKind::Brute, Brutes);
	Place(ECrowdKind::Grunt, PlainGrunts);
	Place(ECrowdKind::Archer, Archers);
	return Placed;
}

bool UCrowdSubsystem::SpawnCommander(FVector2D Post, int32 EscortGrunts, int32 EscortBrutes, int32 EscortArchers, float Radius)
{
	const int32 Squad = NextSquad;
	if (!Sim.AddGarrisonEnemy(ECrowdKind::Commander, Post, Squad))
	{
		return false;
	}
	// The escort stands in the commander's group, so they wake together.
	const int32 Escort = SpawnGarrison(Post, EscortGrunts, EscortBrutes, EscortArchers, Radius);
	bCommanderPlaced = true;
	bAreaTaken = false;
	PatrolTimer = Sim.Tuning.PatrolInterval;
	UE_LOG(LogWizard, Log, TEXT("Commander placed at %.0f, %.0f with an escort of %d"), Post.X, Post.Y, Escort);
	return true;
}

int32 UCrowdSubsystem::SendPatrol()
{
	const FVector2D Center = Wizard.IsValid() ? Wizard->GetPosition2D() : FVector2D::ZeroVector;
	FVector2D From;
	if (!Sim.GetNearestPost(Center, From))
	{
		return 0;
	}
	// It comes from the next manned post, but appears close enough to find its way to the wizard.
	const FVector2D Offset = From - Center;
	const double Distance = Offset.Size();
	if (Distance > PatrolSpawnDistance)
	{
		From = Center + Offset / Distance * PatrolSpawnDistance;
	}
	int32 Sent = 0;
	const int32 Size = FMath::RoundToInt32(Sim.Tuning.PatrolSize);
	for (int32 Index = 0; Index < Size; ++Index)
	{
		const double Angle = FMath::FRandRange(0.0, UE_DOUBLE_TWO_PI);
		Sent += Sim.AddEnemy(ECrowdKind::Grunt, From + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * FMath::FRandRange(0.0, static_cast<double>(PostScatter))) ? 1 : 0;
	}
	PatrolsSent += Sent > 0 ? 1 : 0;
	UE_LOG(LogWizard, Log, TEXT("Patrol sent: %d grunts from %.0f, %.0f"), Sent, From.X, From.Y);
	return Sent;
}

bool UCrowdSubsystem::SendAssassin()
{
	const FVector2D Center = Wizard.IsValid() ? Wizard->GetPosition2D() : FVector2D::ZeroVector;
	FVector2D From;
	if (!Sim.GetNearestPost(Center, From))
	{
		return false;
	}
	// Like a patrol, it comes from the next manned post, but appears close enough to find its way to the wizard.
	const FVector2D Offset = From - Center;
	const double Distance = Offset.Size();
	if (Distance > PatrolSpawnDistance)
	{
		From = Center + Offset / Distance * PatrolSpawnDistance;
	}
	if (!Sim.AddEnemy(ECrowdKind::Assassin, From))
	{
		return false;
	}
	++AssassinsSent;
	UE_LOG(LogWizard, Log, TEXT("Assassin sent from %.0f, %.0f"), From.X, From.Y);
	return true;
}

bool UCrowdSubsystem::ToggleCommandMode()
{
	SetCommandMode(!Command.IsActive());
	return Command.IsActive();
}

void UCrowdSubsystem::SetCommandMode(bool bOn)
{
	if (!bOn)
	{
		Command.Exit();
	}
	else if (Wizard.IsValid() && !Meter.IsFalling() && Shrine.AllowsAbilities())
	{
		// Nothing happens with no army to send.
		Command.Enter(Sim.CountCommandable(ECrowdCommandKind::Any));
	}
}

void UCrowdSubsystem::ScrollCommand(int32 Notches)
{
	if (Command.IsActive())
	{
		Command.Scroll(Notches, GetCommandAvailable());
	}
}

void UCrowdSubsystem::CycleCommandKind()
{
	if (Command.IsActive())
	{
		// Only a kind the army has with the wizard right now can be chosen.
		const int32 Available[CrowdCommandKindCount] = { Sim.CountCommandable(ECrowdCommandKind::Any),
			Sim.CountCommandable(ECrowdCommandKind::Skeleton), Sim.CountCommandable(ECrowdCommandKind::Guard) };
		Command.CycleKind(Available);
		Command.Clamp(GetCommandAvailable());
	}
}

int32 UCrowdSubsystem::ConfirmCommand(FVector2D Point)
{
	if (!Command.IsActive())
	{
		return 0;
	}
	const int32 Sent = SendCommand(Point, Command.GetCount(), Command.GetKind());
	Command.Exit();
	return Sent;
}

int32 UCrowdSubsystem::SendCommand(FVector2D Point, int32 Count, ECrowdCommandKind Kind)
{
	if (!Wizard.IsValid() || Meter.IsFalling())
	{
		return 0;
	}
	return Sim.Command(Point, Count, Kind);
}

int32 UCrowdSubsystem::Recall()
{
	return Sim.Recall();
}

int32 UCrowdSubsystem::AddBones(int32 Count)
{
	const FVector2D Center = Wizard.IsValid() ? Wizard->GetPosition2D() : FVector2D::ZeroVector;
	return Count > 0 && Sim.DropBone(Center, Count) ? Count : 0;
}

bool UCrowdSubsystem::IsSeeThroughReady() const
{
	return Renderer && Renderer->GetSeeThroughCount() > 0;
}

int32 UCrowdSubsystem::GetAwayCount() const
{
	const FVector2D Center = Wizard.IsValid() ? Wizard->GetPosition2D() : FVector2D::ZeroVector;
	int32 Count = 0;
	for (const FCrowdUnit& Unit : Sim.GetUnits())
	{
		Count += Unit.Team == ECrowdTeam::Army && !Sim.IsInPresence(Unit, Center) ? 1 : 0;
	}
	return Count;
}

int32 UCrowdSubsystem::GetBonesInView() const
{
	int32 Count = 0;
	for (const FCrowdBone& Bone : Sim.GetBones())
	{
		Count += Sim.IsInView(Bone.Position) ? 1 : 0;
	}
	return Count;
}

void UCrowdSubsystem::SetArmyModelShown(bool bShown)
{
	if (Renderer)
	{
		Renderer->SetArmyModelShown(bShown);
	}
}

bool UCrowdSubsystem::IsArmyModelShown() const
{
	return Renderer && Renderer->IsArmyModelShown();
}

int32 UCrowdSubsystem::GetArmyModelsDrawn() const
{
	return Renderer ? Renderer->GetArmyModelsDrawn() : 0;
}

UWizardAudioSubsystem* UCrowdSubsystem::GetAudio() const
{
	const UWorld* World = GetWorld();
	const UGameInstance* Game = World ? World->GetGameInstance() : nullptr;
	return Game ? Game->GetSubsystem<UWizardAudioSubsystem>() : nullptr;
}

void UCrowdSubsystem::PlayCue(EWizardCue Cue, FVector2D Position, float SkipSeconds) const
{
	if (UWizardAudioSubsystem* Audio = GetAudio())
	{
		Audio->PlayCueAt(Cue, Position, Wizard.IsValid() ? Wizard->GetPosition2D() : Position, SkipSeconds);
	}
}

FString UCrowdSubsystem::GetEffectSummary() const
{
	return Renderer ? Renderer->GetEffectSummary() : FString();
}

FString UCrowdSubsystem::GetWeaponSummary() const
{
	return Renderer ? Renderer->GetWeaponSummary() : FString();
}

void UCrowdSubsystem::PlaySounds(FVector2D WizardPosition)
{
	const double Now = Sim.GetClock();
	for (const FCrowdEvent& Event : Sim.GetEvents())
	{
		switch (Event.Type)
		{
		case ECrowdEvent::ArmyBlow:
			PlayCue(FWizardSoundRules::ForArmyBlow(Event.Kind), Event.Position);
			break;
		case ECrowdEvent::BoltHit:
		case ECrowdEvent::BoltWall:
			PlayCue(EWizardCue::FireImpact, Event.Position);
			break;
		case ECrowdEvent::SlamMiss:
			PlayCue(EWizardCue::HammerHitsGround, Event.Position);
			break;
		case ECrowdEvent::ArrowLoosed:
			PlayCue(EWizardCue::BowShoot, Event.Position);
			break;
		case ECrowdEvent::AssassinMiss:
			PlayCue(EWizardCue::DaggerMiss, Event.Position);
			break;
		case ECrowdEvent::BonePicked:
			PlayCue(EWizardCue::BonePickup, Event.Position);
			break;
		case ECrowdEvent::SpearThrown:
			// Its sound flies with the spear (below), not left where it was thrown from.
			if (UWizardAudioSubsystem* Audio = GetAudio())
			{
				SpearSounds.Add(Event.Id, Audio->PlayCueFollowed(EWizardCue::CommanderSpearThrow, Event.Position, WizardPosition));
			}
			break;
		case ECrowdEvent::Death:
		{
			if (FWizardSoundRules::DiesAsSkeleton(Event.Team, Event.Kind))
			{
				PlayCue(EWizardCue::SkeletonDies, Event.Position);
			}
			// The thud is HEARD when the death that is shown brings the body to the ground (its "PlaySound" notify).
			// Each sound has a stretch of quiet before its thud, so it is started that much early.
			const EDeathFall Fall = FDeathPick::Pick(Event.Facing, Event.Position, Event.From);
			const float Until = Renderer ? Renderer->GetDeathFallSeconds(Fall) : FDeathPick::FallSeconds(Fall);
			FPendingFall& Pending = PendingFalls.AddDefaulted_GetRef();
			Pending.Position = Event.Position;
			Pending.Cue = FWizardSoundRules::ForBodyFall(IsDirtAt(Event.Position), ++BodyFallPicks);
			float Wait = 0.0f;
			FWizardSoundRules::GetLead(Pending.Cue, Until, Wait, Pending.Skip);
			Pending.Time = Now + Wait;
			break;
		}
		default:
			break;
		}
	}
	if (SpearSounds.Num() > 0)
	{
		UWizardAudioSubsystem* Audio = GetAudio();
		TSet<int32> Flying;
		for (const FCrowdJavelin& Javelin : Sim.GetJavelins())
		{
			Flying.Add(Javelin.Id);
			if (const TWeakObjectPtr<UAudioComponent>* Player = SpearSounds.Find(Javelin.Id); Player && Audio)
			{
				Audio->MoveFollowed(Player->Get(), Javelin.Position);
			}
		}
		// A spear that has landed leaves its sound where it ended.
		for (auto It = SpearSounds.CreateIterator(); It; ++It)
		{
			if (!Flying.Contains(It.Key()))
			{
				It.RemoveCurrent();
			}
		}
	}
	for (int32 Index = PendingFalls.Num() - 1; Index >= 0; --Index)
	{
		if (PendingFalls[Index].Time <= Now)
		{
			++BodyFalls;
			PlayCue(PendingFalls[Index].Cue, PendingFalls[Index].Position, PendingFalls[Index].Skip);
			PendingFalls.RemoveAtSwap(Index);
		}
	}
}

void UCrowdSubsystem::LoadGroundPaint()
{
	// Written by Tools/Level/bake_ground_paint.py from the landscape's paint.
	const TCHAR* Section = TEXT("WizardGroundPaint");
	GroundPaint = FGroundPaint();
	double MinX = 0.0, MinY = 0.0, MaxX = 0.0, MaxY = 0.0;
	if (GConfig && GConfig->GetInt(Section, TEXT("Width"), GroundPaint.Width, GGameIni) && GConfig->GetInt(Section, TEXT("Height"), GroundPaint.Height, GGameIni)
		&& GConfig->GetDouble(Section, TEXT("MinX"), MinX, GGameIni) && GConfig->GetDouble(Section, TEXT("MinY"), MinY, GGameIni)
		&& GConfig->GetDouble(Section, TEXT("MaxX"), MaxX, GGameIni) && GConfig->GetDouble(Section, TEXT("MaxY"), MaxY, GGameIni)
		&& GConfig->GetString(Section, TEXT("Cells"), GroundPaint.Cells, GGameIni))
	{
		GroundPaint.Min = FVector2D(MinX, MinY);
		GroundPaint.Max = FVector2D(MaxX, MaxY);
	}
}

int32 UCrowdSubsystem::GetCorpsesShown() const
{
	return Renderer ? Renderer->GetCorpsesShown() : 0;
}

int32 UCrowdSubsystem::GetDeathsShown() const
{
	return Renderer ? Renderer->GetDeathsShown() : 0;
}

FString UCrowdSubsystem::GetLastDeathName() const
{
	return Renderer ? Renderer->GetLastDeathName() : TEXT("none");
}

FString UCrowdSubsystem::GetMusicName() const
{
	const UWizardAudioSubsystem* Audio = GetAudio();
	return Audio ? Audio->GetMusicName() : TEXT("none");
}

int32 UCrowdSubsystem::GetCuePlays() const
{
	const UWizardAudioSubsystem* Audio = GetAudio();
	return Audio ? Audio->GetTotalCuePlays() : 0;
}

FString UCrowdSubsystem::GetLastCueName() const
{
	const UWizardAudioSubsystem* Audio = GetAudio();
	return Audio ? Audio->GetLastCueName() : TEXT("none");
}

int32 UCrowdSubsystem::GetSwingsReady() const
{
	return Renderer ? Renderer->GetSwingsReady() : 0;
}

FString UCrowdSubsystem::GetModelCostText() const
{
	if (!Renderer)
	{
		return TEXT("{}");
	}
	const ACrowdRenderer::FModelCost& Cost = Renderer->GetModelCost();
	const double Frames = FMath::Max(Cost.Frames, 1);
	return FString::Printf(TEXT("{\"frames\":%d,\"frameMs\":%.2f,\"framePeakMs\":%.2f,\"modelMs\":%.3f,\"modelPeakMs\":%.2f,\"creates\":%d,\"createMs\":%.2f,\"switches\":%d,\"switchMs\":%.2f,\"flips\":%d,\"flipMs\":%.2f}"),
		Cost.Frames, Cost.FrameMs / Frames, Cost.FramePeakMs, Cost.ModelMs / Frames, Cost.ModelPeakMs, Cost.Creates, Cost.CreateMs,
		Cost.Switches, Cost.SwitchMs, Cost.Flips, Cost.FlipMs);
}

FString UCrowdSubsystem::GetModelCostLine() const
{
	if (!Renderer)
	{
		return FString();
	}
	const ACrowdRenderer::FModelCost& Cost = Renderer->GetModelCostWindow();
	return FString::Printf(TEXT("Frame: %.1f ms  worst %.0f ms   new %d  pose %d  show %d   test %d"),
		Cost.FrameMs / FMath::Max(Cost.Frames, 1), Cost.FramePeakMs, Cost.Creates, Cost.Switches, Cost.Flips, Renderer->GetPerfTest());
}

void UCrowdSubsystem::SetPerfTest(int32 Mode)
{
	if (Renderer)
	{
		Renderer->SetPerfTest(FMath::Clamp(Mode, 0, ACrowdRenderer::PerfTestCount - 1));
	}
}

int32 UCrowdSubsystem::GetPerfTest() const
{
	return Renderer ? Renderer->GetPerfTest() : 0;
}

void UCrowdSubsystem::ResetModelCost()
{
	if (Renderer)
	{
		Renderer->ResetModelCost();
	}
}

void UCrowdSubsystem::UpdateView(const AWizardPawn& WizardPawn)
{
	// The ground the camera shows, as a box around where the four corners of the screen meet the ground. A corner that
	// looks at the sky (the camera behind his shoulder) counts as far away. The box is a little larger than the
	// screen, which only ever keeps a bone longer.
	const APlayerController* Controller = Cast<APlayerController>(WizardPawn.GetController());
	int32 SizeX = 0;
	int32 SizeY = 0;
	if (Controller)
	{
		Controller->GetViewportSize(SizeX, SizeY);
	}
	if (SizeX <= 0 || SizeY <= 0)
	{
		Sim.ClearView();
		return;
	}
	const double GroundZ = WizardPawn.GetGroundZ();
	FVector2D Min(TNumericLimits<double>::Max(), TNumericLimits<double>::Max());
	FVector2D Max(-TNumericLimits<double>::Max(), -TNumericLimits<double>::Max());
	bool bAny = false;
	for (int32 Corner = 0; Corner < 4; ++Corner)
	{
		FVector Origin;
		FVector Direction;
		if (!Controller->DeprojectScreenPositionToWorld(Corner % 2 == 0 ? 0.0f : static_cast<float>(SizeX), Corner < 2 ? 0.0f : static_cast<float>(SizeY), Origin, Direction))
		{
			continue;
		}
		double Reach = ViewFarthest;
		if (Direction.Z < -UE_KINDA_SMALL_NUMBER)
		{
			Reach = FMath::Min(Reach, (GroundZ - Origin.Z) / Direction.Z);
		}
		const FVector Hit = Origin + Direction * Reach;
		Min = FVector2D(FMath::Min(Min.X, Hit.X), FMath::Min(Min.Y, Hit.Y));
		Max = FVector2D(FMath::Max(Max.X, Hit.X), FMath::Max(Max.Y, Hit.Y));
		bAny = true;
	}
	if (bAny)
	{
		Sim.SetView(Min, Max);
	}
	else
	{
		Sim.ClearView();
	}
}

int32 UCrowdSubsystem::GetUnitsDrawn() const
{
	return Sim.GetUnits().Num() + Sim.GetBolts().Num() + Sim.GetBones().Num() + Sim.GetSpears().Num() + Sim.GetBlasts().Num() + Sim.GetArrows().Num();
}

void UCrowdSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AWizardPawn* WizardPawn = Wizard.Get();
	if (!WizardPawn || bRestarting)
	{
		return;
	}

	// A background tab can hand over a frame several seconds long. Nothing may jump or fill in one frame.
	const float Step = FMath::Clamp(DeltaTime, 0.0f, Sim.Tuning.MaxStep);
	FVector2D WizardPosition = WizardPawn->GetPosition2D();

	// The game is finished: the fight stands still behind the final cut screen.
	if (Gate.IsEntered())
	{
		Gate.Update(true, WizardPosition, false, Step);
		return;
	}

	// Blink: the wizard glides to where the dash ends, and nothing can hurt him on the way.
	// Every unit's size, and the room it takes, is a share of the wizard's height.
	Sim.WizardTall = WizardPawn->GetModelHeight();
	const bool bWasBlinking = Blink.IsActive();
	Blink.SteerEarly = Sim.Tuning.BlinkSteerEarly;
	Blink.Step(Step, Sim.Tuning.BlinkSeconds, Sim.Tuning.BlinkCooldown, WizardPawn->GetMoveDirection(), Sim.Tuning.BlinkSteer,
		Sim.GetObstacles(), BlinkClearance);
	if (bWasBlinking)
	{
		WizardPosition = Blink.GetPosition();
		WizardPawn->SetActorLocation(FVector(WizardPosition.X, WizardPosition.Y, WizardPawn->GetActorLocation().Z));
	}
	Meter.bInvulnerable = Blink.IsActive();
	Sim.bWizardInvulnerable = Blink.IsActive();
	// He is gone from the start of the blink and back as it slows.
	WizardPawn->SetBlinkHidden(IsWizardHidden());
	UpdateView(*WizardPawn);

	// Command mode ends with the fall, the shrine's menu, or nobody left to send; its number follows the army.
	if (Command.IsActive())
	{
		if (Meter.IsFalling() || !Shrine.AllowsAbilities() || Sim.CountCommandable(ECrowdCommandKind::Any) <= 0)
		{
			Command.Exit();
		}
		else
		{
			if (GetCommandAvailable() <= 0)
			{
				// The last of the chosen kind is gone: on to a kind the army still has.
				CycleCommandKind();
			}
			Command.Clamp(GetCommandAvailable());
		}
	}

	// Assassins come one at a time while the commander lives.
	if (!Meter.IsFalling() && bPatrolsEnabled && bCommanderPlaced && !bAreaTaken && Sim.Tuning.AssassinInterval > 0.0f)
	{
		AssassinTimer -= Step;
		if (AssassinTimer <= 0.0f)
		{
			AssassinTimer = Sim.Tuning.AssassinInterval;
			SendAssassin();
		}
	}

	if (!Meter.IsFalling())
	{
		RunSeconds += Step;
		if (bWavesEnabled)
		{
			WaveTimer -= Step;
			if (WaveTimer <= 0.0f)
			{
				WaveTimer = WaveInterval;
				const int32 Spawned = SpawnEnemies(WaveSize(RunSeconds, FMath::RandRange(0, 4)));
				const int32 Brutes = SpawnEnemies(WaveBrutes(RunSeconds), ECrowdKind::Brute);
				UE_LOG(LogWizard, Log, TEXT("Wave spawned: %d grunts and %d brutes at %.0f seconds"), Spawned, Brutes, RunSeconds);
			}
		}
	}

	// Patrols: while the commander lives, a few grunts come from the next manned post now and then, so standing still is never safe.
	if (!Meter.IsFalling() && bPatrolsEnabled && bCommanderPlaced && !bAreaTaken && Sim.Tuning.PatrolInterval > 0.0f)
	{
		PatrolTimer -= Step;
		if (PatrolTimer <= 0.0f)
		{
			PatrolTimer = Sim.Tuning.PatrolInterval;
			SendPatrol();
		}
	}

	// Road patrols: a group that was killed is replaced after a while, as long as the commander lives.
	if (!Meter.IsFalling() && bPatrolsEnabled && bCommanderPlaced && !bAreaTaken && Sim.Tuning.RoadPatrolRespawn > 0.0f)
	{
		for (int32 RouteIndex = 0; RouteIndex < RoadPatrols.Num(); ++RouteIndex)
		{
			FRoadPatrol& Patrol = RoadPatrols[RouteIndex];
			if (Sim.CountSquad(Patrol.Squad) > 0)
			{
				continue;
			}
			Patrol.Timer -= Step;
			if (Patrol.Timer <= 0.0f)
			{
				const int32 Placed = SpawnRoadPatrol(RouteIndex);
				if (Placed > 0)
				{
					UE_LOG(LogWizard, Log, TEXT("Road patrol replaced: %d grunts on road %d"), Placed, RouteIndex);
				}
			}
		}
	}

	// Walking out of the shrine's range closes its menu. So does the fall.
	Shrine.Update(WizardPosition);
	if (Meter.IsFalling())
	{
		Shrine.Close();
	}

	const FCrowdStepResult Result = Sim.Step(Step, WizardPosition);
	PlaySounds(WizardPosition);
	TotalKilled += Result.EnemiesKilled;
	TotalRaised += Result.SkeletonsRaised;
	if (Result.CommandersKilled > 0 && !bAreaTaken)
	{
		bAreaTaken = true;
		AreaTakenSeconds = 0.0f;
		UE_LOG(LogWizard, Log, TEXT("The commander is dead: the area is taken at %.0f seconds"), RunSeconds);
	}
	if (bAreaTaken)
	{
		AreaTakenSeconds += Step;
	}

	// The kingdom gate opens once the commander is dead, and the game finishes when the wizard walks into it.
	const bool bGateWasOpen = Gate.IsOpen();
	const bool bEntered = Gate.Update(bAreaTaken, WizardPosition, !Meter.IsFalling() && !Blink.IsActive(), Step);
	if (Gate.IsOpen() && !bGateWasOpen)
	{
		if (GateActor.IsValid())
		{
			GateActor->SetOpenShown(true);
		}
		// Heard wherever the wizard stands: it tells him the way is open, however far the gate is.
		PlayCue(EWizardCue::GateOpen, Wizard.IsValid() ? Wizard->GetPosition2D() : Gate.GetCenter());
	}
	if (bEntered)
	{
		UE_LOG(LogWizard, Log, TEXT("The wizard enters the kingdom gate: %d raised, %d killed, %.0f seconds"), TotalRaised, TotalKilled, RunSeconds);
		Shrine.Close();
		Command.Exit();
		AppliedTimeScale = 1.0f;
		UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);
		WizardPawn->SetSpeedScale(1.0f);
		WizardPawn->EndRun();
		return;
	}

	// Time runs slower while the shrine's menu is open.
	const float WantedTimeScale = GetTimeScale();
	if (!FMath::IsNearlyEqual(WantedTimeScale, AppliedTimeScale))
	{
		AppliedTimeScale = WantedTimeScale;
		UGameplayStatics::SetGlobalTimeDilation(this, WantedTimeScale);
	}

	// Each arrow that hits him fills a share of the meter at once.
	ArrowHits += Result.ArrowsHitWizard;
	const bool bFelledByArrow = Result.ArrowsHitWizard > 0 && Meter.Add(Result.ArrowsHitWizard * Sim.Tuning.ArrowOverwhelm);

	// A brute's slam that catches him fills a larger share.
	SlamsLanded += Result.SlamsLanded;
	SlamHits += Result.SlamsHitWizard;
	const bool bFelledBySlam = Result.SlamsHitWizard > 0 && Meter.Add(Result.SlamsHitWizard * Sim.Tuning.BruteSlamOverwhelm);

	// Its sweep fills less.
	SweepsLanded += Result.SweepsLanded;
	SweepHits += Result.SweepsHitWizard;
	const bool bFelledBySweep = Result.SweepsHitWizard > 0 && Meter.Add(Result.SweepsHitWizard * Sim.Tuning.BruteSweepOverwhelm);

	// The commander's slash fills more than a slam. His spear fills a share and slows the wizard for a while.
	SlashHits += Result.SlashesHitWizard;
	const bool bFelledBySlash = Result.SlashesHitWizard > 0 && Meter.Add(Result.SlashesHitWizard * Sim.Tuning.CommanderSlashOverwhelm);
	JavelinHits += Result.JavelinsHitWizard;
	const bool bFelledBySpear = Result.JavelinsHitWizard > 0 && Meter.Add(Result.JavelinsHitWizard * Sim.Tuning.CommanderSpearOverwhelm);
	if (Result.JavelinsHitWizard > 0)
	{
		SlowTimer = Sim.Tuning.CommanderSpearSlowSeconds;
	}
	SlowTimer = FMath::Max(SlowTimer - Step, 0.0f);
	WizardPawn->SetSpeedScale(SlowTimer > 0.0f ? FMath::Clamp(Sim.Tuning.CommanderSpearSlow, 0.1f, 1.0f) : 1.0f);

	// With the commander near, everything on the wizard fills the meter faster.
	bCommanderNear = Sim.IsCommanderNear(WizardPosition);
	Sim.GetEnemiesTouching(WizardPosition, TouchingOffsets);
	// Outside the battlefield he is warned; too long out, or too far, and the mission fails.
	if (Battlefield.Update(WizardPosition, Step, Sim.Tuning.BattlefieldWarnSeconds, Sim.Tuning.BattlefieldFarDistance) && Meter.Fall())
	{
		bLeftBattlefield = true;
		UE_LOG(LogWizard, Log, TEXT("He left the battlefield: %.0f seconds"), RunSeconds);
		WizardPawn->BeginFall();
	}
	else if (Meter.Step(Step, TouchingOffsets, bCommanderNear ? Sim.Tuning.CommanderAuraFill : 1.0f) || bFelledByArrow || bFelledBySlam || bFelledBySweep || bFelledBySlash || bFelledBySpear)
	{
		UE_LOG(LogWizard, Log, TEXT("The god falls: %d raised, %d killed, %.0f seconds"), TotalRaised, TotalKilled, RunSeconds);
		WizardPawn->BeginFall();
	}

	if (Renderer)
	{
		// The circle that cuts buildings away follows the mouse.
		SeeThroughShown = FVector2D(-10.0, -10.0);
		float Aspect = 16.0f / 9.0f;
		if (const APlayerController* Controller = Cast<APlayerController>(WizardPawn->GetController()))
		{
			int32 SizeX = 0;
			int32 SizeY = 0;
			Controller->GetViewportSize(SizeX, SizeY);
			float MouseX = 0.0f;
			float MouseY = 0.0f;
			if (SizeX > 0 && SizeY > 0)
			{
				Aspect = static_cast<float>(SizeX) / SizeY;
				if (SeeThroughForced.X >= 0.0 && SeeThroughForced.Y >= 0.0)
				{
					SeeThroughShown = SeeThroughForced;
				}
				else if (Controller->GetMousePosition(MouseX, MouseY))
				{
					SeeThroughShown = FVector2D(MouseX / SizeX, MouseY / SizeY);
				}
			}
		}
		Renderer->SetSeeThrough(SeeThroughShown, Aspect, SeeThroughRadius);
		Renderer->Refresh(Sim, WizardPawn->GetGroundZ(), WizardPawn->GetModelHeight(), WizardPosition);
	}

	if (Meter.IsFallFinished())
	{
		RestartRun();
	}
}

void UCrowdSubsystem::RestartRun()
{
	bRestarting = true;
	UWorld* World = GetWorld();
	const FString MapPath = UWorld::RemovePIEPrefix(World->GetOutermost()->GetName());
	UE_LOG(LogWizard, Log, TEXT("Run restarts: %s"), *MapPath);
	UGameplayStatics::OpenLevel(World, FName(*MapPath));
}
