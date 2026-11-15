#pragma once

#include "CoreMinimal.h"

/**
 * When the wizard's Bolt leaves his hand, and where his casting animation is. Holding the button casts every
 * Interval seconds. A cast starts the animation; the Bolt leaves Windup seconds later, at the animation's "Fire"
 * moment (the first part of the animation is played faster to get there in that time), and the rest of the
 * animation plays out at its own speed. With no animation, or no windup, the Bolt leaves at once, as it used to.
 * Plain state, so it can be tested without a world.
 */
struct FBoltCast
{
	// Seconds until the next cast may start.
	float Cooldown = 0.0f;
	// Seconds until the Bolt of the cast in hand leaves. Below zero: no cast in hand.
	float WindupLeft = -1.0f;
	// Seconds into the animation. Below zero: not playing.
	float AnimTime = -1.0f;
	// How fast the animation plays while winding up.
	float AnimRate = 1.0f;

	/**
	 * One step. bHeld is the button, bMay whether he may cast now (not fallen, no menu). FireTime is the "Fire"
	 * moment in the animation and Length its whole length, in seconds (0: no animation). True when a Bolt leaves.
	 */
	bool Step(float DeltaSeconds, bool bHeld, bool bMay, float Interval, float Windup, float FireTime, float Length)
	{
		Cooldown = FMath::Max(Cooldown - DeltaSeconds, 0.0f);
		if (WindupLeft >= 0.0f)
		{
			WindupLeft -= DeltaSeconds;
			if (WindupLeft > 0.0f)
			{
				AnimTime += DeltaSeconds * AnimRate;
				return false;
			}
			// The Bolt leaves at the animation's Fire moment, exactly. A cast in hand is finished even if the button was let go.
			WindupLeft = -1.0f;
			AnimTime = FireTime;
			AnimRate = 1.0f;
			return bMay;
		}
		if (AnimTime >= 0.0f)
		{
			AnimTime += DeltaSeconds;
			if (AnimTime >= Length)
			{
				AnimTime = -1.0f;
			}
		}
		if (!bHeld || !bMay || Cooldown > 0.0f)
		{
			return false;
		}
		Cooldown = Interval;
		const float Wind = FireTime > 0.0f && Length > FireTime ? FMath::Min(Windup, FireTime) : 0.0f;
		if (Wind <= 0.0f)
		{
			return true;
		}
		WindupLeft = Wind;
		AnimTime = 0.0f;
		AnimRate = FireTime / Wind;
		return false;
	}

	/** How much of the animation shows, 0 to 1: it eases in over the first quarter of the windup and out over the last quarter second. */
	float GetWeight(float FireTime, float Length) const
	{
		if (AnimTime < 0.0f || Length <= 0.0f)
		{
			return 0.0f;
		}
		const float In = FireTime > 0.0f ? FMath::Clamp(AnimTime / (FireTime * 0.25f), 0.0f, 1.0f) : 1.0f;
		const float Out = FMath::Clamp((Length - AnimTime) / 0.25f, 0.0f, 1.0f);
		return FMath::Min(In, Out);
	}

	void Stop()
	{
		WindupLeft = -1.0f;
		AnimTime = -1.0f;
	}
};
