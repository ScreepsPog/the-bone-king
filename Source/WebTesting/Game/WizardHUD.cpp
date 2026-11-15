#include "Game/WizardHUD.h"
#include "Game/WizardAudioSubsystem.h"

#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/FontFace.h"
#include "Fonts/CompositeFont.h"
#include "Engine/Texture2D.h"
#include "Fonts/FontMeasure.h"
#include "Fonts/SlateFontInfo.h"
#include "Framework/Application/SlateApplication.h"
#include "Game/CrowdSubsystem.h"
#include "Engine/GameInstance.h"
#include "Game/WizardPawn.h"
#include "Game/WizardSettingsSubsystem.h"
#include "Misc/App.h"
#include "Rendering/SlateRenderer.h"
#include "Sim/CrowdTypes.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FLinearColor EdgeColor(0.3f, 0.0f, 0.0f);
	const int32 EdgeStrips = 10;
	const FLinearColor DebugTextColor(0.75f, 1.0f, 0.75f);
	const FLinearColor RoamRingColor(0.3f, 1.0f, 0.4f, 0.7f);
	const FLinearColor LeashRingColor(1.0f, 0.85f, 0.2f, 0.7f);
	const FLinearColor PickupRingColor(1.0f, 1.0f, 1.0f, 0.5f);
	const FLinearColor ShrineColor(0.25f, 0.95f, 0.8f, 1.0f);
	// The menus of 2026-11-14 (the UI thread's menus round 2). Colors are the design's, sizes its pixels at 1080p.
	FLinearColor HudHex(uint32 Rgb, float Alpha = 1.0f)
	{
		// The design's colors are as seen on screen (sRGB); the canvas wants them linear.
		FLinearColor Color = FLinearColor::FromSRGBColor(FColor((Rgb >> 16) & 0xFF, (Rgb >> 8) & 0xFF, Rgb & 0xFF));
		Color.A = Alpha;
		return Color;
	}
	FLinearColor HudFaded(const FLinearColor& Color, float Alpha)
	{
		return FLinearColor(Color.R, Color.G, Color.B, Alpha);
	}
	const FLinearColor ParchmentColor = HudHex(0xCFC6A2);
	const FLinearColor PanelColor = HudHex(0x0B0D12, 0.93f);
	const FLinearColor RoyalColor = HudHex(0x5B2E91);
	const FLinearColor RoyalHiColor = HudHex(0xA57BE0);
	const FLinearColor CutColor = HudHex(0xFF7360);
	const FLinearColor CutDeepColor = HudHex(0x6E1A14);
	const FLinearColor OffFillColor = HudHex(0x26282E);
	const FLinearColor OffBorderColor = HudHex(0x3A3D45);
	const FLinearColor OffTextColor = HudHex(0x7D8088);
	const FLinearColor DetailGreyColor = HudHex(0xA9ADB5);
	const FLinearColor MenuReadyColor = HudHex(0x9FE3B0);
	const FLinearColor StartShadeColor = HudHex(0x06070A);
	// The final cut: gold is the title's alone; fallen is the ledger's third part.
	const FLinearColor GoldColor = HudHex(0xFFEEB3);
	const FLinearColor FallenColor = HudHex(0x5D6270);
	const float FinalCutWidth = 1240.0f;
	const float FinalCutPanelTop = 380.0f;
	const float FinalCutPanelHeight = 284.0f;
	const float FinalCutButtonsTop = 710.0f;
	const float FinalCutButtonGap = 22.0f;
	const float FinalCutDarkenSeconds = 1.0f;
	const float FinalCutPartSeconds = 0.6f;
	const float FinalCutPartStarts[] = { 0.2f, 0.6f, 0.9f, 1.2f, 1.9f };
	// The gate: orange is its color, the warmest light in the level.
	const FLinearColor EmberColor = HudHex(0xF29A1A);
	const FLinearColor EmberHiColor = HudHex(0xFFC46B);
	const FLinearColor EmberDarkColor = HudHex(0x1A0F04);
	const float GateEdgeInsetX = 70.0f;
	// 200 from the top and bottom: under the line that says where to go, and over the ability bar.
	const float GateEdgeInsetY = 200.0f;
	// A share that rises from 0 at Start to 1 at Start + Length.
	float HudRamp(float Seconds, float Start, float Length)
	{
		return FMath::Clamp((Seconds - Start) / Length, 0.0f, 1.0f);
	}
	float HudEaseOut(float Share)
	{
		return FMath::InterpEaseOut(0.0f, 1.0f, Share, 2.0f);
	}
	// The shrine menu: its rows start this far under the panel's top, one this tall for each thing sold.
	const float MenuSidePadding = 28.0f;
	const float MenuRowsTop = 170.0f;
	const float MenuRowHeight = 96.0f;
	const float MenuResearchButtonWidth = 150.0f;
	const float MenuUnitButtonWidth = 64.0f;
	const float MenuButtonHeight = 48.0f;
	const float MenuButtonGap = 8.0f;
	const float MenuIconSize = 56.0f;
	const float MenuNameHeight = 22.0f;
	const float MenuDetailHeight = 17.0f;
	// The start menu's column, and the buttons that close it.
	const float StartColumnX = 120.0f;
	const float StartButtonsTop = 634.0f;
	const float StartButtonWidth = 400.0f;
	const float StartButtonHeight = 70.0f;
	const float StartButtonGap = 16.0f;
	// The pause menu's buttons, and the controls panel under them (or on the start menu's controls screen).
	// Three buttons since the settings hand-off: a little closer together, and the panel a little lower.
	const float PauseButtonsTop = 240.0f;
	const float PauseButtonWidth = 340.0f;
	const float PauseButtonHeight = 64.0f;
	const float PauseButtonGap = 12.0f;
	const float ControlsPanelWidth = 1020.0f;
	const float ControlsPanelHeight = 386.0f;
	const float ControlsPanelTopPaused = 480.0f;
	// The settings screen (the UI thread's settings hand-off of 2026-11-14), in its pixels at 1080p.
	const float SettingsPanelWidth = 1240.0f;
	const float SettingsPanelHeight = 860.0f;
	const float SettingsTabsTop = 110.0f;
	const float SettingsTabWidth = 180.0f;
	const float SettingsTabHeight = 44.0f;
	const float SettingsTabGap = 56.0f;
	const float SettingsBodyTop = 184.0f;
	const float SettingsBodySide = 56.0f;
	const float SettingsFooterHeight = 102.0f;
	const float SettingsRowPitch = 80.0f;
	const float SettingsLabelWidth = 380.0f;
	const float SettingsChoiceWidth = 130.0f;
	const float SettingsChoiceHeight = 48.0f;
	const float SettingsValueWidth = 76.0f;
	const float SettingsHandleWidth = 22.0f;
	const float SettingsHandleHeight = 34.0f;
	// Room at the right of the "HUD and menu size" row for its sample.
	const float SettingsSampleWidth = 230.0f;
	const float SettingsButtonHeight = 58.0f;
	// The Controls tab: two columns of rows, a key box at the right of each.
	const float SettingsColumnGap = 60.0f;
	const float SettingsControlsPitch = 72.0f;
	const float SettingsKeyWidth = 120.0f;
	const float SettingsMoveKeyWidth = 58.0f;
	const float SettingsKeyHeight = 46.0f;
	const float SettingsMuteSize = 48.0f;
	const float SettingsBackgroundWidth = 150.0f;
	/** The keys as the player has them now, in words, for wherever the HUD names a key. */
	struct FHudKeyLabels
	{
		FString Up, Left, Down, Right, Bolt, Shrine, Pause, Summon, Spear, Blink, Command, Recall, Restart, CommandHint;
		FHudKeyLabels()
		{
			const FWizardKeyMap& Keys = WizardKeys();
			Up = Keys.GetLabel(TEXT("Move Forward"));
			Left = Keys.GetLabel(TEXT("Move Left"));
			Down = Keys.GetLabel(TEXT("Move Back"));
			Right = Keys.GetLabel(TEXT("Move Right"));
			Bolt = Keys.GetLabel(TEXT("Bolt"));
			Shrine = Keys.GetLabel(TEXT("Shrine"));
			Pause = Keys.GetLabel(TEXT("Pause"));
			Summon = Keys.GetLabel(TEXT("Summon"));
			Spear = Keys.GetLabel(TEXT("Spear"));
			Blink = Keys.GetLabel(TEXT("Blink"));
			Command = Keys.GetLabel(TEXT("Command"));
			Recall = Keys.GetLabel(TEXT("Recall"));
			Restart = Keys.GetLabel(TEXT("Restart"));
			// In command mode Spear's key changes the kind and Bolt's sends.
			CommandHint = FString::Printf(TEXT("Wheel: how many \u00B7 %s: kind \u00B7 %s: send"), *Spear, *Bolt);
		}
	};
	const FLinearColor SettingsTrackColor = FLinearColor::FromSRGBColor(FColor(0x2B, 0x2F, 0x37));
	const FLinearColor SettingsWellColor = FLinearColor::FromSRGBColor(FColor(0x0A, 0x0C, 0x10));
	// The window's own share of the HUD's scale, before the player's size setting.
	float HudWindowScale(float Width, float Height)
	{
		return FMath::Max(FMath::Min(Width / 1920.0f, Height / 1080.0f), AWizardHUD::MinLayoutScale);
	}
	float HudSizeFactor = 1.0f;
	const float KeyCapHeight = 28.0f;
	const float KeyCapTextHeight = 18.0f;
	const FLinearColor BoneTextColor(0.95f, 0.92f, 0.78f);
	// The HUD of 2026-11-14 (the UI thread's round 2). Sizes are the design's pixels at 1080p.
	const FLinearColor HudBoneDimColor(0.725f, 0.7f, 0.6f);
	const FLinearColor HudGoldColor(1.0f, 0.82f, 0.25f);
	const FLinearColor HudBloodColor(0.85f, 0.15f, 0.1f);
	const FLinearColor HudCrownLightColor(1.0f, 0.7f, 0.65f);
	const FLinearColor HudSlotBackColor(0.04f, 0.047f, 0.063f, 0.78f);
	const FLinearColor HudKeyTextColor(0.07f, 0.07f, 0.07f);
	const float HudArmyNumberHeight = 56.0f;
	const float HudBarWidth = 520.0f;
	const float HudBarHeight = 22.0f;
	const float HudBarTop = 58.0f;
	const float HudSlotSize = 92.0f;
	const float HudSmallSlotSize = 72.0f;
	const float HudSlotGap = 14.0f;
	const float HudBarBottom = 34.0f;
	const float HudNameHeight = 18.0f;
	// The icon pictures have a margin: this much of the middle is shown.
	const float HudIconCrop = 1.0f / 1.1f;
	const float HudCommanderRadius = 26.0f;
	const float HudCommanderGap = 22.0f;
	const float HudCommanderPulseSeconds = 1.1f;
	// A digit is about this share of the text's height wide (Roboto bold).
	const float HudDigitWidth = 0.58f;
	const FLinearColor DimTextColor(0.7f, 0.7f, 0.7f);
	const FLinearColor ReadyTextColor(0.45f, 1.0f, 0.6f);
	const FLinearColor HealthBarBackColor(0.0f, 0.0f, 0.0f, 0.65f);
	const FLinearColor HealthBarFillColor(0.95f, 0.2f, 0.12f, 1.0f);
	// A bar floats this far above the top of its unit. A brute and a commander get wider bars.
	const float HealthBarGap = 30.0f;
	const float BruteBarWidthScale = 1.7f;
	const float CommanderBarWidthScale = 2.8f;
	// The commander's icon beside the Overwhelm bar, and the gap between them.
	const float CommanderIconSize = 40.0f;
	const float CommanderIconGap = 14.0f;
	const FLinearColor CommandColor(1.0f, 0.82f, 0.25f, 1.0f);
	const FLinearColor CommandHeldColor(0.45f, 0.7f, 1.0f, 0.55f);
	const int32 RingSegments = 48;

	// Text heights in pixels on a 1920x1080 screen.
	const float ArmyTextHeight = 34.0f;
	const float LabelTextHeight = 20.0f;
	const FLinearColor PresenceRingColor(0.75f, 0.45f, 1.0f, 0.7f);
	// Command mode: the number beside the cursor, the kind under it, and how far right of the cursor they start.
	const float CommandCountHeight = 34.0f;
	const float CommandKindHeight = 20.0f;
	const float CommandCursorGap = 30.0f;
	const float DebugTextHeight = 17.0f;
	const float FallTextHeight = 72.0f;
	const float SmallestTextHeight = 11.0f;
}

