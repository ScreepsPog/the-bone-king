#pragma once

#include "CoreMinimal.h"

/** The settings screen's tabs, in the order they are drawn. */
enum class EWizardSettingsTab : uint8
{
	Controls,
	Sound,
	Display,
};

/** The Display tab's rows, top to bottom. */
enum class EWizardDisplayRow : uint8
{
	Quality,
	RenderScale,
	HudSize,
	DangerEdges,
	ScreenShake,
	FrameRate,
	FullScreen,
	Count,
};

/** The sound channels, in the order the Sound tab lists them. Each is a sound class in /Game/Audio/Mix. */
enum class EWizardSoundChannel : uint8
{
	Master,
	Music,
	Effects,
	Interface,
	Count,
};

/**
 * What the player has chosen on the settings screen (the UI thread's settings hand-off of 2026-11-14). Plain numbers,
 * so they can be tested without a world. Every value is kept inside its row's range and on its steps.
 */
struct FWizardSettings
{
	// 0 low, 1 medium, 2 high.
	int32 Quality = DefaultQuality;
	// Percent of the window's pixels the game is drawn at.
	int32 RenderScale = 100;
	// Percent, on top of the size the window gives the HUD and the menus.
	int32 HudSize = 100;
	// Percent of the red edges' strength; 0 is off.
	int32 DangerEdges = 100;
	// 0 off, 1 on.
	int32 FrameRate = 0;

	// The four channels' volumes, 0 to 100 in steps of 5, and whether each is muted (0 or 1).
	int32 Volume[4] = { 80, 60, 90, 70 };
	int32 Muted[4] = { 0, 0, 0, 0 };
	// 1: the game falls silent while its window or browser tab is not the one in front.
	int32 BackgroundMute = 1;

	// High is how the game has looked until now: nothing changes for a player who never opens the screen.
	static constexpr int32 DefaultQuality = 2;
	static constexpr int32 Channels = 4;

	static const TCHAR* ChannelName(int32 Channel)
	{
		const TCHAR* Names[] = { TEXT("master"), TEXT("music"), TEXT("effects"), TEXT("interface") };
		return Channel >= 0 && Channel < Channels ? Names[Channel] : TEXT("");
	}

	/** The channel a name stands for in the console, or INDEX_NONE. */
	static int32 ChannelByName(const FString& Name)
	{
		for (int32 Channel = 0; Channel < Channels; ++Channel)
		{
			if (Name.Equals(ChannelName(Channel), ESearchCase::IgnoreCase))
			{
				return Channel;
			}
		}
		return INDEX_NONE;
	}

	static int32 SnapVolume(int32 Value)
	{
		return FMath::Clamp(FMath::RoundToInt32(Value / 5.0f) * 5, 0, 100);
	}

	/** Sets a channel's volume, on its steps. Moving a volume takes its mute off. True when anything changed. */
	bool SetVolume(int32 Channel, int32 Value)
	{
		if (Channel < 0 || Channel >= Channels)
		{
			return false;
		}
		const int32 Snapped = SnapVolume(Value);
		if (Volume[Channel] == Snapped && Muted[Channel] == 0)
		{
			return false;
		}
		Volume[Channel] = Snapped;
		Muted[Channel] = 0;
		return true;
	}

	bool SetMuted(int32 Channel, bool bMuted)
	{
		if (Channel < 0 || Channel >= Channels || (Muted[Channel] != 0) == bMuted)
		{
			return false;
		}
		Muted[Channel] = bMuted ? 1 : 0;
		return true;
	}

	bool SetBackgroundMute(bool bMute)
	{
		if ((BackgroundMute != 0) == bMute)
		{
			return false;
		}
		BackgroundMute = bMute ? 1 : 0;
		return true;
	}

	/** How loud a channel's own sound class is set, 0 to 1: nothing when muted. The master's also scales the other three. */
	float Loudness(int32 Channel) const
	{
		return Channel < 0 || Channel >= Channels || Muted[Channel] != 0 ? 0.0f : Volume[Channel] / 100.0f;
	}

