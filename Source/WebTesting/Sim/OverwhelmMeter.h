#pragma once

#include "CoreMinimal.h"

struct FOverwhelmTuning
{
	float FillPerEnemyPerSecond = 0.08f;
	// It goes down this fast, but only after DrainDelay seconds with no build-up at all: nobody on him and no hit.
	float DrainPerSecond = 0.2f;
	float DrainDelay = 2.0f;
	float FallSeconds = 2.0f;
};

/**
 * How a god falls: enemies touching the wizard fill the meter, a clear moment drains it,
 * and a full meter starts the fall. Plain data, no engine objects.
 */
class FOverwhelmMeter
{
public:
	FOverwhelmTuning Tuning;
	bool bGodMode = false;
	// While set (the blink), enemies on the wizard add nothing and the meter holds where it is.
	bool bInvulnerable = false;

	/**
	 * TouchingOffsets are offsets from the wizard to each enemy on him. FillScale is how much faster they fill it than
	 * usual (the commander being near). Returns true on the one step the meter first fills.
	 */
	bool Step(float DeltaSeconds, const TArray<FVector2D>& TouchingOffsets, float FillScale = 1.0f);

	/** A hit that fills the meter at once (an arrow). Nothing in god mode, during the blink or the fall. True when this fills it and the fall starts. */
	bool Add(float Amount);

	/** 0 to 1. */
	float GetValue() const { return Value; }

	/** Unit vector toward the pressure, or zero when there is none. */
	FVector2D GetPressureDirection() const { return Direction; }

	bool IsFalling() const { return bFalling; }

	/** Seconds since anything last built it up. */
	float GetQuietSeconds() const { return QuietSeconds; }

	/** True while it has something in it and is waiting out Tuning.DrainDelay before it goes down. */
	bool IsHolding() const { return !bFalling && Value > 0.0f && QuietSeconds < Tuning.DrainDelay; }

	/** 0 to 1 over Tuning.FallSeconds. */
	float GetFallAlpha() const;

	bool IsFallFinished() const { return bFalling && GetFallAlpha() >= 1.0f; }

	/** The fall starts now, whatever the meter held (he left the battlefield). Nothing in god mode. True when it starts. */
	bool Fall();

	void Reset();

private:
	float Value = 0.0f;
	float FallTime = 0.0f;
	float QuietSeconds = 0.0f;
	bool bFalling = false;
	FVector2D Direction = FVector2D::ZeroVector;
};
