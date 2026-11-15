#pragma once

#include "CoreMinimal.h"

/**
 * One shrine as plain data: where it stands, how close the wizard must be, and whether its menu is open.
 * The fight goes on while the menu is open and the wizard can still walk, but he cannot use Bolt or the spear,
 * and walking out of range closes the menu.
 */
struct FShrineState
{
	FVector2D Position = FVector2D::ZeroVector;

	/** The wizard can open the shrine within this distance, and is put out of it past this distance. */
	float Range = 345.0f;

	bool IsInRange(FVector2D WizardPosition) const;

	bool IsOpen() const { return bOpen; }

	/** Opens the menu. False, and it stays closed, when the wizard is out of range. */
	bool TryOpen(FVector2D WizardPosition);

	void Close() { bOpen = false; }

	/** Call every frame. Closes the menu when the wizard has walked out of range. True when it closed it. */
	bool Update(FVector2D WizardPosition);

	/** How fast time runs: OpenScale (kept between 0.05 and 1) while the menu is open, 1 otherwise. */
	float GetTimeScale(float OpenScale) const;

	/** False while the menu is open: no Bolt and no spear. */
	bool AllowsAbilities() const;

private:
	bool bOpen = false;
};
