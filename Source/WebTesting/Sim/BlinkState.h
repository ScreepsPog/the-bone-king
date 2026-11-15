#pragma once

#include "CoreMinimal.h"
#include "Sim/CrowdTypes.h"

/**
 * Blink: the wizard's dash. Plain data, no engine objects. It goes the way he was moving when it was cast, Range
 * far, as a burst that slows to a stop, and a direction held during it bends it to that side. Walls and water stop
 * it. The cooldown starts when the dash is over.
 */
class FBlinkState
{
public:
	/** Starts a blink from From along Direction. False while one is under way, during the cooldown, or with no direction. */
	bool Start(FVector2D InFrom, FVector2D InDirection, float InRange);

	/**
	 * Advances time and moves him. Steer is the direction held right now (zero for none): the part of it across the
	 * blink's direction moves him sideways, SteerShare of Range over a whole blink, at an even pace. He never enters a wall (Radius
	 * is how far his middle stays from one) and slides along it instead. True on the one step the blink ends.
	 */
	bool Step(float DeltaSeconds, float BlinkSeconds, float CooldownSeconds, FVector2D Steer, float SteerShare,
		TConstArrayView<FCrowdObstacle> Obstacles, float Radius);

	/** How much of the steering follows the burst (strongest at the start) instead of going at an even pace: 0 to 1. */
	float SteerEarly = 0.0f;

	/** True from the start of the dash until HiddenShare of its time has passed: he cannot be seen. */
	bool IsHidden(float HiddenShare) const;

	/** True during the dash: nothing can hurt the wizard. */
	bool IsActive() const { return bActive; }

	bool CanStart() const { return !bActive && Cooldown <= 0.0f; }

	/** Seconds until the next blink can start. 0 during the dash and when ready. */
	float GetCooldown() const { return Cooldown; }

	/** Where the wizard is right now. */
	FVector2D GetPosition() const { return Position; }

	/** The way the blink was cast. */
	FVector2D GetDirection() const { return Direction; }

	void Reset() { *this = FBlinkState(); }

private:
	FVector2D Position = FVector2D::ZeroVector;
	FVector2D Direction = FVector2D(1.0, 0.0);
	float Range = 0.0f;
	float Elapsed = 0.0f;
	// How much of the blink's time has passed, 0 to 1.
	float TimeShare = 0.0f;
	float Cooldown = 0.0f;
	bool bActive = false;
};
