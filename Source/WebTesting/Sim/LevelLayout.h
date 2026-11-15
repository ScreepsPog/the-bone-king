#pragma once

#include "CoreMinimal.h"
#include "Sim/CrowdTypes.h"
#include "LevelLayout.generated.h"

/** What a piece of a level is. The picture gives each kind its own shape and color; meshes replace them later. */
UENUM()
enum class ELevelPiece : uint8
{
	House,
	Medical,
	Dock,
	Coal,
	Tree,
	River,
	Bridge,
	Path,
	Farmland,
	Graveyard,
	Lamp,
	Count UMETA(Hidden)
};

/** One piece of a level: a box on the ground, turned by Yaw (degrees). Pieces that block are never turned. */
struct FLevelPiece
{
	ELevelPiece Kind = ELevelPiece::House;
	FVector2D Center = FVector2D::ZeroVector;
	FVector2D HalfSize = FVector2D(50.0, 50.0);
	float Yaw = 0.0f;
	// A box's height, as a share of its kind's height. Set by a piece placed in a level and scaled there.
	float HeightScale = 1.0f;
};

/** A post: a group of enemies that stands here until the wizard comes. */
struct FLevelPost
{
	FVector2D Position = FVector2D::ZeroVector;
	int32 Grunts = 0;
	int32 Brutes = 0;
	// Archers on top of the grunts; below zero, a share of the grunts are archers instead (Tuning.PostArcherShare).
	int32 Archers = -1;
	// The ground the group stands and walks about on. 0: the old scatter and Tuning.PostWanderRadius.
	float Radius = 0.0f;
};

/**
 * A whole level as plain data: where things stand, where the wizard starts, the shrine, the posts and the commander.
 * No engine objects, so the layout can be checked in unit tests (can the wizard walk from the start to the commander?).
 */
struct FLevelLayout
{
	FString Name;
	FVector2D Start = FVector2D::ZeroVector;
	FVector2D Shrine = FVector2D::ZeroVector;
	bool bHasCommander = false;
	FVector2D Commander = FVector2D::ZeroVector;
	int32 EscortGrunts = 0;
	int32 EscortBrutes = 0;
	int32 EscortArchers = -1;
	float CommanderRadius = 0.0f;
	// How close the wizard must be to use the shrine. 0: the shrine's own default.
	float ShrineRange = 0.0f;
	// The kingdom gate: the game finishes when the wizard walks into it after the commander is dead.
	bool bHasGate = false;
	FVector2D Gate = FVector2D::ZeroVector;
	// Half its depth (the way through) and half its width, before the turn by GateYaw (degrees).
	FVector2D GateHalfSize = FVector2D(150.0, 300.0);
	float GateYaw = 0.0f;
	// The battlefield's edge, when the level has one: a box with no wall. (Then the level has no walls around it.)
	bool bHasBattlefield = false;
	FVector2D Battlefield = FVector2D::ZeroVector;
	FVector2D BattlefieldHalfSize = FVector2D::ZeroVector;
	// Grunts in the patrol of each road, in the order of Routes. Missing or below zero: Tuning.RoadPatrolSize.
	TArray<int32> RouteSizes;
	// The old test bed: waves that spawn in a ring and charge.
	bool bWaves = false;

	TArray<FLevelPiece> Pieces;
	TArray<FLevelPost> Posts;
	// Roads that patrols walk, each a line of points with nothing in the way between one and the next.
	TArray<TArray<FVector2D>> Routes;
	// Plain walls, drawn as gray boxes: the practice layout.
	TArray<FCrowdObstacle> Walls;

	/** True for a piece nothing can walk through. */
	static bool Blocks(ELevelPiece Kind);

	/** True for a piece that stops feet but not Bolts or the spear: water. */
	static bool IsLow(ELevelPiece Kind);

	static const TCHAR* GetPieceName(ELevelPiece Kind);

	int32 CountPieces(ELevelPiece Kind) const;

	/** Enemies at every post and in the commander's escort. */
	int32 CountPostEnemies() const;

	/** Everything that blocks, as the simulation's boxes. */
	TArray<FCrowdObstacle> MakeObstacles() const;

	/** True when a unit can walk from one point to the other, around everything that blocks. Slow: for tests and tools. */
	bool CanWalk(FVector2D From, FVector2D To, double Clearance = 40.0) const;

	/** The Outskirts, from the director's drawing of 2026-11-12: river, two bridges, roads, farmland, houses, graveyards. */
	static FLevelLayout MakeOutskirts(double UnitsPerPixel = OutskirtsUnitsPerPixel);

	/** Units for one pixel of the drawing. 10 at first; larger so a house is a house beside the wizard. */
	static constexpr double OutskirtsUnitsPerPixel = 15.0;

	/** The first test bed: ten walls around the start, waves from every side. */
	static FLevelLayout MakePractice();

	/** Makes a layout by name (Outskirts, Practice). False for an unknown name. */
	static bool Make(const FString& InName, FLevelLayout& OutLayout, double UnitsPerPixel = OutskirtsUnitsPerPixel);

	static TArray<FString> GetNames() { return { TEXT("Outskirts"), TEXT("Practice") }; }
};
