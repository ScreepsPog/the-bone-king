#pragma once

#include "CoreMinimal.h"

/**
 * The kingdom gate as plain data: a box on the ground at the far end of the level. It is shut while the commander
 * lives. Once he is dead it is open, and the game finishes when the wizard walks into it.
 */
struct FKingdomGate
{
	/** Puts the gate in the level. HalfSize is half its depth (X, the way through) and half its width (Y), before the turn. */
	void Place(FVector2D InCenter, FVector2D InHalfSize, float InYaw);

	/** A new run: shut and not entered. The gate stays where it was placed. */
	void Reset();

	/** Opens it whatever the commander is doing: for testers. */
	void Open() { bOpen = bPlaced; }

	bool IsPlaced() const { return bPlaced; }
	bool IsOpen() const { return bOpen; }
	bool IsEntered() const { return bEntered; }

	/** Seconds since the wizard entered. */
	float GetEnteredSeconds() const { return EnteredSeconds; }

	FVector2D GetCenter() const { return Center; }
	FVector2D GetHalfSize() const { return HalfSize; }
	float GetYaw() const { return Yaw; }

	/** True for a point on the gate's ground. */
	bool Contains(FVector2D Point) const;

	/**
	 * Call every frame. The gate opens once the commander is dead. True at the moment the wizard enters the open gate
	 * (he cannot while bCanEnter is false: falling, or gone in a blink), and never again in that run.
	 */
	bool Update(bool bCommanderDead, FVector2D WizardPosition, bool bCanEnter, float Seconds);

private:
	bool bPlaced = false;
	bool bOpen = false;
	bool bEntered = false;
	float EnteredSeconds = 0.0f;
	FVector2D Center = FVector2D::ZeroVector;
	FVector2D HalfSize = FVector2D(150.0, 300.0);
	float Yaw = 0.0f;
};