AWizardHUD::AWizardHUD()
{
	// A hard reference, so the font is always in the package: the HUD needs it from the first frame.
	static ConstructorHelpers::FObjectFinder<UFont> RobotoFont(TEXT("/Engine/EngineFonts/Roboto"));
	TextFont = RobotoFont.Object;
	// Cinzel Bold, for titles and labels. Without it they are drawn in Roboto.
	static ConstructorHelpers::FObjectFinder<UFontFace> CinzelFace(TEXT("/Game/UI/Fonts/FF_Cinzel_Bold"));
	TitleFace = CinzelFace.Object;
	static ConstructorHelpers::FObjectFinder<UTexture2D> SummonFinder(TEXT("/Game/UI/T_Icon_Summon"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> SpearFinder(TEXT("/Game/UI/T_Icon_Spear"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> BlinkFinder(TEXT("/Game/UI/T_Icon_Blink"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> CommandFinder(TEXT("/Game/UI/T_Icon_Command"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> BoneFinder(TEXT("/Game/UI/T_Icon_Bone"));
	SummonIcon = SummonFinder.Object;
	SpearIcon = SpearFinder.Object;
	BlinkIcon = BlinkFinder.Object;
	CommandIcon = CommandFinder.Object;
	BoneIcon = BoneFinder.Object;
	static ConstructorHelpers::FObjectFinder<UTexture2D> ArmorFinder(TEXT("/Game/UI/T_Icon_Armor"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> BladesFinder(TEXT("/Game/UI/T_Icon_Blades"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> FocusFinder(TEXT("/Game/UI/T_Icon_Focus"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> GuardFinder(TEXT("/Game/UI/T_Icon_Guard"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> StartFinder(TEXT("/Game/UI/T_Start_Background"));
	ArmorIcon = ArmorFinder.Object;
	BladesIcon = BladesFinder.Object;
	FocusIcon = FocusFinder.Object;
	GuardIcon = GuardFinder.Object;
	static ConstructorHelpers::FObjectFinder<UTexture2D> MarksmanFinder(TEXT("/Game/UI/T_Icon_Marksman"));
	MarksmanIcon = MarksmanFinder.Object;
	StartBackground = StartFinder.Object;

#if !UE_BUILD_SHIPPING
	bDebugVisible = true;
#endif
}

float AWizardHUD::LayoutScale(float Width, float Height)
{
	return HudWindowScale(Width, Height) * HudSizeFactor;
}

void AWizardHUD::SetSizeFactor(float Factor)
{
	HudSizeFactor = FMath::Clamp(Factor, 0.5f, 2.0f);
}

float AWizardHUD::GetSizeFactor()
{
	return HudSizeFactor;
}

float AWizardHUD::MenuScale(float Width, float Height)
{
	// The settings panel and the pause menu are nearly as tall as the window: a larger size is held to what fits.
	const float Window = HudWindowScale(Width, Height);
	const float Fits = FMath::Max(Window, FMath::Min(Width / (SettingsPanelWidth + 20.0f), Height / (SettingsPanelHeight + 20.0f)));
	return FMath::Min(Window * HudSizeFactor, Fits);
}

FBox2D AWizardHUD::SettingsPanelBox(float Width, float Height)
{
	const float Scale = MenuScale(Width, Height);
	const FVector2D Size(FMath::RoundToFloat(SettingsPanelWidth * Scale), FMath::RoundToFloat(SettingsPanelHeight * Scale));
	const FVector2D Min(FMath::RoundToFloat((Width - Size.X) * 0.5f), FMath::RoundToFloat((Height - Size.Y) * 0.5f));
	return FBox2D(Min, Min + Size);
}

FBox2D AWizardHUD::SettingsTabBox(int32 Tab, float Width, float Height)
{
	const float Scale = MenuScale(Width, Height);
	const FBox2D Panel = SettingsPanelBox(Width, Height);
	const float All = SettingsTabWidth * 3.0f + SettingsTabGap * 2.0f;
	const FVector2D Min(FMath::RoundToFloat(Panel.GetCenter().X + (Tab * (SettingsTabWidth + SettingsTabGap) - All * 0.5f) * Scale), FMath::RoundToFloat(Panel.Min.Y + SettingsTabsTop * Scale));
	return FBox2D(Min, Min + FVector2D(FMath::RoundToFloat(SettingsTabWidth * Scale), FMath::RoundToFloat(SettingsTabHeight * Scale)));
}

FBox2D AWizardHUD::SettingsBackBox(float Width, float Height)
{
	const float Scale = MenuScale(Width, Height);
	const FBox2D Panel = SettingsPanelBox(Width, Height);
	const FVector2D Max(FMath::RoundToFloat(Panel.Max.X - 48.0f * Scale), FMath::RoundToFloat(Panel.Max.Y - 26.0f * Scale));
	return FBox2D(Max - FVector2D(FMath::RoundToFloat(200.0f * Scale), FMath::RoundToFloat(SettingsButtonHeight * Scale)), Max);
}

FBox2D AWizardHUD::SettingsResetBox(float Width, float Height)
{
	const float Scale = MenuScale(Width, Height);
	const FBox2D Back = SettingsBackBox(Width, Height);
	const FVector2D Max(Back.Min.X - FMath::RoundToFloat(16.0f * Scale), Back.Max.Y);
	return FBox2D(FVector2D(Max.X - FMath::RoundToFloat(230.0f * Scale), Back.Min.Y), Max);
}

FBox2D AWizardHUD::SettingsRowBox(int32 Row, float Width, float Height)
{
	const float Scale = MenuScale(Width, Height);
	const FBox2D Panel = SettingsPanelBox(Width, Height);
	const float Top = FMath::RoundToFloat(static_cast<float>(Panel.Min.Y) + (SettingsBodyTop + Row * SettingsRowPitch) * Scale);
	return FBox2D(FVector2D(FMath::RoundToFloat(Panel.Min.X + SettingsBodySide * Scale), Top),
		// Down to where the next row starts: a height rounded by itself can overlap it by a pixel.
		FVector2D(FMath::RoundToFloat(Panel.Max.X - SettingsBodySide * Scale), FMath::RoundToFloat(static_cast<float>(Panel.Min.Y) + (SettingsBodyTop + (Row + 1) * SettingsRowPitch) * Scale)));
}

FBox2D AWizardHUD::SettingsChoiceBox(int32 Row, int32 Index, float Width, float Height)
{
	const float Scale = MenuScale(Width, Height);
	const FBox2D RowBox = SettingsRowBox(Row, Width, Height);
	const FVector2D Min(FMath::RoundToFloat(RowBox.Min.X + (SettingsLabelWidth + 20.0f + Index * SettingsChoiceWidth) * Scale),
		FMath::RoundToFloat(RowBox.Min.Y + (SettingsRowPitch - SettingsChoiceHeight) * 0.5f * Scale));
	return FBox2D(Min, Min + FVector2D(FMath::RoundToFloat(SettingsChoiceWidth * Scale), FMath::RoundToFloat(SettingsChoiceHeight * Scale)));
}

FBox2D AWizardHUD::SettingsSliderBox(int32 Row, float Width, float Height)
{
	const float Scale = MenuScale(Width, Height);
	const FBox2D RowBox = SettingsRowBox(Row, Width, Height);
	// After the track come the value and, on the size row, the sample.
	const float After = SettingsValueWidth + 20.0f + (Row == static_cast<int32>(EWizardDisplayRow::HudSize) ? SettingsSampleWidth : 0.0f);
	const float Left = FMath::RoundToFloat(static_cast<float>(RowBox.Min.X) + (SettingsLabelWidth + 20.0f + SettingsHandleWidth * 0.5f) * Scale);
	const float Right = FMath::RoundToFloat(static_cast<float>(RowBox.Max.X) - (After + SettingsHandleWidth * 0.5f) * Scale);
	const float Top = FMath::RoundToFloat(static_cast<float>(RowBox.Min.Y) + (SettingsRowPitch - SettingsHandleHeight) * 0.5f * Scale);
	return FBox2D(FVector2D(Left, Top), FVector2D(Right, Top + FMath::RoundToFloat(SettingsHandleHeight * Scale)));
}

int32 AWizardHUD::SettingsSliderValue(int32 Row, float X, float Width, float Height)
{
	const FBox2D Track = SettingsSliderBox(Row, Width, Height);
	const EWizardDisplayRow DisplayRow = static_cast<EWizardDisplayRow>(Row);
	int32 Min = 0;
	int32 Max = 0;
	int32 Step = 1;
	FWizardSettings::Range(DisplayRow, Min, Max, Step);
	const double Share = FMath::Clamp((X - Track.Min.X) / FMath::Max(Track.GetSize().X, 1.0), 0.0, 1.0);
	return FWizardSettings::Snap(DisplayRow, FMath::RoundToInt32(Min + Share * (Max - Min)));
}

FBox2D AWizardHUD::SettingsKeyBox(int32 Index, float Width, float Height)
{
	const float Scale = MenuScale(Width, Height);
	const FBox2D Panel = SettingsPanelBox(Width, Height);
	const float ColumnWidth = (SettingsPanelWidth - SettingsBodySide * 2.0f - SettingsColumnGap) * 0.5f;
	// The four move keys share the first row of the left column; "Mouse" (Aim) has the second.
	const bool bMove = Index < 4;
	const int32 Column = Index >= 7 ? 1 : 0;
	const int32 Row = bMove ? 0 : Index < 7 ? Index - 2 : Index - 7;
	const float Wide = bMove ? SettingsMoveKeyWidth : SettingsKeyWidth;
	const float Right = SettingsBodySide + ColumnWidth + Column * (ColumnWidth + SettingsColumnGap) - (bMove ? (3 - Index) * (SettingsMoveKeyWidth + 6.0f) : 0.0f);
	const FVector2D Min(FMath::RoundToFloat(static_cast<float>(Panel.Min.X) + (Right - Wide) * Scale), FMath::RoundToFloat(static_cast<float>(Panel.Min.Y) + (SettingsBodyTop + Row * SettingsControlsPitch) * Scale));
	return FBox2D(Min, Min + FVector2D(FMath::RoundToFloat(Wide * Scale), FMath::RoundToFloat(SettingsKeyHeight * Scale)));
}

FBox2D AWizardHUD::SettingsFixedKeyBox(float Width, float Height)
{
	// Where a single key of the left column's second row would be.
	const FBox2D Bolt = SettingsKeyBox(4, Width, Height);
	const double Up = FMath::RoundToFloat(SettingsControlsPitch * MenuScale(Width, Height));
	return FBox2D(Bolt.Min - FVector2D(0.0, Up), Bolt.Max - FVector2D(0.0, Up));
}

FBox2D AWizardHUD::SettingsMuteBox(int32 Channel, float Width, float Height)
{
	const float Scale = MenuScale(Width, Height);
	const FBox2D Row = SettingsRowBox(Channel, Width, Height);
	const FVector2D Min(FMath::RoundToFloat(static_cast<float>(Row.Min.X) + (SettingsLabelWidth + 20.0f) * Scale), FMath::RoundToFloat(static_cast<float>(Row.Min.Y) + (SettingsRowPitch - SettingsMuteSize) * 0.5f * Scale));
	const double Size = FMath::RoundToFloat(SettingsMuteSize * Scale);
	return FBox2D(Min, Min + FVector2D(Size, Size));
}

FBox2D AWizardHUD::SettingsVolumeBox(int32 Channel, float Width, float Height)
{
	const float Scale = MenuScale(Width, Height);
	const FBox2D Row = SettingsRowBox(Channel, Width, Height);
	const FBox2D Mute = SettingsMuteBox(Channel, Width, Height);
	const float Left = FMath::RoundToFloat(static_cast<float>(Mute.Max.X) + (20.0f + SettingsHandleWidth * 0.5f) * Scale);
	const float Right = FMath::RoundToFloat(static_cast<float>(Row.Max.X) - (SettingsValueWidth + 20.0f + SettingsHandleWidth * 0.5f) * Scale);
	const float Top = FMath::RoundToFloat(static_cast<float>(Row.Min.Y) + (SettingsRowPitch - SettingsHandleHeight) * 0.5f * Scale);
	return FBox2D(FVector2D(Left, Top), FVector2D(Right, Top + FMath::RoundToFloat(SettingsHandleHeight * Scale)));
}

int32 AWizardHUD::SettingsVolumeValue(int32 Channel, float X, float Width, float Height)
{
	const FBox2D Track = SettingsVolumeBox(Channel, Width, Height);
	const double Share = FMath::Clamp((X - Track.Min.X) / FMath::Max(Track.GetSize().X, 1.0), 0.0, 1.0);
	return FWizardSettings::SnapVolume(FMath::RoundToInt32(Share * 100.0));
}

FBox2D AWizardHUD::SettingsBackgroundBox(int32 Index, float Width, float Height)
{
	const float Scale = MenuScale(Width, Height);
	const FBox2D Row = SettingsRowBox(FWizardSettings::Channels, Width, Height);
	const FVector2D Min(FMath::RoundToFloat(static_cast<float>(Row.Min.X) + (SettingsLabelWidth + 20.0f + Index * SettingsBackgroundWidth) * Scale),
		FMath::RoundToFloat(static_cast<float>(Row.Min.Y) + (SettingsRowPitch - SettingsChoiceHeight) * 0.5f * Scale));
	return FBox2D(Min, Min + FVector2D(FMath::RoundToFloat(SettingsBackgroundWidth * Scale), FMath::RoundToFloat(SettingsChoiceHeight * Scale)));
}

FBox2D AWizardHUD::SettingsTestSoundBox(float Width, float Height)
{
	const float Scale = MenuScale(Width, Height);
	const FBox2D Row = SettingsRowBox(FWizardSettings::Channels, Width, Height);
	const FBox2D Choice = SettingsBackgroundBox(0, Width, Height);
	return FBox2D(FVector2D(Row.Max.X - FMath::RoundToFloat(230.0f * Scale), Choice.Min.Y), FVector2D(Row.Max.X, Choice.Max.Y));
}

FWizardSettingsHit AWizardHUD::SettingsHitAt(FVector2D ScreenPoint, EWizardSettingsTab Tab, float Width, float Height)
{
	FWizardSettingsHit Hit;
	for (int32 Index = 0; Index < 3; ++Index)
	{
		if (SettingsTabBox(Index, Width, Height).IsInside(ScreenPoint))
		{
			Hit.Kind = EWizardSettingsHitKind::Tab;
			Hit.Row = Index;
			return Hit;
		}
	}
	if (SettingsBackBox(Width, Height).IsInside(ScreenPoint))
	{
		Hit.Kind = EWizardSettingsHitKind::Back;
		return Hit;
	}
	// Each tab has its own reset.
	if (SettingsResetBox(Width, Height).IsInside(ScreenPoint))
	{
		Hit.Kind = EWizardSettingsHitKind::Reset;
		return Hit;
	}
	const float Reach = SettingsHandleWidth * 0.5f * MenuScale(Width, Height);
	if (Tab == EWizardSettingsTab::Controls)
	{
		for (int32 Index = 0; Index < FWizardKeyMap::Changeable().Num(); ++Index)
		{
			if (SettingsKeyBox(Index, Width, Height).IsInside(ScreenPoint))
			{
				Hit.Kind = EWizardSettingsHitKind::Key;
				Hit.Row = Index;
				return Hit;
			}
		}
		return Hit;
	}
	if (Tab == EWizardSettingsTab::Sound)
	{
		for (int32 Channel = 0; Channel < FWizardSettings::Channels; ++Channel)
		{
			if (SettingsMuteBox(Channel, Width, Height).IsInside(ScreenPoint))
			{
				Hit.Kind = EWizardSettingsHitKind::Mute;
				Hit.Row = Channel;
				return Hit;
			}
			const FBox2D Track = SettingsVolumeBox(Channel, Width, Height);
			if (FBox2D(Track.Min - FVector2D(Reach, 0.0), Track.Max + FVector2D(Reach, 0.0)).IsInside(ScreenPoint))
			{
				Hit.Kind = EWizardSettingsHitKind::Volume;
				Hit.Row = Channel;
				return Hit;
			}
		}
		for (int32 Index = 0; Index < 2; ++Index)
		{
			if (SettingsBackgroundBox(Index, Width, Height).IsInside(ScreenPoint))
			{
				Hit.Kind = EWizardSettingsHitKind::Background;
				Hit.Index = Index;
				return Hit;
			}
		}
		if (SettingsTestSoundBox(Width, Height).IsInside(ScreenPoint))
		{
			Hit.Kind = EWizardSettingsHitKind::TestSound;
		}
		return Hit;
	}
	for (int32 Row = 0; Row < static_cast<int32>(EWizardDisplayRow::Count); ++Row)
	{
		const EWizardDisplayRow DisplayRow = static_cast<EWizardDisplayRow>(Row);
		if (!FWizardSettings::IsEnabled(DisplayRow))
		{
			continue;
		}
		if (FWizardSettings::IsSlider(DisplayRow))
		{
			const FBox2D Track = SettingsSliderBox(Row, Width, Height);
			if (FBox2D(Track.Min - FVector2D(Reach, 0.0), Track.Max + FVector2D(Reach, 0.0)).IsInside(ScreenPoint))
			{
				Hit.Kind = EWizardSettingsHitKind::Slider;
				Hit.Row = Row;
				return Hit;
			}
			continue;
		}
		for (int32 Index = 0; Index < FWizardSettings::Choices(DisplayRow); ++Index)
		{
			if (SettingsChoiceBox(Row, Index, Width, Height).IsInside(ScreenPoint))
			{
				Hit.Kind = EWizardSettingsHitKind::Choice;
				Hit.Row = Row;
				Hit.Index = Index;
				return Hit;
			}
		}
	}
	return Hit;
}

FBox2D AWizardHUD::ShrineMenuBox(float Width, float Height)
{
	const float Scale = LayoutScale(Width, Height);
	const FVector2D Half(FMath::RoundToFloat(ShrineMenuWidth * Scale * 0.5f), FMath::RoundToFloat(ShrineMenuHeight * Scale * 0.5f));
	// At the right of the screen, level with the middle: the wizard, in the middle, stays in view while it is open.
	const float Margin = FMath::RoundToFloat(ShrineMenuMargin * Scale);
	const FVector2D Middle(FMath::Max(Width - Margin - Half.X, Margin + Half.X), FMath::RoundToFloat(Height * 0.5f));
	return FBox2D(Middle - Half, Middle + Half);
}

bool AWizardHUD::IsInShrineMenu(FVector2D ScreenPoint, float Width, float Height)
{
	return ShrineMenuBox(Width, Height).IsInside(ScreenPoint);
}

int32 AWizardHUD::ShrineMenuButtonCount(int32 Row)
{
	// Research is bought once. A unit: one, five, or as many as can be paid for.
	const ECrowdShopItem Item = static_cast<ECrowdShopItem>(Row);
	return Item == ECrowdShopItem::Guard || Item == ECrowdShopItem::Marksman ? 3 : 1;
}

int32 AWizardHUD::ShrineMenuButtonAmount(int32 Row, int32 Button)
{
	if (ShrineMenuButtonCount(Row) == 1 || Button <= 0)
	{
		return 1;
	}
	return Button == 1 ? 5 : MAX_int32;
}

FBox2D AWizardHUD::ShrineMenuButtonBox(int32 Row, int32 Button, float Width, float Height)
{
	const float Scale = LayoutScale(Width, Height);
	const FBox2D Menu = ShrineMenuBox(Width, Height);
	const float ButtonHeight = FMath::RoundToFloat(MenuButtonHeight * Scale);
	const float Right = Menu.Max.X - FMath::RoundToFloat(MenuSidePadding * Scale);
	const float Top = Menu.Min.Y + FMath::RoundToFloat((MenuRowsTop + Row * MenuRowHeight + (MenuRowHeight - MenuButtonHeight) * 0.5f) * Scale);
	if (ShrineMenuButtonCount(Row) == 1)
	{
		const float ButtonWidth = FMath::RoundToFloat(MenuResearchButtonWidth * Scale);
		return FBox2D(FVector2D(Right - ButtonWidth, Top), FVector2D(Right, Top + ButtonHeight));
	}
	const float ButtonWidth = FMath::RoundToFloat(MenuUnitButtonWidth * Scale);
	const float Gap = FMath::RoundToFloat(MenuButtonGap * Scale);
	const float X = Right - ButtonWidth * 3.0f - Gap * 2.0f + Button * (ButtonWidth + Gap);
	return FBox2D(FVector2D(X, Top), FVector2D(X + ButtonWidth, Top + ButtonHeight));
}

FWizardLedger AWizardHUD::FinalCutLedger(int32 Raised, int32 Sacrificed, int32 Army)
{
	FWizardLedger Ledger;
	Ledger.Kept = FMath::Max(Army, 0);
	Ledger.Cut = FMath::Max(Sacrificed, 0);
	// Bought units stand at the gate without having been raised, so the rest can come out below zero: nobody fell.
	Ledger.Fell = FMath::Max(Raised - Ledger.Cut - Ledger.Kept, 0);
	const int32 All = Ledger.Kept + Ledger.Cut + Ledger.Fell;
	if (All > 0)
	{
		Ledger.KeptShare = static_cast<float>(Ledger.Kept) / All;
		Ledger.CutShare = static_cast<float>(Ledger.Cut) / All;
		Ledger.FellShare = static_cast<float>(Ledger.Fell) / All;
	}
	return Ledger;
}

float AWizardHUD::FinalCutPartAlpha(int32 Part, float WonSeconds)
{
	if (Part < 0 || Part >= FinalCutParts)
	{
		return 0.0f;
	}
	return HudEaseOut(HudRamp(WonSeconds, FinalCutDarkenSeconds + FinalCutPartStarts[Part], FinalCutPartSeconds));
}

int32 AWizardHUD::FinalCutCount(int32 Value, float WonSeconds)
{
	// A second of counting, from 1.3 seconds after the screen has darkened.
	return FMath::RoundToInt32(Value * HudEaseOut(HudRamp(WonSeconds, FinalCutDarkenSeconds + 1.3f, 1.0f)));
}

FBox2D AWizardHUD::FinalCutButtonBox(int32 Index, float Width, float Height)
{
	const float Scale = LayoutScale(Width, Height);
	const float ButtonWidth = FMath::RoundToFloat(StartButtonWidth * Scale);
	const float Gap = FMath::RoundToFloat(FinalCutButtonGap * Scale);
	const float Left = FMath::RoundToFloat(Width * 0.5f) - ButtonWidth - FMath::RoundToFloat(Gap * 0.5f) + Index * (ButtonWidth + Gap);
	const float Top = FMath::RoundToFloat(FinalCutButtonsTop * Scale);
	return FBox2D(FVector2D(Left, Top), FVector2D(Left + ButtonWidth, Top + FMath::RoundToFloat(StartButtonHeight * Scale)));
}

int32 AWizardHUD::FinalCutButtonAt(FVector2D ScreenPoint, float Width, float Height)
{
	for (int32 Index = 0; Index < FinalCutButtons; ++Index)
	{
		if (FinalCutButtonBox(Index, Width, Height).IsInside(ScreenPoint))
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

float AWizardHUD::GateBannerAlpha(float Seconds)
{
	// In from 0.3 s, fully shown for three seconds from 0.8 s, out by 4.3 s.
	return FMath::Min(HudRamp(Seconds, 0.3f, 0.5f), 1.0f - HudRamp(Seconds, 3.8f, 0.5f));
}

float AWizardHUD::GateObjectiveAlpha(float Seconds)
{
	// Never together with the banner, which is gone by 4.3 s.
	return HudRamp(Seconds, 4.4f, 0.6f);
}

bool AWizardHUD::GateEdgeArrow(FVector2D Projected, bool bInFront, float Width, float Height, FVector2D& OutPoint, FVector2D& OutDirection)
{
	const float Scale = LayoutScale(Width, Height);
	const FVector2D Center(Width * 0.5, Height * 0.5);
	const FVector2D Half(FMath::Max(Width * 0.5 - GateEdgeInsetX * Scale, 1.0), FMath::Max(Height * 0.5 - GateEdgeInsetY * Scale, 1.0));
	FVector2D Offset = Projected - Center;
	if (!bInFront)
	{
		// Behind the camera a projected point is mirrored through the middle.
		Offset = -Offset;
	}
	else if (FMath::Abs(Offset.X) <= Half.X && FMath::Abs(Offset.Y) <= Half.Y)
	{
		return false;
	}
	OutDirection = Offset.IsNearlyZero() ? FVector2D(0.0, 1.0) : Offset.GetSafeNormal();
	const double Reach = FMath::Min(Half.X / FMath::Max(FMath::Abs(OutDirection.X), 1.e-6), Half.Y / FMath::Max(FMath::Abs(OutDirection.Y), 1.e-6));
	OutPoint = Center + OutDirection * Reach;
	return true;
}

FBox2D AWizardHUD::StartMenuButtonBox(int32 Index, float Width, float Height)
{
	const float Scale = LayoutScale(Width, Height);
	const FVector2D Min(FMath::RoundToFloat(StartColumnX * Scale), FMath::RoundToFloat((StartButtonsTop + Index * (StartButtonHeight + StartButtonGap)) * Scale));
	return FBox2D(Min, Min + FVector2D(FMath::RoundToFloat(StartButtonWidth * Scale), FMath::RoundToFloat(StartButtonHeight * Scale)));
}

int32 AWizardHUD::StartMenuButtonAt(FVector2D ScreenPoint, float Width, float Height)
{
	for (int32 Index = 0; Index < StartMenuButtons; ++Index)
	{
		if (StartMenuButtonBox(Index, Width, Height).IsInside(ScreenPoint))
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

FBox2D AWizardHUD::ControlsPanelBox(float Width, float Height)
{
	const float Scale = MenuScale(Width, Height);
	const float Half = FMath::RoundToFloat(ControlsPanelWidth * Scale * 0.5f);
	const float Middle = FMath::RoundToFloat(Width * 0.5f);
	const float Top = FMath::RoundToFloat(ControlsPanelTopPaused * Scale);
	return FBox2D(FVector2D(Middle - Half, Top), FVector2D(Middle + Half, Top + FMath::RoundToFloat(ControlsPanelHeight * Scale)));
}

FBox2D AWizardHUD::PauseMenuButtonBox(int32 Index, float Width, float Height)
{
	const float Scale = MenuScale(Width, Height);
	const float Half = FMath::RoundToFloat(PauseButtonWidth * Scale * 0.5f);
	const float Middle = FMath::RoundToFloat(Width * 0.5f);
	const float Top = FMath::RoundToFloat((PauseButtonsTop + Index * (PauseButtonHeight + PauseButtonGap)) * Scale);
	return FBox2D(FVector2D(Middle - Half, Top), FVector2D(Middle + Half, Top + FMath::RoundToFloat(PauseButtonHeight * Scale)));
}

int32 AWizardHUD::PauseMenuButtonAt(FVector2D ScreenPoint, float Width, float Height)
{
	for (int32 Index = 0; Index < PauseMenuButtons; ++Index)
	{
		if (PauseMenuButtonBox(Index, Width, Height).IsInside(ScreenPoint))
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

bool AWizardHUD::ShrineMenuButtonAt(FVector2D ScreenPoint, float Width, float Height, int32& OutRow, int32& OutButton)
{
	for (int32 Row = 0; Row < ShrineMenuRows; ++Row)
	{
		for (int32 Button = 0; Button < ShrineMenuButtonCount(Row); ++Button)
		{
			if (ShrineMenuButtonBox(Row, Button, Width, Height).IsInside(ScreenPoint))
			{
				OutRow = Row;
				OutButton = Button;
				return true;
			}
		}
	}
	return false;
}

bool AWizardHUD::WantsMenuPointer(bool bStartMenu, bool bPaused, bool bShrineOpen, bool bRunWon)
{
	// Any menu: the start menu, the pause menu, the shrine's shop, the final cut screen.
	return bStartMenu || bPaused || bShrineOpen || bRunWon;
}

float AWizardHUD::HealthBarAlpha(float SecondsLeft)
{
	return FMath::Clamp(SecondsLeft / HealthBarFadeSeconds, 0.0f, 1.0f);
}

FWizardEdgeWeights AWizardHUD::EdgeWeights(FVector2D WorldDirection, float CameraYawDegrees)
{
	FWizardEdgeWeights Weights;
	const FVector2D Direction = WorldDirection.GetSafeNormal();
	if (Direction.IsNearlyZero())
	{
		return Weights;
	}
	// In camera space, X is up the screen and Y is to the right.
	const FVector2D Local = Direction.GetRotated(-CameraYawDegrees);
	Weights.Top = FMath::Max(static_cast<float>(Local.X), 0.0f);
	Weights.Bottom = FMath::Max(static_cast<float>(-Local.X), 0.0f);
	Weights.Right = FMath::Max(static_cast<float>(Local.Y), 0.0f);
	Weights.Left = FMath::Max(static_cast<float>(-Local.Y), 0.0f);
	return Weights;
}

FWizardEdgeWeights AWizardHUD::SideWeights(const TArray<FVector2D>& TouchingOffsets, float CameraYawDegrees)
{
	FWizardEdgeWeights Sides;
	for (const FVector2D& Offset : TouchingOffsets)
	{
		// In camera space, X is up the screen and Y is to the right.
		const FVector2D Local = Offset.GetSafeNormal().GetRotated(-CameraYawDegrees);
		if (Local.IsNearlyZero())
		{
			continue;
		}
		if (FMath::Abs(Local.X) >= FMath::Abs(Local.Y))
		{
			(Local.X >= 0.0 ? Sides.Top : Sides.Bottom) += 1.0f;
		}
		else
		{
			(Local.Y >= 0.0 ? Sides.Right : Sides.Left) += 1.0f;
		}
	}
	const float Most = FMath::Max(FMath::Max(Sides.Left, Sides.Right), FMath::Max(Sides.Top, Sides.Bottom));
	if (Most > 0.0f)
	{
		Sides.Left /= Most;
		Sides.Right /= Most;
		Sides.Top /= Most;
		Sides.Bottom /= Most;
	}
	return Sides;
}

void AWizardHUD::DrawSpike(FVector2D Apex, float BaseLeft, float BaseRight, float BaseY, const FLinearColor& Color)
{
	const int32 Strips = 8;
	const float StripHeight = (BaseY - static_cast<float>(Apex.Y)) / Strips;
	for (int32 Index = 0; Index < Strips; ++Index)
	{
		const float Along = (Index + 0.5f) / Strips;
		const float Left = FMath::Lerp(static_cast<float>(Apex.X), BaseLeft, Along);
		const float Right = FMath::Lerp(static_cast<float>(Apex.X), BaseRight, Along);
		DrawRect(Color, Left, static_cast<float>(Apex.Y) + Index * StripHeight, Right - Left, StripHeight + 1.0f);
	}
}

void AWizardHUD::DrawCommanderIcon(float CenterX, float CenterY, float Scale)
{
	// A red crown in a ring that pulses: the commander is near, and everything on the wizard fills the meter faster.
	const float Radius = HudCommanderRadius * Scale;
	const float Beat = FMath::Fmod(static_cast<float>(GetWorld()->GetRealTimeSeconds()), HudCommanderPulseSeconds) / HudCommanderPulseSeconds;
	const FVector2D Center(CenterX, CenterY);
	// The pulse: a ring that spreads and fades.
	FCanvasNGonItem Pulse(Center, FVector2D(Radius + (3.0f + 10.0f * Beat) * Scale), 32, FLinearColor(HudBloodColor.R, HudBloodColor.G, HudBloodColor.B, 0.6f * (1.0f - Beat)));
	Canvas->DrawItem(Pulse);
	FCanvasNGonItem Ring(Center, FVector2D(Radius + 3.0f * Scale), 32, HudBloodColor);
	Canvas->DrawItem(Ring);
	FCanvasNGonItem Disc(Center, FVector2D(Radius), 32, FLinearColor(0.16f, 0.0f, 0.0f, 1.0f));
	Canvas->DrawItem(Disc);
	// The crown, on the design's 34 by 34 grid: a band, three points (the middle one tallest), a light rim under it.
	DrawCrown(CenterX - 17.0f * Scale, CenterY - 17.0f * Scale, Scale, HudBloodColor, HudCrownLightColor);
}

void AWizardHUD::DrawCrown(float Left, float Top, float Unit, const FLinearColor& Fill, const FLinearColor& Rim)
{
	DrawRect(Fill, Left + 5.0f * Unit, Top + 18.0f * Unit, 24.0f * Unit, 8.0f * Unit);
	DrawSpike(FVector2D(Left + 5.0f * Unit, Top + 12.0f * Unit), Left + 5.0f * Unit, Left + 11.0f * Unit, Top + 18.0f * Unit, Fill);
	DrawSpike(FVector2D(Left + 17.0f * Unit, Top + 7.0f * Unit), Left + 11.0f * Unit, Left + 23.0f * Unit, Top + 18.0f * Unit, Fill);
	DrawSpike(FVector2D(Left + 29.0f * Unit, Top + 12.0f * Unit), Left + 23.0f * Unit, Left + 29.0f * Unit, Top + 18.0f * Unit, Fill);
	DrawRect(Rim, Left + 5.0f * Unit, Top + 27.0f * Unit, 24.0f * Unit, 3.0f * Unit);
}

void AWizardHUD::DrawFadingLine(float X, float Y, float Length, const FLinearColor& Color)
{
	const int32 Pieces = 16;
	const float Piece = Length / Pieces;
	for (int32 Index = 0; Index < Pieces; ++Index)
	{
		const float From = X + Index * Piece;
		DrawRect(HudFaded(Color, Color.A * (1.0f - (Index + 0.5f) / Pieces)), FMath::Min(From, From + Piece), Y, FMath::Abs(Piece), 1.0f);
	}
}

void AWizardHUD::DrawGateMessage(const UCrowdSubsystem& Crowd, const AWizardPawn* Wizard, float Width, float Height, float Scale)
{
	if (!Crowd.IsAreaTaken() || Crowd.IsFalling() || Crowd.IsRunWon())
	{
		return;
	}
	const float Seconds = Crowd.GetAreaTakenSeconds();
	const float Middle = Width * 0.5f;

	// The banner: a broken crown, what happened, and what it means.
	const float Banner = GateBannerAlpha(Seconds);
	if (Banner > 0.0f)
	{
		const float CrownSize = 64.0f * Scale;
		const float CrownLeft = Middle - CrownSize * 0.5f;
		// Just under the Overwhelm bar. No bars: the HUD stays clear the whole time.
		const float CrownTop = 130.0f * Scale;
		DrawCrown(CrownLeft, CrownTop, CrownSize / 34.0f, HudHex(0x7D120C, Banner), HudHex(0xFFB3A6, 0.85f * Banner));
		// Struck through, corner to corner.
		DrawLine(CrownLeft + 6.0f * Scale, CrownTop + CrownSize - 6.0f * Scale, CrownLeft + CrownSize - 6.0f * Scale, CrownTop + 6.0f * Scale,
			HudFaded(BoneTextColor, Banner), FMath::Max(5.0f * Scale, 2.0f));
		DrawTitleText(TEXT("The commander has fallen"), HudFaded(BoneTextColor, Banner), Middle, 206.0f * Scale, 64.0f * Scale, true);
		const FString Meaning(Crowd.HasGate() ? TEXT("The kingdom gate is open") : TEXT("The Outskirts are yours"));
		const float MeaningY = 302.0f * Scale;
		DrawSharpText(Meaning, HudFaded(EmberHiColor, Banner), Middle, MeaningY, 32.0f * Scale, true, true);
		const float HalfWords = TextWidth(Meaning, 32.0f * Scale, true) * 0.5f + 20.0f * Scale;
		DrawFadingLine(Middle + HalfWords, MeaningY + 20.0f * Scale, 160.0f * Scale, HudFaded(EmberColor, 0.9f * Banner));
		DrawFadingLine(Middle - HalfWords, MeaningY + 20.0f * Scale, -160.0f * Scale, HudFaded(EmberColor, 0.9f * Banner));
	}

	// Where to go, until he walks in.
	const float Objective = GateObjectiveAlpha(Seconds);
	if (Objective <= 0.0f || !Crowd.HasGate() || !Crowd.IsGateOpen() || !Wizard)
	{
		return;
	}
	const FVector2D Gate = Crowd.GetGatePosition();
	const FString Metres = FString::Printf(TEXT("%d m"), FMath::RoundToInt32(FVector2D::Distance(Gate, Wizard->GetPosition2D()) / 100.0));
	const FString Words(TEXT("Enter the kingdom gate"));
	const float SignSize = FMath::RoundToFloat(28.0f * Scale);
	const float Gap = 12.0f * Scale;
	const float WordsWidth = TextWidth(Words, 24.0f * Scale, true);
	const float MetresWidth = TextWidth(Metres, 20.0f * Scale, false);
	const float LineWidth = FMath::RoundToFloat(SignSize + Gap + WordsWidth + Gap + MetresWidth + 40.0f * Scale);
	const float LineHeight = FMath::RoundToFloat(44.0f * Scale);
	const float LineLeft = FMath::RoundToFloat(Middle - LineWidth * 0.5f);
	const float LineTop = FMath::RoundToFloat(104.0f * Scale);
	DrawRect(HudHex(0x0B0D12, 0.72f * Objective), LineLeft, LineTop, LineWidth, LineHeight);
	DrawFrame(FBox2D(FVector2D(LineLeft, LineTop), FVector2D(LineLeft + LineWidth, LineTop + LineHeight)), 1.0f, HudFaded(ParchmentColor, 0.5f * Objective));
	// The gate's sign: an arch with three bars.
	{
		const FLinearColor Sign = HudFaded(EmberColor, Objective);
		const float Unit = SignSize / 28.0f;
		const float SignLeft = LineLeft + 20.0f * Scale;
		const float SignTop = LineTop + 8.0f * Scale;
		const float Thick = FMath::Max(2.6f * Scale, 1.5f);
		const FVector2D ArchMiddle(SignLeft + 14.0f * Unit, SignTop + 13.0f * Unit);
		const float Radius = 10.0f * Unit;
		FVector2D Previous(ArchMiddle.X - Radius, SignTop + 26.0f * Unit);
		for (int32 Step = 0; Step <= 10; ++Step)
		{
			const double Angle = UE_DOUBLE_PI + UE_DOUBLE_PI * Step / 10.0;
			const FVector2D Next = ArchMiddle + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius;
			DrawLine(Previous.X, Previous.Y, Next.X, Next.Y, Sign, Thick);
			Previous = Next;
		}
		DrawLine(Previous.X, Previous.Y, Previous.X, SignTop + 26.0f * Unit, Sign, Thick);
		for (int32 Bar = -1; Bar <= 1; ++Bar)
		{
			const float BarX = static_cast<float>(ArchMiddle.X) + Bar * 5.0f * Unit;
			DrawLine(BarX, SignTop + (Bar == 0 ? 4.0f : 6.0f) * Unit, BarX, SignTop + 26.0f * Unit, Sign, FMath::Max(1.8f * Scale, 1.0f));
		}
	}
	float TextX = LineLeft + 20.0f * Scale + SignSize + Gap;
	DrawSharpText(Words, HudFaded(BoneTextColor, Objective), TextX, LineTop + 7.0f * Scale, 24.0f * Scale, true);
	TextX += WordsWidth + Gap;
	DrawSharpText(Metres, HudFaded(HudBoneDimColor, Objective), TextX, LineTop + 10.0f * Scale, 20.0f * Scale, false);

	// The pointer: an arrow at the screen's edge while the gate is out of view, a mark over it once it is in view.
	const FVector Projected = Canvas->Project(FVector(Gate.X, Gate.Y, Wizard->GetGroundZ() + 350.0));
	const FLinearColor Ember = HudFaded(EmberColor, Objective);
	const FLinearColor Outline = HudFaded(EmberDarkColor, Objective);
	FVector2D Point;
	FVector2D Direction;
	if (GateEdgeArrow(FVector2D(Projected.X, Projected.Y), Projected.Z > 0.0, Width, Height, Point, Direction))
	{
		const FVector2D Side(-Direction.Y, Direction.X);
		const FVector2D Tip = Point + Direction * (22.0 * Scale);
		const FVector2D WingA = Tip - Direction * (30.0 * Scale) + Side * (22.0 * Scale);
		const FVector2D WingB = Tip - Direction * (30.0 * Scale) - Side * (22.0 * Scale);
		DrawLine(WingA.X, WingA.Y, Tip.X, Tip.Y, Outline, 12.0f * Scale);
		DrawLine(WingB.X, WingB.Y, Tip.X, Tip.Y, Outline, 12.0f * Scale);
		DrawLine(WingA.X, WingA.Y, Tip.X, Tip.Y, Ember, 7.0f * Scale);
		DrawLine(WingB.X, WingB.Y, Tip.X, Tip.Y, Ember, 7.0f * Scale);
		const FVector2D Label = Point - Direction * (58.0 * Scale);
		DrawSharpText(TEXT("Gate"), HudFaded(EmberHiColor, Objective), Label.X, Label.Y - 12.0f * Scale, 20.0f * Scale, true, true);
	}
	else
	{
		// It bobs down and back every 1.2 seconds.
		const float Bob = 6.0f * Scale * 0.5f * (1.0f - FMath::Cos(UE_TWO_PI * FMath::Fmod(static_cast<float>(GetWorld()->GetRealTimeSeconds()), 1.2f) / 1.2f));
		const float TipX = static_cast<float>(Projected.X);
		const float TipY = static_cast<float>(Projected.Y) + Bob;
		const float Tall = 22.0f * Scale;
		const float HalfWide = 15.0f * Scale;
		const int32 Strips = 8;
		for (int32 Pass = 0; Pass < 2; ++Pass)
		{
			// The dark outline first, a little larger, then the mark itself.
			const float Grow = Pass == 0 ? 2.0f * Scale : 0.0f;
			for (int32 Strip = 0; Strip < Strips; ++Strip)
			{
				const float Along = (Strip + 0.5f) / Strips;
				const float Half = (HalfWide + Grow) * (1.0f - Along);
				DrawRect(Pass == 0 ? Outline : Ember, TipX - Half, TipY - Tall - Grow + Strip * (Tall + Grow * 2.0f) / Strips, Half * 2.0f, (Tall + Grow * 2.0f) / Strips + 1.0f);
			}
		}
		DrawSharpText(FString::Printf(TEXT("Gate \u00B7 %s"), *Metres), HudFaded(EmberHiColor, Objective), TipX, TipY - Tall - 30.0f * Scale, 20.0f * Scale, true, true);
	}
}

void AWizardHUD::DrawAbilityBar(const UCrowdSubsystem& Crowd, float Width, float Height, float Scale)
{
	// The abilities, as square slots along the bottom. A slot that cannot be used at all is not shown, so the bar
	// shrinks and stays in the middle.
	const FHudKeyLabels KeyLabels;
	struct FSlot
	{
		UTexture2D* Icon = nullptr;
		const TCHAR* Key = TEXT("");
		const TCHAR* Name = TEXT("");
		bool bSmall = false;
		bool bReady = true;
		// Seconds left, and the whole of it, for the dark wipe.
		float Cooldown = 0.0f;
		float CooldownMost = 1.0f;
		FString Badge;
		bool bRecall = false;
	};
	TArray<FSlot, TInlineAllocator<5>> Slots;
	const FCrowdTuning& Tuning = Crowd.GetTuning();
	const int32 Sent = Crowd.GetCommandedCount();
	const int32 SummonCount = Crowd.GetSummonCount();
	if (SummonCount > 0)
	{
		FSlot Slot;
		Slot.Icon = SummonIcon;
		Slot.Key = *KeyLabels.Summon;
		Slot.Name = TEXT("Summon");
		Slot.Badge = FString::Printf(TEXT("+%d"), SummonCount);
		Slots.Add(Slot);
	}
	if (Crowd.IsSpearUnlocked())
	{
		FSlot Slot;
		Slot.Icon = SpearIcon;
		Slot.Key = *KeyLabels.Spear;
		Slot.Name = TEXT("Spear");
		Slot.Cooldown = Crowd.GetSpearCooldown();
		Slot.CooldownMost = FMath::Max(Tuning.SpearCooldown, 0.01f);
		// Dim when it cannot be paid for: no bones for it and no skeleton to give.
		Slot.bReady = Crowd.GetSim().CanThrowSpear();
		Slots.Add(Slot);
	}
	{
		FSlot Slot;
		Slot.Icon = BlinkIcon;
		Slot.Key = *KeyLabels.Blink;
		Slot.Name = TEXT("Blink");
		Slot.Cooldown = Crowd.IsBlinking() ? 0.0f : Crowd.GetBlinkCooldown();
		Slot.CooldownMost = FMath::Max(Tuning.BlinkCooldown, 0.01f);
		Slot.bReady = !Crowd.IsBlinking() && Crowd.GetBlinkCooldown() <= 0.0f;
		Slots.Add(Slot);
	}
	const int32 BigSlots = Slots.Num();
	if (!Crowd.IsCommandMode() && Crowd.GetArmyCount() - Sent > 0)
	{
		FSlot Slot;
		Slot.Icon = CommandIcon;
		Slot.Key = *KeyLabels.Command;
		Slot.Name = TEXT("Command");
		Slot.bSmall = true;
		Slots.Add(Slot);
	}
	if (Sent > 0)
	{
		FSlot Slot;
		Slot.Key = *KeyLabels.Recall;
		Slot.Name = TEXT("Recall");
		Slot.bSmall = true;
		Slot.bRecall = true;
		Slots.Add(Slot);
	}
	const bool bDivider = Slots.Num() > BigSlots && BigSlots > 0;

	const float Gap = FMath::RoundToFloat(HudSlotGap * Scale);
	const float DividerWidth = FMath::Max(FMath::RoundToFloat(2.0f * Scale), 1.0f);
	float Total = bDivider ? DividerWidth + Gap : 0.0f;
	for (const FSlot& Slot : Slots)
	{
		Total += FMath::RoundToFloat((Slot.bSmall ? HudSmallSlotSize : HudSlotSize) * Scale) + Gap;
	}
	Total -= Gap;
	// The faces stand on one line; the names are under it.
	const float FaceBottom = Height - FMath::RoundToFloat((HudBarBottom + HudNameHeight + 10.0f) * Scale);
	float X = FMath::RoundToFloat((Width - Total) * 0.5f);
	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		const FSlot& Slot = Slots[Index];
		if (bDivider && Index == BigSlots)
		{
			DrawRect(FLinearColor(BoneTextColor.R, BoneTextColor.G, BoneTextColor.B, 0.25f), X, FaceBottom - HudSmallSlotSize * Scale, DividerWidth, HudSmallSlotSize * Scale);
			X += DividerWidth + Gap;
		}
		const float Size = FMath::RoundToFloat((Slot.bSmall ? HudSmallSlotSize : HudSlotSize) * Scale);
		const float Y = FaceBottom - Size;
		const bool bCooling = Slot.Cooldown > 0.0f;
		const bool bLit = Slot.bReady && !bCooling;
		if (Slot.Icon)
		{
			const float Margin = (1.0f - HudIconCrop) * 0.5f;
			const float Shade = bLit ? 1.0f : 0.55f;
			DrawTexture(Slot.Icon, X, Y, Size, Size, Margin, Margin, HudIconCrop, HudIconCrop, FLinearColor(Shade, Shade, Shade, 1.0f));
		}
		else
		{
			const float Line = FMath::Max(FMath::RoundToFloat(2.0f * Scale), 1.0f);
			DrawRect(FLinearColor(BoneTextColor.R, BoneTextColor.G, BoneTextColor.B, bLit ? 1.0f : 0.5f), X, Y, Size, Size);
			DrawRect(FLinearColor(HudSlotBackColor.R, HudSlotBackColor.G, HudSlotBackColor.B, 1.0f), X + Line, Y + Line, Size - Line * 2.0f, Size - Line * 2.0f);
		}
		if (Slot.bRecall)
		{
			// A turning arrow: three quarters of a circle and its head. A stand-in until it has a picture.
			const FVector2D Middle(X + Size * 0.5f, Y + Size * 0.52f);
			const float Radius = Size * 0.27f;
			const float Thick = FMath::Max(3.0f * Scale, 2.0f);
			const int32 Steps = 12;
			FVector2D Previous = Middle + FVector2D(FMath::Cos(-0.6), FMath::Sin(-0.6)) * Radius;
			for (int32 Step = 1; Step <= Steps; ++Step)
			{
				const double Angle = -0.6 - 4.6 * Step / Steps;
				const FVector2D Next = Middle + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius;
				DrawLine(Previous.X, Previous.Y, Next.X, Next.Y, BoneTextColor, Thick);
				Previous = Next;
			}
			const FVector2D Tip = Middle + FVector2D(FMath::Cos(-0.6), FMath::Sin(-0.6)) * Radius;
			DrawLine(Tip.X, Tip.Y, Tip.X, Tip.Y - Size * 0.2f, BoneTextColor, Thick);
			DrawLine(Tip.X, Tip.Y, Tip.X - Size * 0.2f, Tip.Y, BoneTextColor, Thick);
		}
		if (bCooling)
		{
			// It wipes clear from the top as the time runs out, and counts down in the middle.
			const float Left = FMath::Clamp(Slot.Cooldown / Slot.CooldownMost, 0.0f, 1.0f);
			DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.6f), X, Y + Size * (1.0f - Left), Size, Size * Left);
			DrawSharpText(FString::Printf(TEXT("%.1f"), Slot.Cooldown), FLinearColor::White, X + Size * 0.5f, Y + Size * 0.5f - 20.0f * Scale, 34.0f * Scale, true, true);
		}
		// Its key, on the corner.
		const FString Key(Slot.Key);
		const float KeyHeight = FMath::RoundToFloat(28.0f * Scale);
		const float KeyWidth = FMath::Max(FMath::RoundToFloat(30.0f * Scale), FMath::RoundToFloat((Key.Len() * 11.0f + 12.0f) * Scale));
		DrawRect(BoneTextColor, X - 2.0f * Scale, Y - 2.0f * Scale, KeyWidth, KeyHeight);
		DrawSharpText(Key, HudKeyTextColor, X - 2.0f * Scale + KeyWidth * 0.5f, Y - 2.0f * Scale + 3.0f * Scale, 18.0f * Scale, true, true);
		if (!Slot.Badge.IsEmpty())
		{
			DrawSharpText(Slot.Badge, ReadyTextColor, X + Size - (Slot.Badge.Len() * 20.0f * HudDigitWidth + 6.0f) * Scale, Y + Size - 27.0f * Scale, 20.0f * Scale, true);
		}
		DrawSharpText(Slot.Name, BoneTextColor, X + Size * 0.5f, FaceBottom + 8.0f * Scale, HudNameHeight * Scale, false, true);
		X += Size + Gap;
	}
}

void AWizardHUD::DrawSharpText(const FString& Text, const FLinearColor& Color, float X, float Y, float PixelHeight, bool bBold, bool bCentered, float Spacing, bool bShadow)
{
	// The font is rasterized at the size it is shown, never stretched, so it is sharp at every resolution.
	// Font sizes are in points: 0.75 points to a pixel.
	const float Height = FMath::Max(PixelHeight, SmallestTextHeight);
	const int32 PointSize = FMath::Max(FMath::RoundToInt32(Height * 0.75f), 6);
	if (!TextFont)
	{
		// Without the outline font, fall back to the engine's bitmap font so the HUD is never blank.
		DrawText(Text, Color, X, Y, GEngine->GetMediumFont(), Height / 14.0f);
		return;
	}
	FSlateFontInfo Font(TextFont, PointSize, bBold ? FName("Bold") : FName("Regular"));
	// Letter spacing is in thousandths of the text's height.
	Font.LetterSpacing = FMath::RoundToInt32(Spacing * 1000.0f);
	FCanvasTextItem Item(FVector2D(FMath::RoundToFloat(X), FMath::RoundToFloat(Y)), FText::FromString(Text), Font, Color);
	if (bShadow)
	{
		Item.EnableShadow(FLinearColor(0.0f, 0.0f, 0.0f, 0.75f * Color.A));
	}
	Item.bCentreX = bCentered;
	Canvas->DrawItem(Item);
}

float AWizardHUD::DrawTitleText(const FString& Text, const FLinearColor& Color, float X, float Y, float PixelHeight, bool bCentered, float Spacing)
{
	if (!TitleFace)
	{
		DrawSharpText(Text, Color, X, Y, PixelHeight, true, bCentered);
		return TextWidth(Text, PixelHeight, true);
	}
	if (!TitleFont)
	{
		// A font with one typeface, "Bold", made from the face. A canvas text draws nothing without a font object.
		TitleFont = NewObject<UFont>(this);
		TitleFont->FontCacheType = EFontCacheType::Runtime;
		FTypefaceEntry& Entry = TitleFont->GetMutableInternalCompositeFont().DefaultTypeface.Fonts.AddDefaulted_GetRef();
		Entry.Name = FName("Bold");
		Entry.Font = FFontData(TitleFace);
	}
	const float Height = FMath::Max(PixelHeight, SmallestTextHeight);
	const FSlateFontInfo Font(TitleFont, FMath::Max(FMath::RoundToInt32(Height * 0.75f), 6), FName("Bold"));
	const FLinearColor Shadow(0.0f, 0.0f, 0.0f, 0.75f * Color.A);
	if (Spacing <= 0.0f || !FSlateApplication::IsInitialized() || !FSlateApplication::Get().GetRenderer())
	{
		FCanvasTextItem Item(FVector2D(FMath::RoundToFloat(X), FMath::RoundToFloat(Y)), FText::FromString(Text), Font, Color);
		Item.EnableShadow(Shadow);
		Item.bCentreX = bCentered;
		Canvas->DrawItem(Item);
		return FSlateApplication::IsInitialized() && FSlateApplication::Get().GetRenderer()
			? static_cast<float>(FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Text, Font).X) : Text.Len() * Height * 0.7f;
	}
	// Spaced out: a canvas text has no letter spacing of its own, so the letters are placed one at a time.
	const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const float Gap = Spacing * Height;
	TArray<float, TInlineAllocator<32>> Widths;
	float Total = 0.0f;
	for (int32 Index = 0; Index < Text.Len(); ++Index)
	{
		Widths.Add(static_cast<float>(Measure->Measure(Text.Mid(Index, 1), Font).X));
		Total += Widths.Last() + (Index > 0 ? Gap : 0.0f);
	}
	float At = bCentered ? X - Total * 0.5f : X;
	for (int32 Index = 0; Index < Text.Len(); ++Index)
	{
		FCanvasTextItem Item(FVector2D(FMath::RoundToFloat(At), FMath::RoundToFloat(Y)), FText::FromString(Text.Mid(Index, 1)), Font, Color);
		Item.EnableShadow(Shadow);
		Canvas->DrawItem(Item);
		At += Widths[Index] + Gap;
	}
	return Total;
}

float AWizardHUD::TextWidth(const FString& Text, float PixelHeight, bool bBold, float Spacing) const
{
	const float Height = FMath::Max(PixelHeight, SmallestTextHeight);
	if (TextFont && FSlateApplication::IsInitialized() && FSlateApplication::Get().GetRenderer())
	{
		FSlateFontInfo Font(TextFont, FMath::Max(FMath::RoundToInt32(Height * 0.75f), 6), bBold ? FName("Bold") : FName("Regular"));
		Font.LetterSpacing = FMath::RoundToInt32(Spacing * 1000.0f);
		return static_cast<float>(FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Text, Font).X);
	}
	// Roboto's letters are a little over half their height wide.
	return Text.Len() * Height * (0.52f + Spacing);
}

void AWizardHUD::DrawFrame(const FBox2D& Box, float Thickness, const FLinearColor& Color)
{
	const float Left = static_cast<float>(Box.Min.X);
	const float Top = static_cast<float>(Box.Min.Y);
	const float Wide = static_cast<float>(Box.GetSize().X);
	const float Tall = static_cast<float>(Box.GetSize().Y);
	DrawRect(Color, Left, Top, Wide, Thickness);
	DrawRect(Color, Left, Top + Tall - Thickness, Wide, Thickness);
	DrawRect(Color, Left, Top + Thickness, Thickness, Tall - Thickness * 2.0f);
	DrawRect(Color, Left + Wide - Thickness, Top + Thickness, Thickness, Tall - Thickness * 2.0f);
}

void AWizardHUD::DrawPanel(const FBox2D& Box, float Scale)
{
	const float Frame = FMath::Max(FMath::RoundToFloat(3.0f * Scale), 2.0f);
	DrawRect(HudFaded(PanelColor, PanelColor.A * MenuAlpha), Box.Min.X, Box.Min.Y, Box.GetSize().X, Box.GetSize().Y);
	DrawFrame(FBox2D(Box.Min - FVector2D(Frame, Frame), Box.Max + FVector2D(Frame, Frame)), Frame, HudFaded(ParchmentColor, MenuAlpha));
	const float Inset = FMath::RoundToFloat(6.0f * Scale);
	DrawFrame(FBox2D(Box.Min + FVector2D(Inset, Inset), Box.Max - FVector2D(Inset, Inset)), 1.0f, HudFaded(BoneTextColor, 0.14f * MenuAlpha));
}

float AWizardHUD::KeyCapWidth(const FString& Key, float Scale, bool bNarrow) const
{
	const float Least = FMath::RoundToFloat((bNarrow ? 28.0f : 30.0f) * Scale);
	return FMath::Max(Least, FMath::RoundToFloat(TextWidth(Key, KeyCapTextHeight * Scale, true) + (bNarrow ? 8.0f : 14.0f) * Scale));
}

float AWizardHUD::DrawKeyCap(const FString& Key, float X, float Y, float Scale, bool bRightAligned, bool bNarrow)
{
	const float CapWidth = KeyCapWidth(Key, Scale, bNarrow);
	const float Left = bRightAligned ? X - CapWidth : X;
	DrawRect(HudFaded(BoneTextColor, MenuAlpha), Left, Y, CapWidth, FMath::RoundToFloat(KeyCapHeight * Scale));
	// Dark letters on a pale cap: no shadow under them.
	DrawSharpText(Key, HudFaded(HudKeyTextColor, MenuAlpha), Left + CapWidth * 0.5f, Y + 3.0f * Scale, KeyCapTextHeight * Scale, true, true, 0.0f, false);
	return CapWidth;
}

void AWizardHUD::DrawMenuButton(const FBox2D& Box, const FString& Label, EWizardButtonLook Look, float TextHeight, float Scale, const TCHAR* Key, bool bLabelLeft)
{
	FLinearColor Fill = RoyalColor;
	FLinearColor Border = RoyalHiColor;
	FLinearColor TextColor = FLinearColor::White;
	if (Look == EWizardButtonLook::Secondary)
	{
		Fill = HudFaded(PanelColor, 0.7f);
		Border = ParchmentColor;
		TextColor = BoneTextColor;
	}
	else if (Look == EWizardButtonLook::Sacrifice)
	{
		Fill = CutDeepColor;
		Border = CutColor;
	}
	else if (Look == EWizardButtonLook::Off)
	{
		Fill = OffFillColor;
		Border = OffBorderColor;
		TextColor = OffTextColor;
	}
	Fill.A *= MenuAlpha;
	Border.A *= MenuAlpha;
	TextColor.A *= MenuAlpha;
	const float Left = static_cast<float>(Box.Min.X);
	const float Top = static_cast<float>(Box.Min.Y);
	const float Wide = static_cast<float>(Box.GetSize().X);
	const float Tall = static_cast<float>(Box.GetSize().Y);
	DrawRect(Fill, Left, Top, Wide, Tall);
	DrawFrame(Box, FMath::Max(FMath::RoundToFloat(2.0f * Scale), 1.0f), Border);
	const float TextY = Top + (Tall - TextHeight) * 0.5f - TextHeight * 0.1f;
	const FString KeyText(Key ? Key : TEXT(""));
	if (KeyText.IsEmpty())
	{
		if (bLabelLeft)
		{
			// In a column of left-aligned buttons, one with no key lines up with the others.
			DrawSharpText(Label, TextColor, Left + 24.0f * Scale, TextY, TextHeight, true);
			return;
		}
		DrawSharpText(Label, TextColor, Left + Wide * 0.5f, TextY, TextHeight, true, true);
		return;
	}
	const float CapY = Top + FMath::RoundToFloat((Tall - KeyCapHeight * Scale) * 0.5f);
	if (bLabelLeft)
	{
		DrawSharpText(Label, TextColor, Left + 24.0f * Scale, TextY, TextHeight, true);
		DrawKeyCap(KeyText, Left + Wide - 24.0f * Scale, CapY, Scale, true);
		return;
	}
	// The label and its key, together in the middle.
	const float LabelWidth = TextWidth(Label, TextHeight, true);
	const float Gap = 14.0f * Scale;
	const float Start = Left + (Wide - LabelWidth - Gap - KeyCapWidth(KeyText, Scale, false)) * 0.5f;
	DrawSharpText(Label, TextColor, Start, TextY, TextHeight, true);
	DrawKeyCap(KeyText, Start + LabelWidth + Gap, CapY, Scale, false);
}

void AWizardHUD::DrawRecallArrow(float X, float Y, float Size, float Scale)
{
	// A turning arrow: three quarters of a circle and its head. A stand-in until it has a picture.
	const FVector2D Middle(X + Size * 0.5f, Y + Size * 0.52f);
	const float Radius = Size * 0.27f;
	const float Thick = FMath::Max(3.0f * Scale, 2.0f);
	const int32 Steps = 12;
	FVector2D Previous = Middle + FVector2D(FMath::Cos(-0.6), FMath::Sin(-0.6)) * Radius;
	for (int32 Step = 1; Step <= Steps; ++Step)
	{
		const double Angle = -0.6 - 4.6 * Step / Steps;
		const FVector2D Next = Middle + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius;
		DrawLine(Previous.X, Previous.Y, Next.X, Next.Y, BoneTextColor, Thick);
		Previous = Next;
	}
	const FVector2D Tip = Middle + FVector2D(FMath::Cos(-0.6), FMath::Sin(-0.6)) * Radius;
	DrawLine(Tip.X, Tip.Y, Tip.X, Tip.Y - Size * 0.2f, BoneTextColor, Thick);
	DrawLine(Tip.X, Tip.Y, Tip.X - Size * 0.2f, Tip.Y, BoneTextColor, Thick);
}

void AWizardHUD::DrawHUD()
{
	Super::DrawHUD();

	const UCrowdSubsystem* Crowd = GetWorld() ? GetWorld()->GetSubsystem<UCrowdSubsystem>() : nullptr;
	if (!Canvas || !Crowd)
	{
		return;
	}

	const float Width = Canvas->ClipX;
	const float Height = Canvas->ClipY;
	const float Scale = LayoutScale(Width, Height);
	const float Overwhelm = Crowd->GetOverwhelm();
	const AWizardPawn* Wizard = Crowd->GetWizard();
	const UGameInstance* Game = GetGameInstance();
	const UWizardSettingsSubsystem* Store = Game ? Game->GetSubsystem<UWizardSettingsSubsystem>() : nullptr;
	const FWizardSettings Chosen = Store ? Store->Get() : FWizardSettings();
	EdgeFactor = Chosen.DangerEdges / 100.0f;

	// The music follows the menus, which pause the game: the wizard does not tick then, the HUD is still drawn.
	if (UWizardAudioSubsystem* Audio = Game && Wizard ? Game->GetSubsystem<UWizardAudioSubsystem>() : nullptr)
	{
		Audio->SetMusic(Wizard->GetWantedMusic());
	}

	// The mouse is a plain pointer on any menu and the crosshair in the game. Set here: the HUD is drawn while paused too.
	if (PlayerOwner)
	{
		const bool bMenu = WantsMenuPointer(Wizard && Wizard->IsMenuOpen(), GetWorld()->IsPaused(), Crowd->IsShrineOpen(), Crowd->IsRunWon());
		PlayerOwner->CurrentMouseCursor = bMenu ? EMouseCursor::Default : EMouseCursor::Crosshairs;
	}

	if (bDebugVisible && Wizard)
	{
		DrawRings(*Crowd, *Wizard);
	}
	if (bHealthBarsVisible && Wizard)
	{
		DrawHealthBars(*Crowd, *Wizard, Scale);
	}
	if (Wizard)
	{
		DrawCommand(*Crowd, *Wizard, Width, Height, Scale);
	}

	// The edge that leans in from the side the pressure comes from.
	// Each edge closes in by how many are on him from that side, so being surrounded closes in from all four at once.
	const FWizardEdgeWeights Target = SideWeights(Crowd->GetTouchingOffsets(), Wizard ? Wizard->GetCameraYaw() : 0.0f);
	const float FadeSpeed = 3.0f / EdgeFadeSeconds;
	ShownEdges.Left = FMath::FInterpTo(ShownEdges.Left, Target.Left * Overwhelm, RenderDelta, FadeSpeed);
	ShownEdges.Right = FMath::FInterpTo(ShownEdges.Right, Target.Right * Overwhelm, RenderDelta, FadeSpeed);
	ShownEdges.Top = FMath::FInterpTo(ShownEdges.Top, Target.Top * Overwhelm, RenderDelta, FadeSpeed);
	ShownEdges.Bottom = FMath::FInterpTo(ShownEdges.Bottom, Target.Bottom * Overwhelm, RenderDelta, FadeSpeed);
	DrawEdges(Width, Height);

	// The fall: the whole screen darkens as the god sinks.
	if (Crowd->IsFalling())
	{
		const float FallAlpha = Crowd->GetFallAlpha();
		DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, FallAlpha * FallAlpha * 0.9f), 0.0f, 0.0f, Width, Height);
		DrawSharpText(Crowd->HasLeftBattlefield() ? TEXT("You abandoned the battlefield") : TEXT("Overwhelmed"), FLinearColor(1.0f, 0.85f, 0.8f, FallAlpha),
			Width * 0.5f, Height * 0.42f, (Crowd->HasLeftBattlefield() ? 54.0f : FallTextHeight) * Scale, true, true);
	}

	const int32 SentCount = Crowd->GetCommandedCount();
	const bool bPlaying = !Crowd->IsFalling() && !Crowd->IsRunWon();

	// Top left: the army is the biggest number on screen, with what was sent beside it in gold, and the bones under it.
	const float LeftX = 32.0f * Scale;
	DrawTitleText(TEXT("ARMY"), HudBoneDimColor, LeftX, 28.0f * Scale, 18.0f * Scale, false, 0.18f);
	const FString ArmyText = FString::FromInt(Crowd->GetArmyCount());
	DrawSharpText(ArmyText, BoneTextColor, LeftX, 50.0f * Scale, HudArmyNumberHeight * Scale, true);
	if (SentCount > 0)
	{
		DrawSharpText(FString::Printf(TEXT("%d sent"), SentCount), HudGoldColor,
			LeftX + (ArmyText.Len() * HudArmyNumberHeight * HudDigitWidth + 14.0f) * Scale, 78.0f * Scale, 24.0f * Scale, false);
	}
	const float BoneSize = FMath::RoundToFloat(44.0f * Scale);
	const float BoneTop = FMath::RoundToFloat(118.0f * Scale);
	if (BoneIcon)
	{
		DrawTexture(BoneIcon, LeftX, BoneTop, BoneSize, BoneSize, 0.0f, 0.0f, 1.0f, 1.0f);
	}
	const FString BonesText = FString::FromInt(Crowd->GetBonesHeld());
	const float BonesX = LeftX + BoneSize + 10.0f * Scale;
	DrawSharpText(BonesText, BoneTextColor, BonesX, BoneTop + 5.0f * Scale, 30.0f * Scale, true);
	DrawSharpText(Crowd->GetBonesHeld() == 1 ? TEXT("bone") : TEXT("bones"), HudBoneDimColor, BonesX + (BonesText.Len() * 30.0f * HudDigitWidth + 10.0f) * Scale, BoneTop + 13.0f * Scale, 20.0f * Scale, false);

	// Overwhelm, top center: its name, then the bar with quarter marks.
	const float BarWidth = FMath::RoundToFloat(HudBarWidth * Scale);
	const float BarHeight = FMath::RoundToFloat(HudBarHeight * Scale);
	const float BarX = FMath::RoundToFloat((Width - BarWidth) * 0.5f);
	const float BarY = FMath::RoundToFloat(HudBarTop * Scale);
	const float Border = FMath::Max(FMath::RoundToFloat(2.0f * Scale), 2.0f);
	DrawTitleText(TEXT("OVERWHELM"), FLinearColor(0.945f, 0.89f, 0.86f), Width * 0.5f, 28.0f * Scale, 18.0f * Scale, true, 0.18f);
	DrawRect(FLinearColor(BoneTextColor.R, BoneTextColor.G, BoneTextColor.B, 0.35f), BarX - Border - 1.0f, BarY - Border - 1.0f, BarWidth + (Border + 1.0f) * 2.0f, BarHeight + (Border + 1.0f) * 2.0f);
	DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f), BarX - 1.0f, BarY - 1.0f, BarWidth + 2.0f, BarHeight + 2.0f);
	DrawRect(FMath::Lerp(FLinearColor(0.95f, 0.6f, 0.1f), FLinearColor(0.9f, 0.05f, 0.05f), Overwhelm), BarX, BarY, BarWidth * Overwhelm, BarHeight);
	for (int32 Mark = 1; Mark <= 3; ++Mark)
	{
		DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.55f), BarX + BarWidth * Mark * 0.25f - 1.0f, BarY, FMath::Max(2.0f * Scale, 1.0f), BarHeight);
	}

	// The commander is near: a pulsing red crown just right of the bar, where the eye already is.
	if (Crowd->IsCommanderNear() && bPlaying)
	{
		DrawCommanderIcon(BarX + BarWidth + (HudCommanderGap + HudCommanderRadius + 3.0f) * Scale, BarY + BarHeight * 0.5f, Scale);
	}
	// The commander's spear has him slowed.
	if (Crowd->IsWizardSlowed() && bPlaying)
	{
		DrawSharpText(FString::Printf(TEXT("Slowed %.1f s"), Crowd->GetSlowSeconds()), FLinearColor(0.6f, 0.8f, 1.0f),
			Width * 0.5f, BarY + BarHeight + 10.0f * Scale, LabelTextHeight * Scale, true, true);
	}

	if (bPlaying)
	{
		DrawAbilityBar(*Crowd, Width, Height, Scale);
	}

	if (bDebugVisible)
	{
		DrawDebug(Width, Scale, BarX + BarWidth + Border, BarY + BarHeight + (7.0f + LabelTextHeight * 1.6f) * Scale);
	}

	// Drawn after the debug panel, so the menu is never covered by it.
	if (Wizard)
	{
		DrawShrine(*Crowd, *Wizard, Width, Height, Scale);
	}

	// Outside the battlefield: come back, and how long is left to.
	if (Crowd->IsOutsideBattlefield())
	{
		const float Beat = 0.75f + 0.25f * FMath::Sin(static_cast<float>(GetWorld()->GetRealTimeSeconds()) * 8.0f);
		DrawTitleText(TEXT("RETURN TO THE BATTLEFIELD"), HudFaded(CutColor, Beat), Width * 0.5f, Height * 0.28f, 44.0f * Scale, true, 0.1f);
		DrawSharpText(FString::Printf(TEXT("The mission fails in %.1f s"), Crowd->GetBattlefieldSecondsLeft()), BoneTextColor,
			Width * 0.5f, Height * 0.28f + 62.0f * Scale, 26.0f * Scale, true, true);
	}

	// The commander has fallen: bars and a banner, then where to go.
	DrawGateMessage(*Crowd, Wizard, Width, Height, Scale);

	if (Crowd->IsRunWon())
	{
		DrawFinalCut(*Crowd, Width, Height, Scale);
	}

	// The frame rate, when the player asked for it: small, in the corner, under nothing but the menus.
	// The app's own frame time: the game's stands still while it is paused.
	ShownFrameMs = FMath::Lerp(ShownFrameMs, static_cast<float>(FApp::GetDeltaTime()) * 1000.0f, 0.05f);
	if (Chosen.FrameRate != 0)
	{
		const FString Rate = FString::Printf(TEXT("%.0f fps"), ShownFrameMs > 0.0f ? 1000.0f / ShownFrameMs : 0.0f);
		DrawSharpText(Rate, DetailGreyColor, Width - 20.0f * Scale - TextWidth(Rate, 16.0f * Scale, false), 14.0f * Scale, 16.0f * Scale, false);
	}

	if (Wizard && Wizard->IsMenuOpen())
	{
		DrawStartMenu(Width, Height, Scale);
	}
	else if (GetWorld()->IsPaused())
	{
		DrawPauseMenu(Width, Height, MenuScale(Width, Height));
	}
	if (Wizard && Wizard->GetSettingsScreen().IsOpen())
	{
		// The HUD is drawn while the game is paused and the pawn does not tick: a slider that is held follows the mouse from here.
		if (AWizardPawn* Holder = PlayerOwner ? Cast<AWizardPawn>(PlayerOwner->GetPawn()) : nullptr)
		{
			Holder->UpdateSettingsDrag();
			Holder->UpdateKeyCapture();
		}
		DrawSettings(*Wizard, Store ? Store->Get() : Chosen, Width, Height);
	}
}

void AWizardHUD::DrawShrine(const UCrowdSubsystem& Crowd, const AWizardPawn& Wizard, float Width, float Height, float Scale)
{
	// The circle on the ground shows how close the wizard has to be, and how far he can walk before the menu closes.
	const FVector2D Middle = Crowd.GetShrinePosition();
	const float Radius = Crowd.GetShrineRange();
	const float GroundZ = Wizard.GetGroundZ() + 4.0f;
	FLinearColor RingColor = ShrineColor;
	RingColor.A = Crowd.IsShrineOpen() ? 0.9f : 0.45f;
	FVector Previous = Canvas->Project(FVector(Middle.X + Radius, Middle.Y, GroundZ));
	for (int32 Index = 1; Index <= RingSegments; ++Index)
	{
		const double Angle = UE_DOUBLE_TWO_PI * Index / RingSegments;
		const FVector Next = Canvas->Project(FVector(Middle.X + FMath::Cos(Angle) * Radius, Middle.Y + FMath::Sin(Angle) * Radius, GroundZ));
		if (Previous.Z > 0.0 && Next.Z > 0.0)
		{
			DrawLine(Previous.X, Previous.Y, Next.X, Next.Y, RingColor, 2.0f);
		}
		Previous = Next;
	}

	if (!Crowd.IsShrineOpen())
	{
		if (Crowd.IsWizardAtShrine() && !Crowd.IsFalling())
		{
			DrawSharpText(FString::Printf(TEXT("%s: use the shrine"), *WizardKeys().GetLabel(TEXT("Shrine"))), ShrineColor, Width * 0.5f, Height * 0.78f, 30.0f * Scale, true, true);
		}
		return;
	}

	// The shop. Time runs slower behind it.
	const FBox2D Box = ShrineMenuBox(Width, Height);
	DrawPanel(Box, Scale);
	const float CenterX = Box.GetCenter().X;
	const float Side = FMath::RoundToFloat(MenuSidePadding * Scale);
	const float PanelLeft = static_cast<float>(Box.Min.X);
	const float PanelTop = static_cast<float>(Box.Min.Y);
	const float PanelWidth = static_cast<float>(Box.GetSize().X);
	DrawTitleText(TEXT("SHRINE"), BoneTextColor, CenterX, PanelTop + 20.0f * Scale, 44.0f * Scale, true, 0.14f);

	// The purse: the bones held, and what the army could pay on top, in the color of a sacrifice.
	const FCrowdShopPrice Most = Crowd.GetShopPrice(0, 100000);
	const int32 ArmyPays = Most.Skeletons * Crowd.GetSim().GetUnitWorth(ECrowdKind::Skeleton) + Most.Guards * Crowd.GetSim().GetUnitWorth(ECrowdKind::Guard);
	const FString BonesText = FString::FromInt(Crowd.GetBonesHeld());
	const FString ArmyText = FString::FromInt(ArmyPays);
	const float PurseIcon = FMath::RoundToFloat(36.0f * Scale);
	const float BonesWidth = TextWidth(BonesText, 28.0f * Scale, true);
	const TCHAR* BonesWord = Crowd.GetBonesHeld() == 1 ? TEXT("bone") : TEXT("bones");
	const float WordWidth = TextWidth(BonesWord, 18.0f * Scale, false);
	const float ArmyLabelWidth = TextWidth(TEXT("Army can pay"), 18.0f * Scale, false);
	const float ArmyWidth = TextWidth(ArmyText, 28.0f * Scale, true);
	const float PurseTop = PanelTop + FMath::RoundToFloat(84.0f * Scale);
	float PurseX = CenterX - (PurseIcon + BonesWidth + WordWidth + ArmyLabelWidth + ArmyWidth + (10.0f + 8.0f + 22.0f + 22.0f + 10.0f) * Scale) * 0.5f;
	if (BoneIcon)
	{
		DrawTexture(BoneIcon, PurseX, PurseTop, PurseIcon, PurseIcon, 0.0f, 0.0f, 1.0f, 1.0f);
	}
	PurseX += PurseIcon + 10.0f * Scale;
	DrawSharpText(BonesText, BoneTextColor, PurseX, PurseTop + 1.0f * Scale, 28.0f * Scale, true);
	PurseX += BonesWidth + 8.0f * Scale;
	DrawSharpText(BonesWord, HudBoneDimColor, PurseX, PurseTop + 9.0f * Scale, 18.0f * Scale, false);
	PurseX += WordWidth + 22.0f * Scale;
	DrawRect(HudFaded(BoneTextColor, 0.25f), PurseX, PurseTop + 3.0f * Scale, 1.0f, 30.0f * Scale);
	PurseX += 22.0f * Scale;
	DrawSharpText(TEXT("Army can pay"), HudBoneDimColor, PurseX, PurseTop + 9.0f * Scale, 18.0f * Scale, false);
	PurseX += ArmyLabelWidth + 10.0f * Scale;
	DrawSharpText(ArmyText, CutColor, PurseX, PurseTop + 1.0f * Scale, 28.0f * Scale, true);

	DrawTitleText(TEXT("RESEARCH"), BoneTextColor, PanelLeft + Side, PanelTop + 138.0f * Scale, 18.0f * Scale, false, 0.12f);
	DrawRect(HudFaded(BoneTextColor, 0.18f), PanelLeft + Side, PanelTop + FMath::RoundToFloat(168.0f * Scale), PanelWidth - Side * 2.0f, 1.0f);

	const FCrowdTuning& Tuning = Crowd.GetTuning();
	const FString Names[ShrineMenuRows] = {
		TEXT("Spear of Sacrifice"),
		TEXT("Bone armor"),
		TEXT("Bone blades"),
		TEXT("Dark focus"),
		TEXT("Guard skeleton"),
		TEXT("Marksman skeleton"),
	};
	const FString Details[ShrineMenuRows] = {
		FString::Printf(TEXT("%d bones \u00B7 A new ability"), Crowd.GetShopCost(0)),
		FString::Printf(TEXT("%d bones \u00B7 Skeletons +%.0f health"), Crowd.GetShopCost(1), Tuning.ArmorHealth),
		FString::Printf(TEXT("%d bones \u00B7 Skeletons +%.0f damage"), Crowd.GetShopCost(2), Tuning.BladesDamage),
		FString::Printf(TEXT("%d bones \u00B7 Bolt +%.0f damage"), Crowd.GetShopCost(3), Tuning.FocusBoltDamage),
		FString::Printf(TEXT("%d bones each"), Crowd.GetShopCost(4)),
		FString::Printf(TEXT("%d bones each \u00B7 archers first"), Crowd.GetShopCost(5)),
	};
	// A row with no picture shows its first letter.
	UTexture2D* const RowIcons[ShrineMenuRows] = { SpearIcon, ArmorIcon, BladesIcon, FocusIcon, GuardIcon, MarksmanIcon };
	const float RowHeight = FMath::RoundToFloat(MenuRowHeight * Scale);
	const float IconSize = FMath::RoundToFloat(MenuIconSize * Scale);
	const float Line = FMath::Max(FMath::RoundToFloat(2.0f * Scale), 1.0f);
	for (int32 Row = 0; Row < ShrineMenuRows; ++Row)
	{
		const float RowTop = PanelTop + FMath::RoundToFloat((MenuRowsTop + Row * MenuRowHeight) * Scale);
		DrawRect(HudFaded(BoneTextColor, 0.1f), PanelLeft + Side, RowTop + RowHeight - 1.0f, PanelWidth - Side * 2.0f, 1.0f);

		const float IconX = PanelLeft + Side;
		const float IconY = RowTop + FMath::RoundToFloat((RowHeight - IconSize) * 0.5f);
		DrawRect(FLinearColor::Black, IconX, IconY, IconSize, IconSize);
		if (RowIcons[Row])
		{
			const float Margin = (1.0f - HudIconCrop) * 0.5f;
			DrawTexture(RowIcons[Row], IconX, IconY, IconSize, IconSize, Margin, Margin, HudIconCrop, HudIconCrop);
		}
		else
		{
			DrawSharpText(Names[Row].Left(1), HudBoneDimColor, IconX + IconSize * 0.5f, IconY + 11.0f * Scale, 28.0f * Scale, true, true);
		}
		DrawFrame(FBox2D(FVector2D(IconX, IconY), FVector2D(IconX + IconSize, IconY + IconSize)), Line, HudFaded(BoneTextColor, 0.35f));

		const bool bUnit = ShrineMenuButtonCount(Row) > 1;
		const int32 CanBuy = Crowd.GetShopMax(Row);
		const bool bResearched = Crowd.IsResearched(Row);
		// When the bones held do not reach, the row says who in the army pays the rest, in the color of a sacrifice.
		const FCrowdShopPrice Price = Crowd.GetShopPrice(Row, 1);
		const bool bArmyPays = !bResearched && Price.bCanPay && Price.Skeletons + Price.Guards > 0;
		FString Detail = Details[Row];
		FLinearColor DetailColor = DetailGreyColor;
		if (bResearched)
		{
			Detail = TEXT("Researched");
			DetailColor = MenuReadyColor;
		}
		else if (bArmyPays)
		{
			Detail = FString::Printf(TEXT("%d bone%s + %d skeleton%s"), Price.Bones, Price.Bones == 1 ? TEXT("") : TEXT("s"), Price.Skeletons, Price.Skeletons == 1 ? TEXT("") : TEXT("s"));
			if (Price.Guards > 0)
			{
				Detail += FString::Printf(TEXT(", %d Guard%s"), Price.Guards, Price.Guards == 1 ? TEXT("") : TEXT("s"));
			}
			DetailColor = CutColor;
		}
		// The name, its detail under it, and for a unit how many can be bought: together in the middle of the row.
		const float TextX = IconX + IconSize + 16.0f * Scale;
		const float TextTall = MenuNameHeight + 5.0f + MenuDetailHeight + (bUnit ? 4.0f + MenuDetailHeight : 0.0f);
		const float TextY = RowTop + ((MenuRowHeight - TextTall) * 0.5f - 3.0f) * Scale;
		DrawSharpText(Names[Row], BoneTextColor, TextX, TextY, MenuNameHeight * Scale, true);
		DrawSharpText(Detail, DetailColor, TextX, TextY + (MenuNameHeight + 5.0f) * Scale, MenuDetailHeight * Scale, false);
		if (bUnit)
		{
			DrawSharpText(FString::Printf(TEXT("Can buy %d"), CanBuy), DetailGreyColor, TextX, TextY + (MenuNameHeight + 9.0f + MenuDetailHeight) * Scale, MenuDetailHeight * Scale, false);
		}

		// A research that is done has no button left.
		const int32 Buttons = bResearched ? 0 : ShrineMenuButtonCount(Row);
		for (int32 Button = 0; Button < Buttons; ++Button)
		{
			FString Label = bArmyPays ? TEXT("Sacrifice") : TEXT("Research");
			bool bOn = CanBuy > 0;
			if (bUnit)
			{
				Label = Button == 0 ? TEXT("x1") : Button == 1 ? TEXT("x5") : TEXT("Max");
				// When only the army can pay, one at a time: the other two are off.
				bOn = bArmyPays ? Button == 0 : Button == 1 ? CanBuy >= 5 : CanBuy > 0;
			}
			DrawMenuButton(ShrineMenuButtonBox(Row, Button, Width, Height), Label,
				!bOn ? EWizardButtonLook::Off : bArmyPays ? EWizardButtonLook::Sacrifice : EWizardButtonLook::Royal, 20.0f * Scale, Scale);
		}
	}

	const float FooterY = static_cast<float>(Box.Max.Y) - 40.0f * Scale;
	DrawSharpText(TEXT("Close: E, click outside, or walk away"), HudBoneDimColor, PanelLeft + Side, FooterY, 16.0f * Scale, false);
	const float TimeScale = Tuning.ShrineTimeScale;
	const FString TimeText = FMath::IsNearlyEqual(TimeScale, 0.5f, 0.01f) ? FString(TEXT("Time runs at half speed"))
		: FString::Printf(TEXT("Time runs at %.0f%% speed"), TimeScale * 100.0f);
	DrawSharpText(TimeText, HudBoneDimColor, PanelLeft + PanelWidth - Side - TextWidth(TimeText, 16.0f * Scale, false), FooterY, 16.0f * Scale, false);
}

void AWizardHUD::DrawControlsPanel(const FBox2D& Box, float Scale)
{
	DrawPanel(Box, Scale);
	const float PanelLeft = static_cast<float>(Box.Min.X);
	const float PanelTop = static_cast<float>(Box.Min.Y);
	DrawTitleText(TEXT("CONTROLS"), BoneTextColor, Box.GetCenter().X, PanelTop + 24.0f * Scale, 22.0f * Scale, true, 0.16f);

	struct FControl
	{
		const TCHAR* Keys[4];
		UTexture2D* Icon;
		bool bRecall;
		const TCHAR* Action;
		const TCHAR* Hint;
	};
	// The keys as the player has them now.
	const FHudKeyLabels K;
	const FControl LeftRows[] = {
		{ { *K.Up, *K.Left, *K.Down, *K.Right }, nullptr, false, TEXT("Move"), nullptr },
		{ { TEXT("Mouse"), nullptr, nullptr, nullptr }, nullptr, false, TEXT("Aim"), nullptr },
		{ { *K.Bolt, nullptr, nullptr, nullptr }, nullptr, false, TEXT("Bolt"), nullptr },
		{ { *K.Shrine, nullptr, nullptr, nullptr }, nullptr, false, TEXT("Use the shrine"), nullptr },
		{ { *K.Pause, nullptr, nullptr, nullptr }, nullptr, false, TEXT("Pause"), nullptr },
	};
	const FControl RightRows[] = {
		{ { *K.Summon, nullptr, nullptr, nullptr }, SummonIcon, false, TEXT("Summon the Afterlife"), TEXT("Raise skeletons from the bones you hold") },
		{ { *K.Spear, nullptr, nullptr, nullptr }, SpearIcon, false, TEXT("Spear of Sacrifice"), TEXT("Paid in bones, or by a skeleton") },
		{ { *K.Blink, nullptr, nullptr, nullptr }, BlinkIcon, false, TEXT("Blink"), TEXT("Dash the way you move") },
		{ { *K.Command, nullptr, nullptr, nullptr }, CommandIcon, false, TEXT("Command the army"), *K.CommandHint },
		{ { *K.Recall, nullptr, nullptr, nullptr }, nullptr, true, TEXT("Recall the army"), TEXT("Every sent unit comes back") },
	};
	// The right column is the wider one: its rows have a hint under the action.
	const float RowsTop = PanelTop + 64.0f * Scale;
	const float RowPitch = 56.0f * Scale;
	const float IconSize = FMath::RoundToFloat(40.0f * Scale);
	const auto DrawColumn = [&](const FControl* Rows, int32 Count, float X)
	{
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FControl& Control = Rows[Index];
			const float Y = RowsTop + Index * RowPitch;
			// The keys, right aligned in their column: four narrow ones fit side by side.
			int32 Keys = 0;
			while (Keys < 4 && Control.Keys[Keys])
			{
				++Keys;
			}
			float CapRight = X + 150.0f * Scale;
			for (int32 Key = Keys - 1; Key >= 0; --Key)
			{
				CapRight -= DrawKeyCap(Control.Keys[Key], CapRight, Y + 8.0f * Scale, Scale, true, Keys > 1) + 6.0f * Scale;
			}
			const float IconX = X + 162.0f * Scale;
			const float IconY = Y + 2.0f * Scale;
			if (Control.Icon || Control.bRecall)
			{
				DrawRect(HudHex(0x0A0C10), IconX, IconY, IconSize, IconSize);
				if (Control.Icon)
				{
					const float Margin = (1.0f - HudIconCrop) * 0.5f;
					DrawTexture(Control.Icon, IconX, IconY, IconSize, IconSize, Margin, Margin, HudIconCrop, HudIconCrop);
				}
				else
				{
					DrawRecallArrow(IconX, IconY, IconSize, Scale * 0.6f);
				}
				DrawFrame(FBox2D(FVector2D(IconX, IconY), FVector2D(IconX + IconSize, IconY + IconSize)), 1.0f, HudFaded(BoneTextColor, 0.35f));
			}
			const float TextX = IconX + IconSize + 12.0f * Scale;
			if (Control.Hint)
			{
				DrawSharpText(Control.Action, BoneTextColor, TextX, Y - 1.0f * Scale, 21.0f * Scale, false);
				DrawSharpText(Control.Hint, DetailGreyColor, TextX, Y + 25.0f * Scale, 16.0f * Scale, false);
			}
			else
			{
				DrawSharpText(Control.Action, BoneTextColor, TextX, Y + 9.0f * Scale, 21.0f * Scale, false);
			}
		}
	};
	const float ColumnX = PanelLeft + 34.0f * Scale;
	DrawColumn(LeftRows, UE_ARRAY_COUNT(LeftRows), ColumnX);
	DrawColumn(RightRows, UE_ARRAY_COUNT(RightRows), ColumnX + (380.0f + 46.0f) * Scale);
#if !UE_BUILD_SHIPPING
	DrawSharpText(TEXT("Testing builds only: H debug stats \u00B7 1 / 2 / 3 camera"), OffTextColor, Box.GetCenter().X, PanelTop + 346.0f * Scale, 16.0f * Scale, false, true);
#endif
}

void AWizardHUD::DrawPauseMenu(float Width, float Height, float Scale)
{
	DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.62f), 0.0f, 0.0f, Width, Height);
	DrawTitleText(TEXT("PAUSED"), BoneTextColor, Width * 0.5f, 126.0f * Scale, 76.0f * Scale, true, 0.16f);
	DrawMenuButton(PauseMenuButtonBox(0, Width, Height), TEXT("Resume"), EWizardButtonLook::Royal, 24.0f * Scale, Scale, *WizardKeys().GetLabel(TEXT("Pause")));
	DrawMenuButton(PauseMenuButtonBox(1, Width, Height), TEXT("Settings"), EWizardButtonLook::Secondary, 24.0f * Scale, Scale);
	DrawMenuButton(PauseMenuButtonBox(2, Width, Height), TEXT("Restart the run"), EWizardButtonLook::Secondary, 24.0f * Scale, Scale, TEXT("R"));
	DrawControlsPanel(ControlsPanelBox(Width, Height), Scale);
}

void AWizardHUD::DrawStartMenu(float Width, float Height, float Scale)
{
	// The picture fills the screen, cut at the sides or at top and bottom to keep its shape.
	DrawRect(FLinearColor(StartShadeColor.R, StartShadeColor.G, StartShadeColor.B, 1.0f), 0.0f, 0.0f, Width, Height);
	if (StartBackground && StartBackground->GetSizeY() > 0 && Height > 0.0f)
	{
		const float Picture = static_cast<float>(StartBackground->GetSizeX()) / StartBackground->GetSizeY();
		const float Screen = Width / Height;
		const float ShareU = Screen < Picture ? Screen / Picture : 1.0f;
		const float ShareV = Screen > Picture ? Picture / Screen : 1.0f;
		DrawTexture(StartBackground, 0.0f, 0.0f, Width, Height, (1.0f - ShareU) * 0.5f, (1.0f - ShareV) * 0.5f, ShareU, ShareV);
	}

	// Dark from the left, so the words read over the picture: 0.9 at the edge, 0.7 a quarter in, clear before the middle.
	const int32 Strips = 46;
	const float StripWidth = Width * 0.46f / Strips;
	for (int32 Strip = 0; Strip < Strips; ++Strip)
	{
		const float Along = (Strip + 0.5f) * 0.46f / Strips;
		const float Alpha = Along < 0.26f ? FMath::Lerp(0.9f, 0.7f, Along / 0.26f) : FMath::Lerp(0.7f, 0.0f, (Along - 0.26f) / 0.2f);
		// Whole pixels, edge to edge: strips that overlap or leave a gap show as lines.
		const float StripLeft = FMath::RoundToFloat(Strip * StripWidth);
		DrawRect(HudFaded(StartShadeColor, Alpha), StripLeft, 0.0f, FMath::RoundToFloat((Strip + 1) * StripWidth) - StripLeft, Height);
	}

	const float X = StartColumnX * Scale;
	DrawTitleText(TEXT("THE VILLAIN'S CUT"), RoyalHiColor, X, 200.0f * Scale, 22.0f * Scale, false, 0.32f);
	// The name is a stand-in: the game has no title yet.
	DrawTitleText(TEXT("THE BONE"), BoneTextColor, X, 236.0f * Scale, 112.0f * Scale);
	DrawTitleText(TEXT("KING"), BoneTextColor, X, 342.0f * Scale, 112.0f * Scale);
	const TCHAR* Tagline[] = {
		TEXT("They drove you from what was rightfully yours."),
		TEXT("Raise the dead, build your army,"),
		TEXT("and take back what is yours."),
	};
	for (int32 LineIndex = 0; LineIndex < UE_ARRAY_COUNT(Tagline); ++LineIndex)
	{
		DrawSharpText(Tagline[LineIndex], HudBoneDimColor, X, (479.0f + LineIndex * 38.0f) * Scale, 28.0f * Scale, false);
	}
	DrawMenuButton(StartMenuButtonBox(0, Width, Height), TEXT("March"), EWizardButtonLook::Royal, 26.0f * Scale, Scale, TEXT("Enter"), true);
	DrawMenuButton(StartMenuButtonBox(1, Width, Height), TEXT("Settings"), EWizardButtonLook::Secondary, 26.0f * Scale, Scale, nullptr, true);

	const FLinearColor FooterColor = HudHex(0x8B8D93);
	const float FooterY = Height - (56.0f + 18.0f) * Scale;
	const FString Made(TEXT("Made for the Epic AI MegaJam 2026 \u00B7 theme \"Director's Cut\""));
	DrawSharpText(Made, FooterColor, Width - 80.0f * Scale - TextWidth(Made, 18.0f * Scale, false), FooterY, 18.0f * Scale, false);
}

void AWizardHUD::DrawFinalCut(const UCrowdSubsystem& Crowd, float Width, float Height, float Scale)
{
	// The game, frozen behind it, darkens over a second, eased. Then each part comes up from a little lower.
	const float Seconds = Crowd.GetWonSeconds();
	DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.82f * HudEaseOut(HudRamp(Seconds, 0.0f, FinalCutDarkenSeconds))), 0.0f, 0.0f, Width, Height);
	const float Middle = Width * 0.5f;
	const float Rise = 12.0f * Scale;

	// "FINAL CUT", with a line to each side that fades away from it.
	{
		const float Alpha = FinalCutPartAlpha(0, Seconds);
		const float Y = 150.0f * Scale + (1.0f - Alpha) * Rise;
		const float Half = DrawTitleText(TEXT("FINAL CUT"), HudFaded(RoyalHiColor, Alpha), Middle, Y, 26.0f * Scale, true, 0.5f) * 0.5f + 26.0f * Scale;
		DrawFadingLine(Middle + Half, Y + 18.0f * Scale, 180.0f * Scale, HudFaded(RoyalHiColor, 0.8f * Alpha));
		DrawFadingLine(Middle - Half, Y + 18.0f * Scale, -180.0f * Scale, HudFaded(RoyalHiColor, 0.8f * Alpha));
	}
	// The title: the one gold on any screen.
	{
		const float Alpha = FinalCutPartAlpha(1, Seconds);
		DrawTitleText(TEXT("The Outskirts are yours"), HudFaded(GoldColor, Alpha), Middle, 196.0f * Scale + (1.0f - Alpha) * Rise, 84.0f * Scale, true);
	}
	// How long it took.
	{
		const float Alpha = FinalCutPartAlpha(2, Seconds);
		const int32 Whole = FMath::FloorToInt32(Crowd.GetRunSeconds());
		const FString Time = FString::Printf(TEXT("%d:%02d"), Whole / 60, Whole % 60);
		const float WordsWidth = TextWidth(TEXT("Taken in "), 28.0f * Scale, false);
		const float Left = Middle - (WordsWidth + TextWidth(Time, 28.0f * Scale, true)) * 0.5f;
		const float Y = 310.0f * Scale + (1.0f - Alpha) * Rise;
		DrawSharpText(TEXT("Taken in "), HudFaded(HudBoneDimColor, Alpha), Left, Y, 28.0f * Scale, false);
		DrawSharpText(Time, HudFaded(BoneTextColor, Alpha), Left + WordsWidth, Y, 28.0f * Scale, true);
	}
	// The tally: four numbers, then the ledger of the raised.
	{
		const float Alpha = FinalCutPartAlpha(3, Seconds);
		const float PanelLeft = FMath::RoundToFloat(Middle - FinalCutWidth * Scale * 0.5f);
		const float PanelTop = FMath::RoundToFloat(FinalCutPanelTop * Scale + (1.0f - Alpha) * Rise);
		const float PanelWidth = FMath::RoundToFloat(FinalCutWidth * Scale);
		MenuAlpha = Alpha;
		DrawPanel(FBox2D(FVector2D(PanelLeft, PanelTop), FVector2D(PanelLeft + PanelWidth, PanelTop + FMath::RoundToFloat(FinalCutPanelHeight * Scale))), Scale);
		MenuAlpha = 1.0f;

		const FWizardLedger Ledger = FinalCutLedger(Crowd.GetTotalRaised(), Crowd.GetSacrificed(), Crowd.GetArmyCount());
		const float Side = 48.0f * Scale;
		const float Inner = PanelWidth - Side * 2.0f;
		const float Column = Inner / 4.0f;
		const int32 Numbers[4] = { Crowd.GetTotalRaised(), Crowd.GetSacrificed(), Crowd.GetTotalKilled(), Crowd.GetArmyCount() };
		const TCHAR* Labels[4] = { TEXT("SKELETONS RAISED"), TEXT("SKELETONS SACRIFICED"), TEXT("ENEMIES KILLED"), TEXT("ARMY AT THE GATE") };
		for (int32 Index = 0; Index < 4; ++Index)
		{
			const float ColumnMiddle = PanelLeft + Side + (Index + 0.5f) * Column;
			DrawSharpText(FString::FromInt(FinalCutCount(Numbers[Index], Seconds)), HudFaded(Index == 1 ? CutColor : BoneTextColor, Alpha),
				ColumnMiddle, PanelTop + 26.0f * Scale, 68.0f * Scale, true, true);
			DrawTitleText(Labels[Index], HudFaded(HudBoneDimColor, Alpha), ColumnMiddle, PanelTop + 110.0f * Scale, 17.0f * Scale, true, 0.08f);
			if (Index > 0)
			{
				DrawRect(HudFaded(BoneTextColor, 0.18f * Alpha), PanelLeft + Side + Index * Column, PanelTop + 34.0f * Scale, 1.0f, 98.0f * Scale);
			}
		}

		const float LedgerLeft = PanelLeft + Side;
		const float LedgerTop = PanelTop + 164.0f * Scale;
		const float OfWidth = TextWidth(TEXT("Of the "), 20.0f * Scale, false);
		const FString RaisedText = FString::FromInt(Crowd.GetTotalRaised());
		DrawSharpText(TEXT("Of the "), HudFaded(HudBoneDimColor, Alpha), LedgerLeft, LedgerTop, 20.0f * Scale, false);
		DrawSharpText(RaisedText, HudFaded(BoneTextColor, Alpha), LedgerLeft + OfWidth, LedgerTop, 20.0f * Scale, true);
		DrawSharpText(TEXT(" you raised"), HudFaded(HudBoneDimColor, Alpha), LedgerLeft + OfWidth + TextWidth(RaisedText, 20.0f * Scale, true), LedgerTop, 20.0f * Scale, false);

		// The bar: kept, cut, fell, each its share, growing in.
		const float BarTop = FMath::RoundToFloat(LedgerTop + 36.0f * Scale);
		const float BarHeight = FMath::RoundToFloat(20.0f * Scale);
		const float Grown = HudEaseOut(HudRamp(Seconds, FinalCutDarkenSeconds + 1.5f, 0.9f));
		DrawFrame(FBox2D(FVector2D(LedgerLeft - 3.0f, BarTop - 3.0f), FVector2D(LedgerLeft + Inner + 3.0f, BarTop + BarHeight + 3.0f)), 1.0f, HudFaded(BoneTextColor, 0.3f * Alpha));
		DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, Alpha), LedgerLeft, BarTop, Inner, BarHeight);
		const float Shares[3] = { Ledger.KeptShare, Ledger.CutShare, Ledger.FellShare };
		const FLinearColor Colors[3] = { BoneTextColor, CutColor, FallenColor };
		float PartLeft = LedgerLeft;
		for (int32 Part = 0; Part < 3; ++Part)
		{
			const float PartWidth = Inner * Shares[Part] * Grown;
			DrawRect(HudFaded(Colors[Part], Alpha), PartLeft, BarTop, PartWidth, BarHeight);
			PartLeft += Inner * Shares[Part];
		}

		// What each color is.
		const float LegendTop = BarTop + BarHeight + 14.0f * Scale;
		const FString Words[3] = { TEXT("Kept "), TEXT("Cut for power "), TEXT("Fell in battle ") };
		const int32 Counts[3] = { Ledger.Kept, Ledger.Cut, Ledger.Fell };
		float LegendX = LedgerLeft;
		for (int32 Part = 0; Part < 3; ++Part)
		{
			DrawRect(HudFaded(Colors[Part], Alpha), LegendX, LegendTop + 4.0f * Scale, 16.0f * Scale, 16.0f * Scale);
			LegendX += 26.0f * Scale;
			DrawSharpText(Words[Part], HudFaded(DetailGreyColor, Alpha), LegendX, LegendTop, 19.0f * Scale, false);
			LegendX += TextWidth(Words[Part], 19.0f * Scale, false);
			const FString CountText = FString::FromInt(Counts[Part]);
			DrawSharpText(CountText, HudFaded(BoneTextColor, Alpha), LegendX, LegendTop, 19.0f * Scale, true);
			LegendX += TextWidth(CountText, 19.0f * Scale, true) + 34.0f * Scale;
		}
	}
	// The buttons: again, or onward (locked: that part of the tale is not made).
	{
		const float Alpha = FinalCutPartAlpha(4, Seconds);
		MenuAlpha = Alpha;
		DrawMenuButton(FinalCutButtonBox(0, Width, Height), TEXT("March again"), EWizardButtonLook::Royal, 26.0f * Scale, Scale, TEXT("R"), true);
		const FBox2D Locked = FinalCutButtonBox(1, Width, Height);
		DrawMenuButton(Locked, TEXT(""), EWizardButtonLook::Off, 26.0f * Scale, Scale);
		MenuAlpha = 1.0f;
		const float LockedLeft = static_cast<float>(Locked.Min.X);
		const float LockedTop = static_cast<float>(Locked.Min.Y);
		const float LockedTall = static_cast<float>(Locked.GetSize().Y);
		const FLinearColor Grey = HudFaded(OffTextColor, Alpha);
		DrawSharpText(TEXT("Enter the kingdom"), Grey, LockedLeft + 24.0f * Scale, LockedTop + (LockedTall - 26.0f * Scale) * 0.5f - 2.6f * Scale, 26.0f * Scale, true);
		// A padlock at the right end.
		{
			const float LockLeft = static_cast<float>(Locked.Max.X) - (24.0f + 22.0f) * Scale;
			const float LockTop = LockedTop + (LockedTall - 26.0f * Scale) * 0.5f;
			const float Thick = FMath::Max(2.5f * Scale, 1.5f);
			DrawFrame(FBox2D(FVector2D(LockLeft, LockTop + 11.0f * Scale), FVector2D(LockLeft + 22.0f * Scale, LockTop + 26.0f * Scale)), Thick, Grey);
			// The hoop: small squares along a half circle and down its two legs. (Lines do not fade with the rest, so
			// the hoop showed before the screen had come in.)
			if (Alpha > 0.0f)
			{
				const FVector2D Hoop(LockLeft + 11.0f * Scale, LockTop + 8.0f * Scale);
				const float Reach = 6.0f * Scale;
				const float HalfThick = Thick * 0.5f;
				const int32 Steps = FMath::Max(FMath::CeilToInt(UE_PI * Reach / FMath::Max(HalfThick, 0.5f)), 8);
				for (int32 Step = 0; Step <= Steps; ++Step)
				{
					const double Angle = UE_DOUBLE_PI + UE_DOUBLE_PI * Step / Steps;
					const FVector2D Point = Hoop + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Reach;
					DrawRect(Grey, static_cast<float>(Point.X) - HalfThick, static_cast<float>(Point.Y) - HalfThick, Thick, Thick);
				}
				const float LegTall = LockTop + 11.0f * Scale - static_cast<float>(Hoop.Y);
				DrawRect(Grey, static_cast<float>(Hoop.X) - Reach - HalfThick, static_cast<float>(Hoop.Y), Thick, LegTall);
				DrawRect(Grey, static_cast<float>(Hoop.X) + Reach - HalfThick, static_cast<float>(Hoop.Y), Thick, LegTall);
			}
		}

		// Its hover text: why it is locked.
		float MouseX = 0.0f;
		float MouseY = 0.0f;
		const bool bOver = Alpha >= 1.0f && PlayerOwner && PlayerOwner->GetMousePosition(MouseX, MouseY) && Locked.IsInside(FVector2D(MouseX, MouseY));
		LockedHover = FMath::FInterpConstantTo(LockedHover, bOver ? 1.0f : 0.0f, RenderDelta, 1.0f / 0.15f);
		if (LockedHover > 0.0f)
		{
			const FString Lines[2] = { TEXT("The kingdom's gates stay sealed for now."), TEXT("This part of the tale is still being written.") };
			const float TipWidth = FMath::RoundToFloat(FMath::Max(TextWidth(Lines[0], 20.0f * Scale, false), TextWidth(Lines[1], 20.0f * Scale, false)) + 40.0f * Scale);
			const float TipHeight = FMath::RoundToFloat(84.0f * Scale);
			const float Pointer = FMath::RoundToFloat(9.0f * Scale);
			const float TipLeft = FMath::RoundToFloat(static_cast<float>(Locked.GetCenter().X) - TipWidth * 0.5f);
			const float TipTop = LockedTop - 14.0f * Scale - Pointer - TipHeight;
			DrawRect(HudFaded(PanelColor, PanelColor.A * LockedHover), TipLeft, TipTop, TipWidth, TipHeight);
			DrawFrame(FBox2D(FVector2D(TipLeft, TipTop), FVector2D(TipLeft + TipWidth, TipTop + TipHeight)), FMath::Max(FMath::RoundToFloat(2.0f * Scale), 1.0f), HudFaded(ParchmentColor, LockedHover));
			for (int32 Strip = 0; Strip < 6; ++Strip)
			{
				// A pointer down to the button.
				const float Half = Pointer * (1.0f - (Strip + 0.5f) / 6.0f);
				DrawRect(HudFaded(ParchmentColor, LockedHover), TipLeft + TipWidth * 0.5f - Half, TipTop + TipHeight + Strip * Pointer / 6.0f, Half * 2.0f, Pointer / 6.0f + 1.0f);
			}
			DrawSharpText(Lines[0], HudFaded(BoneTextColor, LockedHover), TipLeft + TipWidth * 0.5f, TipTop + 13.0f * Scale, 20.0f * Scale, false, true);
			DrawSharpText(Lines[1], HudFaded(BoneTextColor, LockedHover), TipLeft + TipWidth * 0.5f, TipTop + 41.0f * Scale, 20.0f * Scale, false, true);
		}
	}
}

void AWizardHUD::DrawSettingsKeyBox(const FBox2D& Box, const FString& Key, int32 State, float Scale)
{
	const float Left = static_cast<float>(Box.Min.X);
	const float Top = static_cast<float>(Box.Min.Y);
	const float Wide = static_cast<float>(Box.GetSize().X);
	const float Tall = static_cast<float>(Box.GetSize().Y);
	const float Thick = FMath::Max(FMath::RoundToFloat(2.0f * Scale), 1.0f);
	// The name is made smaller when it would not fit its box ("Mouse 4" in a move key).
	const auto Fitted = [this, Wide, Scale](const FString& Text, float Most)
	{
		const float Room = Wide - 10.0f * Scale;
		const float Needed = TextWidth(Text, Most, true);
		return Needed > Room && Needed > 0.0f ? FMath::Max(Most * Room / Needed, 9.0f * Scale) : Most;
	};
	if (State == 3)
	{
		// A key that cannot change: no cap, a dashed outline.
		const float Dash = FMath::Max(FMath::RoundToFloat(6.0f * Scale), 3.0f);
		const FLinearColor Line = HudFaded(BoneTextColor, 0.55f);
		for (float X = Left; X < Left + Wide; X += Dash * 2.0f)
		{
			const float Length = FMath::Min(Dash, Left + Wide - X);
			DrawRect(Line, X, Top, Length, Thick);
			DrawRect(Line, X, Top + Tall - Thick, Length, Thick);
		}
		for (float Y = Top; Y < Top + Tall; Y += Dash * 2.0f)
		{
			const float Length = FMath::Min(Dash, Top + Tall - Y);
			DrawRect(Line, Left, Y, Thick, Length);
			DrawRect(Line, Left + Wide - Thick, Y, Thick, Length);
		}
		DrawSharpText(Key, HudBoneDimColor, Left + Wide * 0.5f, Top + 12.0f * Scale, 19.0f * Scale, true, true);
		return;
	}
	if (State == 2)
	{
		// Waiting for its new key: royal, its border beating once a second.
		const float Beat = 0.55f + 0.45f * FMath::Abs(FMath::Sin(static_cast<float>(FApp::GetCurrentTime()) * PI));
		DrawRect(RoyalColor, Left, Top, Wide, Tall);
		DrawFrame(Box, Thick, HudFaded(RoyalHiColor, Beat));
		DrawFrame(FBox2D(Box.Min - FVector2D(Thick * 2.0f, Thick * 2.0f), Box.Max + FVector2D(Thick * 2.0f, Thick * 2.0f)), Thick, HudFaded(RoyalHiColor, Beat * 0.6f));
		const FString Ask(Wide >= 100.0f * Scale ? TEXT("Press a key") : TEXT("..."));
		const float Size = Fitted(Ask, 16.0f * Scale);
		DrawSharpText(Ask, FLinearColor::White, Left + Wide * 0.5f, Top + (Tall - Size) * 0.5f - Size * 0.1f, Size, true, true);
		return;
	}
	DrawRect(BoneTextColor, Left, Top, Wide, Tall);
	if (State == 1)
	{
		DrawFrame(Box, Thick, RoyalHiColor);
		DrawFrame(FBox2D(Box.Min - FVector2D(Thick, Thick), Box.Max + FVector2D(Thick, Thick)), Thick, HudFaded(RoyalHiColor, 0.6f));
	}
	const float Size = Fitted(Key, 19.0f * Scale);
	DrawSharpText(Key, HudKeyTextColor, Left + Wide * 0.5f, Top + (Tall - Size) * 0.5f - Size * 0.1f, Size, true, true, 0.0f, false);
}

void AWizardHUD::DrawSettingsSlider(const FBox2D& Track, float Share, bool bEnabled, float Scale, bool bGreyFill)
{
	const float Left = static_cast<float>(Track.Min.X);
	const float Right = static_cast<float>(Track.Max.X);
	const float Half = FMath::RoundToFloat(SettingsHandleWidth * 0.5f * Scale);
	const float BarTall = FMath::Max(FMath::RoundToFloat(10.0f * Scale), 4.0f);
	const float BarTop = FMath::RoundToFloat(static_cast<float>(Track.GetCenter().Y) - BarTall * 0.5f);
	const float HandleX = FMath::RoundToFloat(FMath::Lerp(Left, Right, FMath::Clamp(Share, 0.0f, 1.0f)));
	DrawRect(SettingsTrackColor, Left - Half, BarTop, Right - Left + Half * 2.0f, BarTall);
	if (bEnabled)
	{
		// Muted, the fill is grey: the volume is kept, and not heard.
		DrawRect(bGreyFill ? FLinearColor::FromSRGBColor(FColor(0x55, 0x52, 0x4A)) : BoneTextColor, Left - Half, BarTop, HandleX - Left + Half, BarTall);
		DrawFrame(FBox2D(FVector2D(Left - Half, BarTop), FVector2D(Right + Half, BarTop + BarTall)), 1.0f, HudFaded(BoneTextColor, 0.3f));
	}
	const FBox2D Handle(FVector2D(HandleX - Half, Track.Min.Y), FVector2D(HandleX + Half, Track.Max.Y));
	DrawRect(bEnabled ? RoyalColor : OffFillColor, Handle.Min.X, Handle.Min.Y, Handle.GetSize().X, Handle.GetSize().Y);
	DrawFrame(Handle, FMath::Max(FMath::RoundToFloat(2.0f * Scale), 1.0f), bEnabled ? RoyalHiColor : OffBorderColor);
}

void AWizardHUD::DrawSettingsChoices(const FBox2D* Boxes, const TCHAR* const* Labels, int32 Count, int32 Chosen, bool bEnabled, float Scale)
{
	const float Thick = FMath::Max(FMath::RoundToFloat(2.0f * Scale), 1.0f);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const FBox2D& Box = Boxes[Index];
		const bool bChosen = bEnabled && Index == Chosen;
		const float TextY = static_cast<float>(Box.Min.Y) + (static_cast<float>(Box.GetSize().Y) - 20.0f * Scale) * 0.5f - 2.0f * Scale;
		if (bChosen)
		{
			DrawRect(RoyalColor, Box.Min.X, Box.Min.Y, Box.GetSize().X, Box.GetSize().Y);
			DrawFrame(Box, Thick, RoyalHiColor);
		}
		else if (!bEnabled)
		{
			DrawRect(OffFillColor, Box.Min.X, Box.Min.Y, Box.GetSize().X, Box.GetSize().Y);
		}
		DrawSharpText(Labels[Index], bChosen ? FLinearColor::White : bEnabled ? HudBoneDimColor : OffTextColor, Box.GetCenter().X, TextY, 20.0f * Scale, bChosen, true);
		if (Index > 0 && !bChosen && Index - 1 != Chosen)
		{
			DrawRect(bEnabled ? HudFaded(BoneTextColor, 0.2f) : OffBorderColor, Box.Min.X, Box.Min.Y + Thick, 1.0f, Box.GetSize().Y - Thick * 2.0f);
		}
	}
	// One border around them all, under the chosen one's own.
	const FBox2D All(Boxes[0].Min, Boxes[Count - 1].Max);
	const FLinearColor Border = bEnabled ? HudFaded(BoneTextColor, 0.3f) : OffBorderColor;
	DrawRect(Border, All.Min.X, All.Min.Y - Thick, All.GetSize().X, Thick);
	DrawRect(Border, All.Min.X, All.Max.Y, All.GetSize().X, Thick);
	DrawRect(Border, All.Min.X - Thick, All.Min.Y - Thick, Thick, All.GetSize().Y + Thick * 2.0f);
	DrawRect(Border, All.Max.X, All.Min.Y - Thick, Thick, All.GetSize().Y + Thick * 2.0f);
}

void AWizardHUD::DrawSettingsControls(const AWizardPawn& Wizard, const FBox2D& Panel, float Width, float Height, float Scale)
{
	const FWizardKeyMap& Keys = WizardKeys();
	const TArray<FName>& Changeable = FWizardKeyMap::Changeable();
	const FHudKeyLabels K;
	const FString CommandHint = FString::Printf(TEXT("Then wheel: how many \u00B7 %s: kind \u00B7 %s: send"), *K.Spear, *K.Bolt);
	struct FControl
	{
		// The first of its keys in FWizardKeyMap::Changeable, and how many it has; no key of its own when Count is 0 (Aim).
		int32 First;
		int32 Count;
		UTexture2D* Icon;
		bool bRecall;
		const TCHAR* Action;
		const TCHAR* Hint;
	};
	const FControl LeftRows[] = {
		{ 0, 4, nullptr, false, TEXT("Move"), TEXT("Up \u00B7 Left \u00B7 Down \u00B7 Right") },
		{ 0, 0, nullptr, false, TEXT("Aim"), TEXT("The wizard faces the cursor") },
		{ 4, 1, nullptr, false, TEXT("Bolt"), nullptr },
		{ 5, 1, nullptr, false, TEXT("Use the shrine"), nullptr },
		{ 6, 1, nullptr, false, TEXT("Pause"), nullptr },
	};
	const FControl RightRows[] = {
		{ 7, 1, SummonIcon, false, TEXT("Summon the Afterlife"), nullptr },
		{ 8, 1, SpearIcon, false, TEXT("Spear of Sacrifice"), nullptr },
		{ 9, 1, BlinkIcon, false, TEXT("Blink"), nullptr },
		{ 10, 1, CommandIcon, false, TEXT("Command the army"), *CommandHint },
		{ 11, 1, nullptr, true, TEXT("Recall the army"), nullptr },
	};
	float MouseX = -1.0f;
	float MouseY = -1.0f;
	const bool bMouse = PlayerOwner && PlayerOwner->GetMousePosition(MouseX, MouseY);
	const FName Capturing = Wizard.GetCapturingAction();
	const float Left = static_cast<float>(Panel.Min.X) + SettingsBodySide * Scale;
	const float Top = static_cast<float>(Panel.Min.Y) + SettingsBodyTop * Scale;
	const float ColumnGap = SettingsColumnGap * Scale;
	const float ColumnWidth = (static_cast<float>(Panel.GetSize().X) - SettingsBodySide * 2.0f * Scale - ColumnGap) * 0.5f;
	const float Pitch = SettingsControlsPitch * Scale;
	const float IconSize = FMath::RoundToFloat(44.0f * Scale);
	const auto DrawColumn = [&](const FControl* Rows, int32 Count, float X)
	{
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FControl& Control = Rows[Index];
			const float Y = Top + Index * Pitch;
			if (Control.Icon || Control.bRecall)
			{
				DrawRect(SettingsWellColor, X, Y, IconSize, IconSize);
				if (Control.Icon)
				{
					const float Margin = (1.0f - HudIconCrop) * 0.5f;
					DrawTexture(Control.Icon, X, Y, IconSize, IconSize, Margin, Margin, HudIconCrop, HudIconCrop);
				}
				else
				{
					DrawRecallArrow(X, Y, IconSize, Scale * 0.65f);
				}
				DrawFrame(FBox2D(FVector2D(X, Y), FVector2D(X + IconSize, Y + IconSize)), 1.0f, HudFaded(BoneTextColor, 0.35f));
			}
			const float TextX = X + IconSize + 16.0f * Scale;
			DrawSharpText(Control.Action, BoneTextColor, TextX, Y + (Control.Hint ? 0.0f : 9.0f) * Scale, 22.0f * Scale, false);
			if (Control.Hint)
			{
				DrawSharpText(Control.Hint, DetailGreyColor, TextX, Y + 27.0f * Scale, 16.0f * Scale, false);
			}
			if (Control.Count == 0)
			{
				DrawSettingsKeyBox(SettingsFixedKeyBox(Width, Height), TEXT("Mouse"), 3, Scale);
			}
			for (int32 Key = Control.First; Key < Control.First + Control.Count; ++Key)
			{
				const FBox2D Box = SettingsKeyBox(Key, Width, Height);
				const bool bWaiting = Changeable[Key] == Capturing;
				const bool bOver = Capturing.IsNone() && bMouse && Box.IsInside(FVector2D(MouseX, MouseY));
				DrawSettingsKeyBox(Box, Keys.GetLabel(Changeable[Key]), bWaiting ? 2 : bOver ? 1 : 0, Scale);
			}
			DrawRect(HudFaded(BoneTextColor, 0.08f), X, Y + 53.0f * Scale, ColumnWidth, 1.0f);
		}
	};
	DrawColumn(LeftRows, UE_ARRAY_COUNT(LeftRows), Left);
	DrawColumn(RightRows, UE_ARRAY_COUNT(RightRows), Left + ColumnWidth + ColumnGap);
	// What the last change did, then how changing works.
	const float Under = Top + 5.0f * Pitch;
	if (!Wizard.GetKeyMessage().IsEmpty())
	{
		DrawSharpText(Wizard.GetKeyMessage(), Capturing.IsNone() ? BoneTextColor : RoyalHiColor, Left, Under + 6.0f * Scale, 19.0f * Scale, false);
	}
	DrawSharpText(TEXT("Click a key, then press the new key or mouse button. Esc cancels. If the key is already taken, the two actions swap."), DetailGreyColor, Left, Under + 42.0f * Scale, 17.0f * Scale, false);
	DrawSharpText(FString::Printf(TEXT("Fixed: Esc (the browser keeps it to leave full screen), %s to restart, Enter to march, and the wheel inside Command."), *K.Restart), DetailGreyColor, Left, Under + 68.0f * Scale, 17.0f * Scale, false);
}

void AWizardHUD::DrawSettingsSound(const FWizardSettings& Settings, float Width, float Height, float Scale)
{
	const TCHAR* Names[] = { TEXT("Master"), TEXT("Music"), TEXT("Effects"), TEXT("Interface") };
	const TCHAR* Hints[] = { TEXT("Everything"), TEXT("The score"), TEXT("Spells, bones, battle"), TEXT("Buttons and the shrine") };
	for (int32 Channel = 0; Channel < FWizardSettings::Channels; ++Channel)
	{
		const FBox2D Row = SettingsRowBox(Channel, Width, Height);
		const float Left = static_cast<float>(Row.Min.X);
		const float Right = static_cast<float>(Row.Max.X);
		const float Top = static_cast<float>(Row.Min.Y);
		const bool bMuted = Settings.Muted[Channel] != 0;
		DrawSharpText(Names[Channel], BoneTextColor, Left, Top + 14.0f * Scale, 22.0f * Scale, false);
		DrawSharpText(Hints[Channel], DetailGreyColor, Left, Top + 42.0f * Scale, 16.0f * Scale, false);
		DrawRect(HudFaded(BoneTextColor, 0.08f), Left, Row.Max.Y - 1.0f, Right - Left, 1.0f);

		// The mute button: a speaker with two waves, or a red cross over the waves when muted.
		const FBox2D Mute = SettingsMuteBox(Channel, Width, Height);
		const float MuteLeft = static_cast<float>(Mute.Min.X);
		const float MuteTop = static_cast<float>(Mute.Min.Y);
		const float Size = static_cast<float>(Mute.GetSize().X);
		DrawRect(SettingsWellColor, MuteLeft, MuteTop, Size, Size);
		DrawFrame(Mute, 1.0f, bMuted ? CutColor : HudFaded(BoneTextColor, 0.35f));
		DrawRect(BoneTextColor, MuteLeft + Size * 0.2f, MuteTop + Size * 0.39f, Size * 0.14f, Size * 0.22f);
		DrawRect(BoneTextColor, MuteLeft + Size * 0.34f, MuteTop + Size * 0.33f, Size * 0.08f, Size * 0.34f);
		DrawRect(BoneTextColor, MuteLeft + Size * 0.42f, MuteTop + Size * 0.26f, Size * 0.08f, Size * 0.48f);
		const float Stroke = FMath::Max(2.0f * Scale, 1.5f);
		if (bMuted)
		{
			DrawLine(MuteLeft + Size * 0.6f, MuteTop + Size * 0.36f, MuteLeft + Size * 0.84f, MuteTop + Size * 0.64f, CutColor, Stroke);
			DrawLine(MuteLeft + Size * 0.84f, MuteTop + Size * 0.36f, MuteLeft + Size * 0.6f, MuteTop + Size * 0.64f, CutColor, Stroke);
		}
		else
		{
			DrawRect(BoneTextColor, MuteLeft + Size * 0.6f, MuteTop + Size * 0.38f, Stroke, Size * 0.24f);
			DrawRect(BoneTextColor, MuteLeft + Size * 0.72f, MuteTop + Size * 0.3f, Stroke, Size * 0.4f);
		}

		DrawSettingsSlider(SettingsVolumeBox(Channel, Width, Height), Settings.Volume[Channel] / 100.0f, true, Scale, bMuted);
		const FString Value = bMuted ? FString(TEXT("Off")) : FString::FromInt(Settings.Volume[Channel]);
		DrawSharpText(Value, BoneTextColor, Right - TextWidth(Value, 24.0f * Scale, true), Top + 25.0f * Scale, 24.0f * Scale, true);
	}
	const FBox2D Row = SettingsRowBox(FWizardSettings::Channels, Width, Height);
	DrawSharpText(TEXT("Sound in a background tab"), BoneTextColor, Row.Min.X, static_cast<float>(Row.Min.Y) + 14.0f * Scale, 22.0f * Scale, false);
	DrawSharpText(TEXT("When the game is not the tab in front"), DetailGreyColor, Row.Min.X, static_cast<float>(Row.Min.Y) + 42.0f * Scale, 16.0f * Scale, false);
	const TCHAR* const Labels[] = { TEXT("Keep playing"), TEXT("Mute") };
	const FBox2D Boxes[] = { SettingsBackgroundBox(0, Width, Height), SettingsBackgroundBox(1, Width, Height) };
	DrawSettingsChoices(Boxes, Labels, 2, Settings.BackgroundMute != 0 ? 1 : 0, true, Scale);
	DrawMenuButton(SettingsTestSoundBox(Width, Height), TEXT("Play a test sound"), EWizardButtonLook::Secondary, 20.0f * Scale, Scale);
}

void AWizardHUD::DrawSettingsDisplay(const AWizardPawn& Wizard, const FWizardSettings& Settings, float Width, float Height, float Scale)
{
	struct FRow
	{
		const TCHAR* Name;
		const TCHAR* Hint;
		const TCHAR* Labels[3];
	};
	const FRow Rows[] = {
		{ TEXT("Quality"), TEXT("Lower it if the game stutters"), { TEXT("Low"), TEXT("Medium"), TEXT("High") } },
		{ TEXT("Render scale"), TEXT("Lower it if the game stutters"), { nullptr, nullptr, nullptr } },
		{ TEXT("HUD and menu size"), TEXT("On top of the size set by the window"), { nullptr, nullptr, nullptr } },
		{ TEXT("Danger edges"), TEXT("The red edges as Overwhelm rises"), { nullptr, nullptr, nullptr } },
		{ TEXT("Screen shake"), TEXT("Slams, blasts and the commander's blows"), { TEXT("Full"), TEXT("Light"), TEXT("Off") } },
		{ TEXT("Show frame rate"), TEXT("Top-right corner"), { TEXT("Off"), TEXT("On"), nullptr } },
		{ TEXT("Full screen"), TEXT("Esc leaves it"), { nullptr, nullptr, nullptr } },
	};
	for (int32 Row = 0; Row < static_cast<int32>(EWizardDisplayRow::Count); ++Row)
	{
		const EWizardDisplayRow DisplayRow = static_cast<EWizardDisplayRow>(Row);
		const FBox2D Box = SettingsRowBox(Row, Width, Height);
		const float Left = static_cast<float>(Box.Min.X);
		const float Right = static_cast<float>(Box.Max.X);
		const float Top = static_cast<float>(Box.Min.Y);
		const bool bEnabled = FWizardSettings::IsEnabled(DisplayRow);
		DrawSharpText(Rows[Row].Name, BoneTextColor, Left, Top + 14.0f * Scale, 22.0f * Scale, false);
		DrawSharpText(Rows[Row].Hint, DetailGreyColor, Left, Top + 42.0f * Scale, 16.0f * Scale, false);
		DrawRect(HudFaded(BoneTextColor, 0.08f), Left, Box.Max.Y - 1.0f, Right - Left, 1.0f);
		if (FWizardSettings::IsSlider(DisplayRow))
		{
			int32 Min = 0;
			int32 Max = 0;
			int32 Step = 1;
			FWizardSettings::Range(DisplayRow, Min, Max, Step);
			// While the size slider is held, the value under the mouse is shown; the screen takes it when it is let go.
			const int32 Value = Wizard.IsSettingsDragging(DisplayRow) ? Wizard.GetSettingsDragValue() : Settings.Get(DisplayRow);
			const FBox2D Track = SettingsSliderBox(Row, Width, Height);
			DrawSettingsSlider(Track, static_cast<float>(Value - Min) / FMath::Max(Max - Min, 1), true, Scale);
			const FString Text = DisplayRow == EWizardDisplayRow::DangerEdges && Value == 0 ? FString(TEXT("Off")) : FString::Printf(TEXT("%d %%"), Value);
			const float ValueRight = static_cast<float>(Track.Max.X) + (SettingsHandleWidth * 0.5f + 20.0f + SettingsValueWidth) * Scale;
			DrawSharpText(Text, BoneTextColor, ValueRight - TextWidth(Text, 24.0f * Scale, true), Top + 25.0f * Scale, 24.0f * Scale, true);
			if (DisplayRow == EWizardDisplayRow::HudSize)
			{
				// A piece of the HUD at the size chosen, to judge it by.
				const float Sample = HudWindowScale(Width, Height) * Value / 100.0f;
				const float SampleX = ValueRight + 34.0f * Scale;
				DrawTitleText(TEXT("ARMY"), HudBoneDimColor, SampleX, Top + 16.0f * Scale, 14.0f * Sample, false, 0.18f);
				DrawSharpText(TEXT("15"), BoneTextColor, SampleX + 62.0f * Sample, Top + 40.0f * Scale - 20.0f * Sample, 40.0f * Sample, true);
			}
			continue;
		}
		const int32 Count = FWizardSettings::Choices(DisplayRow);
		if (Count > 0)
		{
			FBox2D Boxes[3];
			for (int32 Index = 0; Index < Count; ++Index)
			{
				Boxes[Index] = SettingsChoiceBox(Row, Index, Width, Height);
			}
			DrawSettingsChoices(Boxes, Rows[Row].Labels, Count, bEnabled ? Settings.Get(DisplayRow) : INDEX_NONE, bEnabled, Scale);
			if (!bEnabled)
			{
				DrawSharpText(TEXT("Later"), OffTextColor, static_cast<float>(Boxes[Count - 1].Max.X) + 24.0f * Scale, Top + 28.0f * Scale, 19.0f * Scale, false);
			}
			continue;
		}
		// Full screen: a button, off until the browser can be asked from the click itself.
		const FBox2D First = SettingsChoiceBox(Row, 0, Width, Height);
		const FBox2D Button(First.Min, FVector2D(First.Min.X + FMath::RoundToFloat(230.0f * Scale), First.Max.Y));
		DrawMenuButton(Button, TEXT("Go full screen"), EWizardButtonLook::Off, 20.0f * Scale, Scale);
		DrawSharpText(TEXT("Later"), OffTextColor, static_cast<float>(Button.Max.X) + 24.0f * Scale, Top + 28.0f * Scale, 19.0f * Scale, false);
	}
}

void AWizardHUD::DrawSettings(const AWizardPawn& Wizard, const FWizardSettings& Settings, float Width, float Height)
{
	const float Scale = MenuScale(Width, Height);
	const FWizardSettingsScreen& Screen = Wizard.GetSettingsScreen();
	const FBox2D Panel = SettingsPanelBox(Width, Height);
	const float Left = static_cast<float>(Panel.Min.X);
	const float Top = static_cast<float>(Panel.Min.Y);
	const float Wide = static_cast<float>(Panel.GetSize().X);
	// Over the menu it came from, darkened once more.
	DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.62f), 0.0f, 0.0f, Width, Height);
	// Solid under the panel: the menu behind it must not read through its words.
	DrawRect(HudFaded(PanelColor, 1.0f), Panel.Min.X, Panel.Min.Y, Panel.GetSize().X, Panel.GetSize().Y);
	DrawPanel(Panel, Scale);
	DrawTitleText(TEXT("SETTINGS"), BoneTextColor, Panel.GetCenter().X, Top + 30.0f * Scale, 52.0f * Scale, true, 0.16f);

	float MouseX = -1.0f;
	float MouseY = -1.0f;
	const bool bMouse = PlayerOwner && PlayerOwner->GetMousePosition(MouseX, MouseY);
	const TCHAR* Tabs[] = { TEXT("CONTROLS"), TEXT("SOUND"), TEXT("DISPLAY") };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Tabs); ++Index)
	{
		const FBox2D Box = SettingsTabBox(Index, Width, Height);
		const bool bOpen = static_cast<int32>(Screen.GetTab()) == Index;
		const bool bOver = bMouse && Box.IsInside(FVector2D(MouseX, MouseY));
		DrawTitleText(Tabs[Index], bOpen || bOver ? BoneTextColor : HudBoneDimColor, Box.GetCenter().X, Box.Min.Y + 8.0f * Scale, 22.0f * Scale, true, 0.14f);
		if (bOpen)
		{
			const float Line = FMath::Max(FMath::RoundToFloat(3.0f * Scale), 2.0f);
			DrawRect(RoyalHiColor, Box.Min.X, Box.Max.Y, Box.GetSize().X, Line);
		}
	}
	DrawRect(HudFaded(BoneTextColor, 0.18f), Left, Top + FMath::RoundToFloat((SettingsTabsTop + SettingsTabHeight + 4.0f) * Scale), Wide, 1.0f);

	if (Screen.GetTab() == EWizardSettingsTab::Controls)
	{
		DrawSettingsControls(Wizard, Panel, Width, Height, Scale);
	}
	else if (Screen.GetTab() == EWizardSettingsTab::Sound)
	{
		DrawSettingsSound(Settings, Width, Height, Scale);
	}
	else
	{
		DrawSettingsDisplay(Wizard, Settings, Width, Height, Scale);
	}

	const float FooterTop = static_cast<float>(Panel.Max.Y) - FMath::RoundToFloat(SettingsFooterHeight * Scale);
	DrawRect(HudFaded(BoneTextColor, 0.12f), Left, FooterTop, Wide, 1.0f);
	DrawSharpText(TEXT("Changes apply at once and are kept in this browser."), HudBoneDimColor, Left + 48.0f * Scale, FooterTop + 38.0f * Scale, 17.0f * Scale, false);
	const TCHAR* Resets[] = { TEXT("Reset controls"), TEXT("Reset sound"), TEXT("Reset display") };
	DrawMenuButton(SettingsResetBox(Width, Height), Resets[static_cast<int32>(Screen.GetTab())], EWizardButtonLook::Secondary, 22.0f * Scale, Scale);
	DrawMenuButton(SettingsBackBox(Width, Height), TEXT("Back"), EWizardButtonLook::Royal, 22.0f * Scale, Scale);
}

