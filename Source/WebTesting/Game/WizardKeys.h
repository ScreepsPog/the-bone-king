#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"

/** One control and the one key that works it. */
struct FWizardKeyBinding
{
	FName Action;
	FKey Key;
};

/** What came of asking for a control to be put on a key. */
enum class EWizardRebind : uint8
{
	// The control is on the key now.
	Set,
	// Another control had the key: the two changed places.
	Swapped,
	// It was on that key already.
	Same,
	// Not done: the control or the key cannot change (the other control that keeps the key is named, when one does).
	Refused,
};

/**
 * Every control and its one key (director, 2026-11-13: a control has one key and a key works one control), with the
 * player's own choices over the defaults (director, 2026-11-14: "make controls changeable and storable").
 * Plain data, so it is tested without a world. The game's one map is WizardKeys().
 */
struct FWizardKeyMap
{
	/** Every control with the key it starts on. */
	static const TArray<FWizardKeyBinding>& Defaults()
	{
		// Escape is not used: a browser takes it to leave full screen, so the game would never see it there.
		static const TArray<FWizardKeyBinding> Bindings = {
			{ TEXT("Move Right"), EKeys::D },
			{ TEXT("Move Left"), EKeys::A },
			{ TEXT("Move Forward"), EKeys::W },
			{ TEXT("Move Back"), EKeys::S },
			{ TEXT("Bolt"), EKeys::LeftMouseButton },
			{ TEXT("Spear"), EKeys::RightMouseButton },
			{ TEXT("Summon"), EKeys::Q },
			{ TEXT("Blink"), EKeys::SpaceBar },
			{ TEXT("Shrine"), EKeys::E },
			{ TEXT("Command"), EKeys::F },
			{ TEXT("Command Count"), EKeys::MouseWheelAxis },
			{ TEXT("Recall"), EKeys::C },
			{ TEXT("Pause"), EKeys::P },
			{ TEXT("Restart"), EKeys::R },
			{ TEXT("Menu Start"), EKeys::Enter },
			{ TEXT("Camera Angled"), EKeys::One },
			{ TEXT("Camera Top Down"), EKeys::Two },
			{ TEXT("Camera Third Person"), EKeys::Three },
			{ TEXT("Debug Overlay"), EKeys::H },
			{ TEXT("Test Models"), EKeys::J },
			{ TEXT("Test Enemies"), EKeys::K },
			{ TEXT("Test Army"), EKeys::L },
			{ TEXT("Test Switch"), EKeys::M },
		};
		return Bindings;
	}

	/** The controls the player may move to another key, in the order the settings screen lists them. */
	static const TArray<FName>& Changeable()
	{
		static const TArray<FName> Names = {
			TEXT("Move Forward"), TEXT("Move Left"), TEXT("Move Back"), TEXT("Move Right"), TEXT("Bolt"), TEXT("Shrine"), TEXT("Pause"),
			TEXT("Summon"), TEXT("Spear"), TEXT("Blink"), TEXT("Command"), TEXT("Recall"),
		};
		return Names;
	}

	/** A control's name as the player reads it. */
	static FString ActionLabel(FName Action)
	{
		static const TMap<FName, FString> Labels = {
			{ TEXT("Move Forward"), TEXT("Move up") }, { TEXT("Move Left"), TEXT("Move left") }, { TEXT("Move Back"), TEXT("Move down") },
			{ TEXT("Move Right"), TEXT("Move right") }, { TEXT("Bolt"), TEXT("Bolt") }, { TEXT("Shrine"), TEXT("Use the shrine") },
			{ TEXT("Pause"), TEXT("Pause") }, { TEXT("Summon"), TEXT("Summon the Afterlife") }, { TEXT("Spear"), TEXT("Spear of Sacrifice") },
			{ TEXT("Blink"), TEXT("Blink") }, { TEXT("Command"), TEXT("Command the army") }, { TEXT("Recall"), TEXT("Recall the army") },
			{ TEXT("Restart"), TEXT("Restart the run") }, { TEXT("Menu Start"), TEXT("March") }, { TEXT("Command Count"), TEXT("the wheel in Command") },
		};
		const FString* Found = Labels.Find(Action);
		return Found ? *Found : FString::Printf(TEXT("a testing key (%s)"), *Action.ToString());
	}