	/** The Sound tab back to its defaults. */
	void ResetSound()
	{
		const FWizardSettings Defaults;
		for (int32 Channel = 0; Channel < Channels; ++Channel)
		{
			Volume[Channel] = Defaults.Volume[Channel];
			Muted[Channel] = 0;
		}
		BackgroundMute = Defaults.BackgroundMute;
	}

	/** The Display tab back to its defaults. */
	void ResetDisplay()
	{
		const FWizardSettings Defaults;
		Quality = Defaults.Quality;
		RenderScale = Defaults.RenderScale;
		HudSize = Defaults.HudSize;
		DangerEdges = Defaults.DangerEdges;
		FrameRate = Defaults.FrameRate;
	}

	/** A row's lowest and highest value and the step between values. A row with no value of its own gives 0, 0, 1. */
	static void Range(EWizardDisplayRow Row, int32& OutMin, int32& OutMax, int32& OutStep)
	{
		OutMin = 0;
		OutMax = 0;
		OutStep = 1;
		switch (Row)
		{
		case EWizardDisplayRow::Quality: OutMax = 2; break;
		case EWizardDisplayRow::RenderScale: OutMin = 50; OutMax = 100; OutStep = 10; break;
		case EWizardDisplayRow::HudSize: OutMin = 80; OutMax = 130; OutStep = 5; break;
		case EWizardDisplayRow::DangerEdges: OutMax = 100; OutStep = 5; break;
		case EWizardDisplayRow::ScreenShake: OutMax = 2; break;
		case EWizardDisplayRow::FrameRate: OutMax = 1; break;
		default: break;
		}
	}

	/** A slider row (a range of percents), as against a row of choices or a button. */
	static bool IsSlider(EWizardDisplayRow Row)
	{
		return Row == EWizardDisplayRow::RenderScale || Row == EWizardDisplayRow::HudSize || Row == EWizardDisplayRow::DangerEdges;
	}

	/** How many choices a row of choices has; 0 for a slider or a button. */
	static int32 Choices(EWizardDisplayRow Row)
	{
		return Row == EWizardDisplayRow::Quality || Row == EWizardDisplayRow::ScreenShake ? 3 : Row == EWizardDisplayRow::FrameRate ? 2 : 0;
	}

	/** False for the rows that are shown but do nothing yet: the game has no camera shake, and full screen is not wired to the browser. */
	static bool IsEnabled(EWizardDisplayRow Row)
	{
		return Row != EWizardDisplayRow::ScreenShake && Row != EWizardDisplayRow::FullScreen && Row != EWizardDisplayRow::Count;
	}

	/** A value brought inside its row's range and onto its nearest step. */
	static int32 Snap(EWizardDisplayRow Row, int32 Value)
	{
		int32 Min = 0;
		int32 Max = 0;
		int32 Step = 1;
		Range(Row, Min, Max, Step);
		const int32 Steps = FMath::RoundToInt32(static_cast<float>(FMath::Clamp(Value, Min, Max) - Min) / Step);
		return FMath::Min(Min + Steps * Step, Max);
	}

	int32 Get(EWizardDisplayRow Row) const
	{
		switch (Row)
		{
		case EWizardDisplayRow::Quality: return Quality;
		case EWizardDisplayRow::RenderScale: return RenderScale;
		case EWizardDisplayRow::HudSize: return HudSize;
		case EWizardDisplayRow::DangerEdges: return DangerEdges;
		case EWizardDisplayRow::FrameRate: return FrameRate;
		default: return 0;
		}
	}