void AWizardHUD::DrawEdges(float Width, float Height)
{
	// Each edge is a stack of strips that fade toward the middle of the screen, and grows thicker as it fills.
	const auto DrawEdge = [this, Width, Height](float Shown, bool bVertical, bool bFarSide)
	{
		// The player's "Danger edges" setting thins them, down to none.
		const float Strength = Shown * EdgeFactor;
		if (Strength <= 0.01f)
		{
			return;
		}
		const float Thickness = Height * (0.08f + 0.2f * Strength);
		const float Strip = Thickness / EdgeStrips;
		for (int32 Index = 0; Index < EdgeStrips; ++Index)
		{
			const float Fade = 1.0f - static_cast<float>(Index) / EdgeStrips;
			FLinearColor Color = EdgeColor;
			Color.A = FMath::Clamp(Strength * 1.3f, 0.0f, 1.0f) * Fade * Fade;
			const float Offset = Index * Strip;
			if (bVertical)
			{
				DrawRect(Color, bFarSide ? Width - Offset - Strip : Offset, 0.0f, Strip, Height);
			}
			else
			{
				DrawRect(Color, 0.0f, bFarSide ? Height - Offset - Strip : Offset, Width, Strip);
			}
		}
	};

	DrawEdge(ShownEdges.Left, true, false);
	DrawEdge(ShownEdges.Right, true, true);
	DrawEdge(ShownEdges.Top, false, false);
	DrawEdge(ShownEdges.Bottom, false, true);
}