	/** A key a control may be put on: a keyboard key or a mouse button. Not Escape (the browser's), the wheel, or a gamepad. */
	static bool IsAllowed(const FKey& Key)
	{
		if (!Key.IsValid() || Key == EKeys::Escape || Key == EKeys::AnyKey || Key == EKeys::MouseScrollUp || Key == EKeys::MouseScrollDown)
		{
			return false;
		}
		return Key.IsDigital() && !Key.IsGamepadKey() && !Key.IsTouch() && !Key.IsGesture() && !Key.IsDeprecated();
	}

	/** A key's name as the player reads it: "LMB", "Space", "Mouse 4", a letter. */
	static FString KeyLabel(const FKey& Key)
	{
		static const TMap<FKey, FString> Labels = {
			{ EKeys::LeftMouseButton, TEXT("LMB") }, { EKeys::RightMouseButton, TEXT("RMB") }, { EKeys::MiddleMouseButton, TEXT("MMB") },
			{ EKeys::ThumbMouseButton, TEXT("Mouse 4") }, { EKeys::ThumbMouseButton2, TEXT("Mouse 5") }, { EKeys::SpaceBar, TEXT("Space") },
			{ EKeys::LeftShift, TEXT("Shift") }, { EKeys::RightShift, TEXT("R Shift") }, { EKeys::LeftControl, TEXT("Ctrl") },
			{ EKeys::RightControl, TEXT("R Ctrl") }, { EKeys::LeftAlt, TEXT("Alt") }, { EKeys::RightAlt, TEXT("R Alt") }, { EKeys::Tab, TEXT("Tab") },
			{ EKeys::CapsLock, TEXT("Caps") }, { EKeys::Enter, TEXT("Enter") }, { EKeys::BackSpace, TEXT("Back") }, { EKeys::Up, TEXT("Up") },
			{ EKeys::Down, TEXT("Down") }, { EKeys::Left, TEXT("Left") }, { EKeys::Right, TEXT("Right") }, { EKeys::MouseWheelAxis, TEXT("Wheel") },
			{ EKeys::Zero, TEXT("0") }, { EKeys::One, TEXT("1") }, { EKeys::Two, TEXT("2") }, { EKeys::Three, TEXT("3") }, { EKeys::Four, TEXT("4") },
			{ EKeys::Five, TEXT("5") }, { EKeys::Six, TEXT("6") }, { EKeys::Seven, TEXT("7") }, { EKeys::Eight, TEXT("8") }, { EKeys::Nine, TEXT("9") },
			{ EKeys::NumPadZero, TEXT("Num 0") }, { EKeys::NumPadOne, TEXT("Num 1") }, { EKeys::NumPadTwo, TEXT("Num 2") }, { EKeys::NumPadThree, TEXT("Num 3") },
			{ EKeys::NumPadFour, TEXT("Num 4") }, { EKeys::NumPadFive, TEXT("Num 5") }, { EKeys::NumPadSix, TEXT("Num 6") }, { EKeys::NumPadSeven, TEXT("Num 7") },
			{ EKeys::NumPadEight, TEXT("Num 8") }, { EKeys::NumPadNine, TEXT("Num 9") },
		};
		if (const FString* Found = Labels.Find(Key))
		{
			return *Found;
		}
		return Key.IsValid() ? Key.GetFName().ToString() : FString(TEXT("-"));
	}

	static bool CanChange(FName Action) { return Changeable().Contains(Action); }

