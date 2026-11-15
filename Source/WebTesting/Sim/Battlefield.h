#pragma once

#include "CoreMinimal.h"

/**
 * The battlefield's edge: a box on the ground with no wall around it. Outside it the wizard is warned to come back;
 * if he stays out too long, or goes too far, the mission fails. Plain state, so it can be tested without a world.
 */
struct FBattlefield
{
	bool bActive = false;
	FVector2D Center = FVector2D::ZeroVector;
	FVector2D HalfSize = FVector2D::ZeroVector;
	// Seconds he has been outside, this time. 0 inside.
	float OutsideSeconds = 0.0f;

	void Place(FVector2D InCenter, FVector2D InHalfSize)
	{
		bActive = InHalfSize.X > 0.0 && InHalfSize.Y > 0.0;
		Center = InCenter;
		HalfSize = InHalfSize;
		OutsideSeconds = 0.0f;
	}

	void Reset() { OutsideSeconds = 0.0f; }

	/** How far outside the box a point is. 0 inside it, or with no battlefield. */
	double GetOutside(FVector2D Point) const
	{
		if (!bActive)
		{
			return 0.0;
		}
		const FVector2D Past((Point - Center).GetAbs() - HalfSize);
		return FVector2D(FMath::Max(Past.X, 0.0), FMath::Max(Past.Y, 0.0)).Size();
	}

	/** One step. True when the mission fails: he has been outside for WarnSeconds, or is FarDistance outside. */
	bool Update(FVector2D Wizard, float DeltaSeconds, float WarnSeconds, float FarDistance)
	{
		const double Outside = GetOutside(Wizard);
		if (Outside <= 0.0)
		{
			OutsideSeconds = 0.0f;
			return false;
		}
		OutsideSeconds += DeltaSeconds;
		return OutsideSeconds >= WarnSeconds || Outside >= FarDistance;
	}

	bool IsWarning() const { return OutsideSeconds > 0.0f; }

	float GetSecondsLeft(float WarnSeconds) const { return FMath::Max(WarnSeconds - OutsideSeconds, 0.0f); }
};