void AWizardHUD::DrawRings(const UCrowdSubsystem& Crowd, const AWizardPawn& Wizard)
{
	// The roam circle (green) and the leash (yellow), drawn on the ground around the wizard.
	const FVector2D Middle = Wizard.GetPosition2D();
	const float GroundZ = Wizard.GetGroundZ() + 4.0f;
	const auto DrawRing = [this, Middle, GroundZ](float Radius, const FLinearColor& Color)
	{
		FVector Previous = Canvas->Project(FVector(Middle.X + Radius, Middle.Y, GroundZ));
		for (int32 Index = 1; Index <= RingSegments; ++Index)
		{
			const double Angle = UE_DOUBLE_TWO_PI * Index / RingSegments;
			const FVector Next = Canvas->Project(FVector(Middle.X + FMath::Cos(Angle) * Radius, Middle.Y + FMath::Sin(Angle) * Radius, GroundZ));
			// Z is zero or less for a point behind the camera.
			if (Previous.Z > 0.0 && Next.Z > 0.0)
			{
				DrawLine(Previous.X, Previous.Y, Next.X, Next.Y, Color, 1.5f);
			}
			Previous = Next;
		}
	};
	DrawRing(Crowd.GetTuning().RoamRadius, RoamRingColor);
	DrawRing(Crowd.GetTuning().LeashRadius, LeashRingColor);
	DrawRing(Crowd.GetTuning().PresenceRadius, PresenceRingColor);
	DrawRing(Crowd.GetTuning().BonePickupRadius, PickupRingColor);
}

