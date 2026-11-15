#include "Sim/CrowdFlowField.h"

namespace
{
	const int32 Unreachable = MAX_int32;
	const int32 StraightCost = 10;
	const int32 DiagonalCost = 14;

	const FIntPoint FlowNeighbors[] = {
		FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1),
		FIntPoint(1, 1), FIntPoint(1, -1), FIntPoint(-1, 1), FIntPoint(-1, -1) };
}

FIntPoint FCrowdFlowField::ToCell(FVector2D Position)
{
	return FIntPoint(FMath::FloorToInt32(Position.X / CellSize), FMath::FloorToInt32(Position.Y / CellSize));
}

bool FCrowdFlowField::IsBuiltFor(FVector2D Target) const
{
	return bBuilt && ToCell(Target) == TargetCell;
}

void FCrowdFlowField::Build(FVector2D Target, TConstArrayView<FCrowdObstacle> Obstacles, double Clearance)
{
	TargetCell = ToCell(Target);
	Origin = TargetCell - FIntPoint(Size / 2, Size / 2);

	// A square is closed when its middle is too close to a wall for a unit to stand there.
	Blocked.Init(false, Size * Size);
	for (const FCrowdObstacle& Obstacle : Obstacles)
	{
		const FVector2D Reach = Obstacle.HalfSize + FVector2D(Clearance, Clearance);
		const FIntPoint Low = ToCell(Obstacle.Center - Reach) - Origin;
		const FIntPoint High = ToCell(Obstacle.Center + Reach) - Origin;
		for (int32 Y = FMath::Max(Low.Y, 0); Y <= FMath::Min(High.Y, Size - 1); ++Y)
		{
			for (int32 X = FMath::Max(Low.X, 0); X <= FMath::Min(High.X, Size - 1); ++X)
			{
				const FVector2D Middle((Origin.X + X + 0.5) * CellSize, (Origin.Y + Y + 0.5) * CellSize);
				if (Obstacle.Contains(Middle, Clearance))
				{
					Blocked[ToIndex(X, Y)] = true;
				}
			}
		}
	}

	// Shortest distance from every open square to the target's square, spreading outward from the target.
	Costs.Init(Unreachable, Size * Size);
	Heap.Reset();
	const int32 Start = ToIndex(Size / 2, Size / 2);
	Blocked[Start] = false;
	Costs[Start] = 0;
	Heap.HeapPush(static_cast<uint64>(Start));
	while (Heap.Num() > 0)
	{
		uint64 Top = 0;
		Heap.HeapPop(Top, EAllowShrinking::No);
		const int32 Cost = static_cast<int32>(Top >> 32);
		const int32 Index = static_cast<int32>(Top & 0xffffffffu);
		if (Cost > Costs[Index])
		{
			continue;
		}
		const int32 X = Index % Size;
		const int32 Y = Index / Size;
		for (const FIntPoint& Step : FlowNeighbors)
		{
			const int32 NextX = X + Step.X;
			const int32 NextY = Y + Step.Y;
			if (!IsInside(NextX, NextY) || Blocked[ToIndex(NextX, NextY)])
			{
				continue;
			}
			const bool bDiagonal = Step.X != 0 && Step.Y != 0;
			// No cutting across the corner of a wall.
			if (bDiagonal && (Blocked[ToIndex(NextX, Y)] || Blocked[ToIndex(X, NextY)]))
			{
				continue;
			}
			const int32 NextCost = Cost + (bDiagonal ? DiagonalCost : StraightCost);
			const int32 NextIndex = ToIndex(NextX, NextY);
			if (NextCost < Costs[NextIndex])
			{
				Costs[NextIndex] = NextCost;
				Heap.HeapPush((static_cast<uint64>(NextCost) << 32) | static_cast<uint64>(NextIndex));
			}
		}
	}
	bBuilt = true;
}

FVector2D FCrowdFlowField::GetDirection(FVector2D Position) const
{
	if (!bBuilt)
	{
		return FVector2D::ZeroVector;
	}
	const FIntPoint Cell = ToCell(Position) - Origin;
	if (!IsInside(Cell.X, Cell.Y))
	{
		return FVector2D::ZeroVector;
	}

	// A unit pressed against a wall can stand in a closed square: any open neighbor will do to get it out.
	const bool bSelfBlocked = Blocked[ToIndex(Cell.X, Cell.Y)];
	int32 BestCost = Costs[ToIndex(Cell.X, Cell.Y)];
	FIntPoint BestStep = FIntPoint::ZeroValue;
	for (const FIntPoint& Step : FlowNeighbors)
	{
		const int32 NextX = Cell.X + Step.X;
		const int32 NextY = Cell.Y + Step.Y;
		if (!IsInside(NextX, NextY))
		{
			continue;
		}
		const int32 Cost = Costs[ToIndex(NextX, NextY)];
		if (Cost >= BestCost)
		{
			continue;
		}
		const bool bDiagonal = Step.X != 0 && Step.Y != 0;
		if (bDiagonal && !bSelfBlocked && (Blocked[ToIndex(NextX, Cell.Y)] || Blocked[ToIndex(Cell.X, NextY)]))
		{
			continue;
		}
		BestCost = Cost;
		BestStep = Step;
	}
	if (BestStep == FIntPoint::ZeroValue)
	{
		return FVector2D::ZeroVector;
	}
	const FVector2D NextMiddle((Origin.X + Cell.X + BestStep.X + 0.5) * CellSize, (Origin.Y + Cell.Y + BestStep.Y + 0.5) * CellSize);
	return (NextMiddle - Position).GetSafeNormal();
}