	/** Sets a row's value, snapped. True when it changed. A row that is not enabled never changes. */
	bool Set(EWizardDisplayRow Row, int32 Value)
	{
		if (!IsEnabled(Row))
		{
			return false;
		}
		const int32 Snapped = Snap(Row, Value);
		int32* Target = nullptr;
		switch (Row)
		{
		case EWizardDisplayRow::Quality: Target = &Quality; break;
		case EWizardDisplayRow::RenderScale: Target = &RenderScale; break;
		case EWizardDisplayRow::HudSize: Target = &HudSize; break;
		case EWizardDisplayRow::DangerEdges: Target = &DangerEdges; break;
		case EWizardDisplayRow::FrameRate: Target = &FrameRate; break;
		default: return false;
		}
		if (*Target == Snapped)
		{
			return false;
		}
		*Target = Snapped;
		return true;
	}

	/** Every row back inside its range (a saved file may hold anything). */
	void Clamp()
	{
		Quality = Snap(EWizardDisplayRow::Quality, Quality);
		RenderScale = Snap(EWizardDisplayRow::RenderScale, RenderScale);
		HudSize = Snap(EWizardDisplayRow::HudSize, HudSize);
		DangerEdges = Snap(EWizardDisplayRow::DangerEdges, DangerEdges);
		FrameRate = Snap(EWizardDisplayRow::FrameRate, FrameRate);
		for (int32 Channel = 0; Channel < Channels; ++Channel)
		{
			Volume[Channel] = SnapVolume(Volume[Channel]);
			Muted[Channel] = Muted[Channel] != 0 ? 1 : 0;
		}
		BackgroundMute = BackgroundMute != 0 ? 1 : 0;
	}

	bool operator==(const FWizardSettings& Other) const
	{
		for (int32 Channel = 0; Channel < Channels; ++Channel)
		{
			if (Volume[Channel] != Other.Volume[Channel] || Muted[Channel] != Other.Muted[Channel])
			{
				return false;
			}
		}
		return Quality == Other.Quality && RenderScale == Other.RenderScale && HudSize == Other.HudSize && DangerEdges == Other.DangerEdges && FrameRate == Other.FrameRate
			&& BackgroundMute == Other.BackgroundMute;
	}

	/** The row a name stands for in the console ("quality", "renderscale", "hudsize", "edges", "framerate"), or Count. */
	static EWizardDisplayRow RowByName(const FString& Name)
	{
		const TCHAR* Names[] = { TEXT("quality"), TEXT("renderscale"), TEXT("hudsize"), TEXT("edges"), TEXT("shake"), TEXT("framerate"), TEXT("fullscreen") };
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Names); ++Index)
		{
			if (Name.Equals(Names[Index], ESearchCase::IgnoreCase))
			{
				return static_cast<EWizardDisplayRow>(Index);
			}
		}
		return EWizardDisplayRow::Count;
	}
};

/**
 * The settings screen: which tab is open, and which menu it was opened from and returns to. It opens on Controls from
 * the start menu, and from the pause menu on the tab last used there (Display the first time).
 */
struct FWizardSettingsScreen
{
	void OpenFromStart()
	{
		bOpen = true;
		bFromStart = true;
		Tab = EWizardSettingsTab::Controls;
	}

	void OpenFromPause()
	{
		bOpen = true;
		bFromStart = false;
		Tab = LastPauseTab;
	}

	void SetTab(EWizardSettingsTab NewTab)
	{
		if (!bOpen)
		{
			return;
		}
		Tab = NewTab;
		if (!bFromStart)
		{
			LastPauseTab = NewTab;
		}
	}

	void Close() { bOpen = false; }

	bool IsOpen() const { return bOpen; }
	bool IsFromStart() const { return bFromStart; }
	EWizardSettingsTab GetTab() const { return Tab; }

	const TCHAR* GetName() const
	{
		return !bOpen ? TEXT("closed") : Tab == EWizardSettingsTab::Controls ? TEXT("controls") : Tab == EWizardSettingsTab::Sound ? TEXT("sound") : TEXT("display");
	}

private:
	bool bOpen = false;
	bool bFromStart = false;
	EWizardSettingsTab Tab = EWizardSettingsTab::Controls;
	EWizardSettingsTab LastPauseTab = EWizardSettingsTab::Display;
};