void AWizardHUD::DrawGroundRing(FVector2D Middle, float Radius, float GroundZ, const FLinearColor& Color, float Thickness)
{
	FVector Previous = Canvas->Project(FVector(Middle.X + Radius, Middle.Y, GroundZ));
	for (int32 Index = 1; Index <= RingSegments; ++Index)
	{
		const double Angle = UE_DOUBLE_TWO_PI * Index / RingSegments;
		const FVector Next = Canvas->Project(FVector(Middle.X + FMath::Cos(Angle) * Radius, Middle.Y + FMath::Sin(Angle) * Radius, GroundZ));
		// Z is zero or less for a point behind the camera.
		if (Previous.Z > 0.0 && Next.Z > 0.0)
		{
			DrawLine(Previous.X, Previous.Y, Next.X, Next.Y, Color, Thickness);
		}
		Previous = Next;
	}
}

void AWizardHUD::DrawCommand(const UCrowdSubsystem& Crowd, const AWizardPawn& Wizard, float Width, float Height, float Scale)
{
	const float GroundZ = Wizard.GetGroundZ() + 4.0f;
	const FCrowdTuning& Tuning = Crowd.GetTuning();

	// Every place that is being held: the circle is how close the wizard has to come for them to rejoin.
	TArray<FVector2D> Points;
	Crowd.GetSim().GetCommandPoints(Points);
	for (const FVector2D& Point : Points)
	{
		DrawGroundRing(Point, Tuning.CommandRejoinRadius, GroundZ, CommandHeldColor, 1.5f);
	}

	if (!Crowd.IsCommandMode())
	{
		return;
	}
	// His presence: outside this circle the army is weaker.
	DrawGroundRing(Wizard.GetPosition2D(), Tuning.PresenceRadius, GroundZ, PresenceRingColor, 1.5f);
	// Where a send would go: the point under the mouse, as wide as the group will spread.
	const FVector2D Aim = Wizard.GetAimPoint2D();
	DrawGroundRing(Aim, Crowd.GetCommandRadius(), GroundZ, CommandColor, 2.5f);

	// Beside the cursor: how many will go, to its right, and under that the kind (an image later).
	float CursorX = 0.0f;
	float CursorY = 0.0f;
	if (!PlayerOwner || !PlayerOwner->GetMousePosition(CursorX, CursorY))
	{
		const FVector OnScreen = Canvas->Project(FVector(Aim.X, Aim.Y, GroundZ));
		CursorX = static_cast<float>(OnScreen.X);
		CursorY = static_cast<float>(OnScreen.Y);
	}
	const float TextX = CursorX + CommandCursorGap * Scale;
	DrawSharpText(FString::FromInt(Crowd.GetCommandCount()), CommandColor, TextX, CursorY - CommandCountHeight * 0.6f * Scale, CommandCountHeight * Scale, true);
	const FString Kind = Crowd.GetCommandKindName();
	DrawSharpText(Kind == TEXT("Any") ? FString(TEXT("Any kind")) : Kind, FLinearColor::White, TextX, CursorY + CommandCountHeight * 0.6f * Scale,
		CommandKindHeight * Scale, false);

	DrawSharpText(TEXT("Mouse wheel: how many    Right mouse: kind    Left mouse: send    F: cancel"), FLinearColor::White,
		Width * 0.5f, Height * 0.86f, 22.0f * Scale, false, true);
}