	/** The key a control is on now. An invalid key for an unknown control. */
	FKey Get(FName Action) const
	{
		if (const FKey* Chosen = Chosen_.Find(Action))
		{
			return *Chosen;
		}
		for (const FWizardKeyBinding& Binding : Defaults())
		{
			if (Binding.Action == Action)
			{
				return Binding.Key;
			}
		}
		return FKey();
	}

	FString GetLabel(FName Action) const { return KeyLabel(Get(Action)); }

	/** Every control with the key it is on now. */
	TArray<FWizardKeyBinding> All() const
	{
		TArray<FWizardKeyBinding> Made;
		for (const FWizardKeyBinding& Binding : Defaults())
		{
			Made.Add({ Binding.Action, Get(Binding.Action) });
		}
		return Made;
	}

	/** The control on a key, or NAME_None. */
	FName ActionOn(const FKey& Key) const
	{
		for (const FWizardKeyBinding& Binding : Defaults())
		{
			if (Get(Binding.Action) == Key)
			{
				return Binding.Action;
			}
		}
		return NAME_None;
	}

	/**
	 * Puts a control on a key. If another changeable control has the key, the two change places. A key kept by a
	 * control that cannot change (R restarts, the testing keys) is refused. OutOther is the other control, when one is involved.
	 */
	EWizardRebind Rebind(FName Action, const FKey& Key, FName* OutOther = nullptr)
	{
		if (OutOther)
		{
			*OutOther = NAME_None;
		}
		if (!CanChange(Action) || !IsAllowed(Key))
		{
			return EWizardRebind::Refused;
		}
		const FKey Old = Get(Action);
		if (Old == Key)
		{
			return EWizardRebind::Same;
		}
		const FName Other = ActionOn(Key);
		if (OutOther)
		{
			*OutOther = Other;
		}
		if (!Other.IsNone() && !CanChange(Other))
		{
			return EWizardRebind::Refused;
		}
		Chosen_.Add(Action, Key);
		if (!Other.IsNone())
		{
			Chosen_.Add(Other, Old);
			return EWizardRebind::Swapped;
		}
		return EWizardRebind::Set;
	}

	void Reset() { Chosen_.Reset(); }

	bool IsDefault() const
	{
		for (const FWizardKeyBinding& Binding : Defaults())
		{
			if (Get(Binding.Action) != Binding.Key)
			{
				return false;
			}
		}
		return true;
	}

	/** The player's choices as text ("Blink=LeftShift"), for the save; only controls that are off their default. */
	TArray<FString> ToStrings() const
	{
		TArray<FString> Made;
		for (const FName& Action : Changeable())
		{
			const FKey Key = Get(Action);
			for (const FWizardKeyBinding& Binding : Defaults())
			{
				if (Binding.Action == Action && Binding.Key != Key)
				{
					Made.Add(Action.ToString() + TEXT("=") + Key.GetFName().ToString());
				}
			}
		}
		return Made;
	}

	/** Reads what ToStrings wrote. Anything that would break the rules (an unknown control, a key not allowed, two controls on a key) puts every key back to its default. */
	void FromStrings(const TArray<FString>& Lines)
	{
		Chosen_.Reset();
		for (const FString& Line : Lines)
		{
			FString Action;
			FString KeyName;
			if (!Line.Split(TEXT("="), &Action, &KeyName))
			{
				continue;
			}
			const FKey Key(*KeyName);
			if (CanChange(*Action) && IsAllowed(Key))
			{
				Chosen_.Add(*Action, Key);
			}
		}
		TSet<FKey> Seen;
		for (const FWizardKeyBinding& Binding : Defaults())
		{
			const FKey Key = Get(Binding.Action);
			if (Seen.Contains(Key))
			{
				Chosen_.Reset();
				return;
			}
			Seen.Add(Key);
		}
	}

private:
	TMap<FName, FKey> Chosen_;
};

/** The game's own key map: what the input is built from and what the HUD shows. */
FWizardKeyMap& WizardKeys();
