#pragma once

#include "CoreMinimal.h"

/** Which screen of the start menu is up. */
enum class EWizardMenu : uint8
{
	None,
	Start,
	Controls,
};

/**
 * The start menu, shown before the first run: "March" starts the game, "Controls" shows the controls with a way back.
 * Plain state, so it can be tested without a world. The pawn pauses the game while it is open.
 */
struct FWizardMenuState
{
	/** Puts the start menu up. */
	void Show() { Menu = EWizardMenu::Start; }

	/** Starts the run from either screen. False when no menu was up. */
	bool March()
	{
		if (Menu == EWizardMenu::None)
		{
			return false;
		}
		Menu = EWizardMenu::None;
		return true;
	}

	/** From the start screen to the controls, and back. Nothing when no menu is up. */
	void ToggleControls()
	{
		if (Menu == EWizardMenu::Start)
		{
			Menu = EWizardMenu::Controls;
		}
		else if (Menu == EWizardMenu::Controls)
		{
			Menu = EWizardMenu::Start;
		}
	}

	bool IsOpen() const { return Menu != EWizardMenu::None; }
	EWizardMenu Get() const { return Menu; }

	const TCHAR* GetName() const
	{
		return Menu == EWizardMenu::Start ? TEXT("start") : Menu == EWizardMenu::Controls ? TEXT("controls") : TEXT("none");
	}

private:
	EWizardMenu Menu = EWizardMenu::None;
};