void AWizardHUD::DrawHealthBars(const UCrowdSubsystem& Crowd, const AWizardPawn& Wizard, float Scale)
{
	// Only an enemy that was hit in the last few seconds has a bar, so most frames draw none or a handful.
	// Each bar is two flat rectangles: no widgets, no components, nothing that ticks.
	const float GruntBarWidth = FMath::Max(FMath::RoundToFloat(46.0f * Scale), 26.0f);
	const float BarHeight = FMath::Max(FMath::RoundToFloat(6.0f * Scale), 4.0f);
	const float GroundZ = Wizard.GetGroundZ();
	for (const FCrowdUnit& Unit : Crowd.GetSim().GetUnits())
	{
		if (Unit.HurtTimer <= 0.0f || Unit.Team != ECrowdTeam::Enemy || Unit.MaxHealth <= 0.0f
			|| Crowd.GetSim().IsCloaked(Unit, Wizard.GetPosition2D()))
		{
			continue;
		}
		const bool bBrute = Unit.Kind == ECrowdKind::Brute;
		const bool bCommander = Unit.Kind == ECrowdKind::Commander;
		const float WidthScale = bCommander ? CommanderBarWidthScale : bBrute ? BruteBarWidthScale : 1.0f;
		const float WorldHeight = Crowd.GetUnitHeight(Unit.Kind) + HealthBarGap;
		const float BarWidth = FMath::RoundToFloat(GruntBarWidth * WidthScale);
		const FVector Screen = Canvas->Project(FVector(Unit.Position.X, Unit.Position.Y, GroundZ + WorldHeight));
		if (Screen.Z <= 0.0 || Screen.X < -BarWidth || Screen.X > Canvas->ClipX + BarWidth || Screen.Y < -BarHeight || Screen.Y > Canvas->ClipY + BarHeight)
		{
			continue;
		}
		const float Alpha = HealthBarAlpha(Unit.HurtTimer);
		const float X = FMath::RoundToFloat(Screen.X - BarWidth * 0.5f);
		const float Y = FMath::RoundToFloat(Screen.Y - BarHeight);
		FLinearColor Back = HealthBarBackColor;
		Back.A *= Alpha;
		FLinearColor Fill = HealthBarFillColor;
		Fill.A *= Alpha;
		DrawRect(Back, X - 1.0f, Y - 1.0f, BarWidth + 2.0f, BarHeight + 2.0f);
		DrawRect(Fill, X, Y, BarWidth * FMath::Clamp(Unit.Health / Unit.MaxHealth, 0.0f, 1.0f), BarHeight);
	}
}

