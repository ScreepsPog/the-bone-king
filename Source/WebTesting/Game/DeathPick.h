#pragma once

#include "CoreMinimal.h"

/** Which way the blow that killed a unit came from, as the unit stood: the death it is shown to die. */
enum class EDeathFall : uint8
{
	FromFront,
	FromBehind,
	FromSide,
	Count,
};

/**
 * The director's three deaths (2026-11-14 22:30 PST: "Have them be used based on where the final blow came from").
 * Measured on the animations: Narbash pitches forward where he stands, so he is the blow from behind; Kallari
 * staggers back first and then drops, so she is the blow from the front; Grux is thrown off his feet to one side,
 * so he is the blow from the side.
 */
struct FDeathPick
{
	/** Facing: the way the unit faced. Position: where it stood. From: where the last blow came from. */
	static EDeathFall Pick(FVector2D Facing, FVector2D Position, FVector2D From)
	{
		const FVector2D ToBlow = (From - Position).GetSafeNormal();
		const FVector2D Ahead = Facing.GetSafeNormal();
		if (ToBlow.IsNearlyZero() || Ahead.IsNearlyZero())
		{
			return EDeathFall::FromFront;
		}
		// Within 60 degrees of straight ahead is the front, within 60 of straight behind is the back.
		const double Along = FVector2D::DotProduct(Ahead, ToBlow);
		return Along >= 0.5 ? EDeathFall::FromFront : Along <= -0.5 ? EDeathFall::FromBehind : EDeathFall::FromSide;
	}

	static const TCHAR* AnimationPath(EDeathFall Fall)
	{
		switch (Fall)
		{
		case EDeathFall::FromBehind: return TEXT("/Game/Paragon_Picks/01_Deaths_Troops/A_Narbash_Death_Fwd.A_Narbash_Death_Fwd");
		case EDeathFall::FromSide: return TEXT("/Game/Paragon_Picks/01_Deaths_Troops/A_Grux_Death_B.A_Grux_Death_B");
		default: return TEXT("/Game/Paragon_Picks/01_Deaths_Troops/A_Kallari_Death_B.A_Kallari_Death_B");
		}
	}

	/**
	 * Seconds into the animation that the body meets the ground: measured (the hips and head reach their lowest).
	 * A notify named "Fall" on the animation is used in place of this.
	 */
	static float FallSeconds(EDeathFall Fall)
	{
		switch (Fall)
		{
		case EDeathFall::FromBehind: return 0.92f;
		case EDeathFall::FromSide: return 0.7f;
		default: return 1.1f;
		}
	}

	static const TCHAR* Name(EDeathFall Fall)
	{
		return Fall == EDeathFall::FromBehind ? TEXT("behind") : Fall == EDeathFall::FromSide ? TEXT("side") : TEXT("front");
	}
};
