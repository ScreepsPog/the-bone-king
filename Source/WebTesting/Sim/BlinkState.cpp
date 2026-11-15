#include "Sim/BlinkState.h"

namespace
{
	// The dash stops this far before the line where the wizard would touch the wall.
	const double WallGap = 3.0;

	/** How much of a move from Start is free of walls, 0 to 1. */
	double FreeShare(FVector2D Start, FVector2D Move, TConstArrayView<FCrowdObstacle> Obstacles, float Radius)
	{
		double Share = 1.0;
		for (const FCrowdObstacle& Obstacle : Obstacles)
		{
			double Alpha = 0.0;
			if (Obstacle.SegmentEnters(Start, Start + Move, Radius, Alpha))
			{
				Share = FMath::Min(Share, Alpha);
			}
		}
		return Share;
	}

	/** Moves as far along Move as the walls allow. Returns the part of the move that was not made. */
	FVector2D Advance(FVector2D& Position, FVector2D Move, TConstArrayView<FCrowdObstacle> Obstacles, float Radius)
	{
		const double Length = Move.Size();
		if (Length < UE_DOUBLE_KINDA_SMALL_NUMBER)
		{
			return FVector2D::ZeroVector;
		}
		const double Share = FreeShare(Position, Move, Obstacles, Radius);
		if (Share >= 1.0)
		{
			Position += Move;
			return FVector2D::ZeroVector;
		}
		const double Made = FMath::Max(Share - WallGap / Length, 0.0);
		Position += Move * Made;
		return Move * (1.0 - Made);
	}
}

bool FBlinkState::IsHidden(float HiddenShare) const
{
	return bActive && TimeShare < HiddenShare;
}

bool FBlinkState::Start(FVector2D InFrom, FVector2D InDirection, float InRange)
{
	const FVector2D Normal = InDirection.GetSafeNormal();
	if (!CanStart() || Normal.IsNearlyZero())
	{
		return false;
	}
	Position = InFrom;
	Direction = Normal;
	Range = FMath::Max(InRange, 0.0f);
	Elapsed = 0.0f;
	TimeShare = 0.0f;
	bActive = true;
	return true;
}

bool FBlinkState::Step(float DeltaSeconds, float BlinkSeconds, float CooldownSeconds, FVector2D Steer, float SteerShare,
	TConstArrayView<FCrowdObstacle> Obstacles, float Radius)
{
	const float Seconds = FMath::Max(DeltaSeconds, 0.0f);
	if (!bActive)
	{
		Cooldown = FMath::Max(Cooldown - Seconds, 0.0f);
		return false;
	}

	// A burst: fastest at the start, slowing to a stop. The distance covered follows an ease-out curve over the time.
	const auto Share = [BlinkSeconds](float Time)
	{
		return BlinkSeconds > 0.0f ? FMath::Clamp(Time / BlinkSeconds, 0.0f, 1.0f) : 1.0f;
	};
	const auto Covered = [](float TimeShare)
	{
		return 1.0f - FMath::Square(1.0f - TimeShare);
	};
	const float Before = Share(Elapsed);
	Elapsed += Seconds;
	const float After = Share(Elapsed);
	TimeShare = After;
	const double Along = Range * (Covered(After) - Covered(Before));

	// Only the part of the held direction that is across the blink bends it: holding back does not shorten it.
	// SteerEarly of the sideways move follows the burst, so most of it comes at the start; the rest goes by time, so
	// the end of the blink can still be steered.
	const FVector2D Held = Steer.SizeSquared() > 1.0 ? Steer.GetSafeNormal() : Steer;
	const FVector2D Side = Held - Direction * FVector2D::DotProduct(Held, Direction);
	const float Early = FMath::Clamp(SteerEarly, 0.0f, 1.0f);
	const float SideShare = Early * (Covered(After) - Covered(Before)) + (1.0f - Early) * (After - Before);
	const FVector2D Move = Direction * Along + Side * (Range * FMath::Max(SteerShare, 0.0f) * SideShare);

	// What a wall stops is tried again along each axis by itself, so he slides along the wall.
	const FVector2D Left = Advance(Position, Move, Obstacles, Radius);
	Advance(Position, FVector2D(Left.X, 0.0), Obstacles, Radius);
	Advance(Position, FVector2D(0.0, Left.Y), Obstacles, Radius);

	// A hair of slack, so steps that add up to the full time in floats end it.
	if (Elapsed < BlinkSeconds - 0.0001f)
	{
		return false;
	}
	bActive = false;
	Cooldown = FMath::Max(CooldownSeconds, 0.0f);
	return true;
}
