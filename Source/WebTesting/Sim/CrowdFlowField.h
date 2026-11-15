#pragma once

#include "Sim/CrowdTypes.h"

/**
 * "Which way to the wizard" for every square of a grid around him, going around walls.
 * Built once each time the wizard steps into another square; every unit then reads its direction with one lookup.
 */
class FCrowdFlowField
{
public:
	static constexpr int32 Size = 64;
	static constexpr double CellSize = 100.0;

	/** Clearance is how far a unit's middle must stay from a wall. */
	void Build(FVector2D Target, TConstArrayView<FCrowdObstacle> Obstacles, double Clearance);

	/** True when the field was built for the square this target stands in. */
	bool IsBuiltFor(FVector2D Target) const;

	void Invalidate() { bBuilt = false; }

	/**
	 * Unit direction along the shortest open way to the target. Zero when the position is outside the grid,
	 * cut off from the target, or already in the target's square.
	 */
	FVector2D GetDirection(FVector2D Position) const;

private:
	static FIntPoint ToCell(FVector2D Position);
	static int32 ToIndex(int32 X, int32 Y) { return Y * Size + X; }
	static bool IsInside(int32 X, int32 Y) { return X >= 0 && Y >= 0 && X < Size && Y < Size; }

	// The world square that sits at grid position (0, 0).
	FIntPoint Origin = FIntPoint::ZeroValue;
	FIntPoint TargetCell = FIntPoint::ZeroValue;
	TArray<int32> Costs;
	TArray<bool> Blocked;
	TArray<uint64> Heap;
	bool bBuilt = false;
};