void AWizardHUD::DrawDebug(float Width, float Scale, float BarRight, float BelowBar)
{
	const UCrowdSubsystem* Crowd = GetWorld()->GetSubsystem<UCrowdSubsystem>();
	const AWizardPawn* Wizard = Crowd->GetWizard();

	// Smoothed so the number can be read.
	SmoothedFrameMs = FMath::Lerp(SmoothedFrameMs, RenderDelta * 1000.0f, 0.1f);

	const FString Lines[] = {
		FString::Printf(TEXT("Frame: %.1f ms (%.0f fps)"), SmoothedFrameMs, SmoothedFrameMs > 0.0f ? 1000.0f / SmoothedFrameMs : 0.0f),
		FString::Printf(TEXT("Screen: %.0f x %.0f"), Canvas->ClipX, Canvas->ClipY),
		FString::Printf(TEXT("Units drawn: %d"), Crowd->GetUnitsDrawn()),
		FString::Printf(TEXT("Army: %d   Enemies: %d"), Crowd->GetArmyCount(), Crowd->GetEnemyCount()),
		FString::Printf(TEXT("Raised: %d   Killed: %d"), Crowd->GetTotalRaised(), Crowd->GetTotalKilled()),
		FString::Printf(TEXT("Bones: %d held, %d on ground   Groups: %d"), Crowd->GetBonesHeld(), Crowd->GetBonesOnGround(), Crowd->GetGroupCount()),
		FString::Printf(TEXT("Bone worth: %d (army counts %d)"), Crowd->GetBoneWorth(), Crowd->GetArmyBoneValue()),
		FString::Printf(TEXT("At posts: %d   Patrols sent: %d"), Crowd->GetAsleepCount(), Crowd->GetPatrolsSent()),
		FString::Printf(TEXT("Overwhelm: %.0f%%   Time: x%.2f"), Crowd->GetOverwhelm() * 100.0f, Crowd->GetTimeScale()),
		FString::Printf(TEXT("Camera: %s"), Wizard ? *AWizardPawn::GetCameraModeName(Wizard->GetCameraMode()) : TEXT("none")),
		FString::Printf(TEXT("Waves: %s   God mode: %s"), Crowd->AreWavesEnabled() ? TEXT("on") : TEXT("off"), Crowd->IsGodMode() ? TEXT("on") : TEXT("off")),
		FString::Printf(TEXT("Walls: %d   Roam: %.0f   Leash: %.0f"), Crowd->GetObstacleCount(), Crowd->GetTuning().RoamRadius, Crowd->GetTuning().LeashRadius),
		FString::Printf(TEXT("Blink: %s %.1f s   cast %d"), *Crowd->GetBlinkStateName(), Crowd->GetBlinkCooldown(), Crowd->GetBlinksCast()),
		FString::Printf(TEXT("Heights: %.0f  low %.0f  brute %.0f  cmdr %.0f"), Wizard ? Wizard->GetModelHeight() : 0.0f,
			Crowd->GetUnitHeight(ECrowdKind::Grunt), Crowd->GetUnitHeight(ECrowdKind::Brute), Crowd->GetUnitHeight(ECrowdKind::Commander)),
		FString::Printf(TEXT("Sent: %d (%d hunting)   Arrows: %d (%d hit)"), Crowd->GetCommandedCount(), Crowd->GetHuntingCount(), Crowd->GetArrowCount(), Crowd->GetArrowHits()),
		FString::Printf(TEXT("Assassins: %d   Bolt every %.2f s"), Crowd->GetAssassinsSent(), Crowd->GetTuning().BoltInterval),
		FString::Printf(TEXT("Away from him: %d   Bones on screen: %d"), Crowd->GetAwayCount(), Crowd->GetBonesInView()),
		FString::Printf(TEXT("Models drawn: %d (%s)"), Crowd->GetArmyModelsDrawn(), Crowd->IsArmyModelShown() ? TEXT("on") : TEXT("off")),
		FString::Printf(TEXT("Sacrificed: %d   See-through: %.2f"), Crowd->GetSacrificed(), Crowd->GetSeeThroughRadius()),
		FString::Printf(TEXT("Slams: %d (%d hit)   Swings ready: %d"), Crowd->GetSlamsLanded(), Crowd->GetSlamHits(), Crowd->GetSwingsReady()),
		FString::Printf(TEXT("Run: %.0f s"), Crowd->GetRunSeconds()),
		Crowd->GetModelCostLine(),
		TEXT("Test keys: J models  K 40 grunts  L 30 skeletons  M test"),
	};

	const float TextHeight = FMath::Max(DebugTextHeight * Scale, SmallestTextHeight);
	const float LineHeight = FMath::RoundToFloat(TextHeight * 1.35f);
	const float PanelWidth = FMath::RoundToFloat(TextHeight * 25.0f);
	const float Padding = FMath::RoundToFloat(10.0f * Scale);
	const float X = Width - PanelWidth - Padding;
	float Y = FMath::RoundToFloat(22.0f * Scale);
	if (X - Padding < BarRight + Padding)
	{
		// A narrow screen: the panel would cover the Overwhelm bar, so it goes under it.
		Y = FMath::RoundToFloat(BelowBar + Padding);
	}
	DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.45f), X - Padding, Y - Padding * 0.7f, PanelWidth + Padding, UE_ARRAY_COUNT(Lines) * LineHeight + Padding * 1.4f);
	for (const FString& Line : Lines)
	{
		DrawSharpText(Line, DebugTextColor, X, Y, TextHeight, false);
		Y += LineHeight;
	}
}
