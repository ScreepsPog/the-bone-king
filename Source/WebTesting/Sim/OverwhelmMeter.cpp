#include "Sim/OverwhelmMeter.h"

bool FOverwhelmMeter::Step(float DeltaSeconds, const TArray<FVector2D>& TouchingOffsets, float FillScale)
{
	const float Seconds = FMath::Max(DeltaSeconds, 0.0f);

	if (bFalling)
	{
		FallTime = FMath::Min(FallTime + Seconds, Tuning.FallSeconds);
		return false;
	}

	FVector2D Sum = FVector2D::ZeroVector;
	for (const FVector2D& Offset : TouchingOffsets)
	{
		Sum += Offset.GetSafeNormal();
	}
	Direction = Sum.GetSafeNormal();

	if (bGodMode)
	{
		Value = 0.0f;
		return false;
	}

	if (bInvulnerable)
	{
		// The blink: nothing reaches him, and the meter waits where it is.
		return false;
	}

	if (TouchingOffsets.Num() > 0)
	{
		Value += Tuning.FillPerEnemyPerSecond * TouchingOffsets.Num() * Seconds * FMath::Max(FillScale, 0.0f);
		QuietSeconds = 0.0f;
	}
	else
	{
		// It holds for DrainDelay seconds after the last build-up, and only the time past that goes down.
		const float Before = QuietSeconds;
		QuietSeconds += Seconds;
		const float Draining = FMath::Clamp(QuietSeconds - FMath::Max(Before, Tuning.DrainDelay), 0.0f, Seconds);
		Value -= Tuning.DrainPerSecond * Draining;
	}
	Value = FMath::Clamp(Value, 0.0f, 1.0f);

	if (Value >= 1.0f)
	{
		bFalling = true;
		FallTime = 0.0f;
		return true;
	}
	return false;
}

bool FOverwhelmMeter::Add(float Amount)
{
	if (bFalling || bGodMode || bInvulnerable || Amount <= 0.0f)
	{
		return false;
	}
	Value = FMath::Min(Value + Amount, 1.0f);
	// A hit is build-up: the wait before it goes down starts again.
	QuietSeconds = 0.0f;
	if (Value >= 1.0f)
	{
		bFalling = true;
		FallTime = 0.0f;
		return true;
	}
	return false;
}

float FOverwhelmMeter::GetFallAlpha() const
{
	if (!bFalling)
	{
		return 0.0f;
	}
	return Tuning.FallSeconds > 0.0f ? FMath::Clamp(FallTime / Tuning.FallSeconds, 0.0f, 1.0f) : 1.0f;
}

bool FOverwhelmMeter::Fall()
{
	if (bGodMode || bFalling)
	{
		return false;
	}
	Value = 1.0f;
	bFalling = true;
	FallTime = 0.0f;
	return true;
}

void FOverwhelmMeter::Reset()
{
	Value = 0.0f;
	FallTime = 0.0f;
	QuietSeconds = 0.0f;
	bFalling = false;
	Direction = FVector2D::ZeroVector;
}
