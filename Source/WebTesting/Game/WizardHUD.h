#pragma once

#include "CoreMinimal.h"
#include "Game/WizardSettings.h"
#include "GameFramework/HUD.h"
#include "WizardHUD.generated.h"

class AWizardPawn;
class UCrowdSubsystem;
class UFont;
class UFontFace;
class UTexture2D;

/** How a menu button is drawn: the main choice, a second choice, one the army pays for, or one that cannot be used. */
enum class EWizardButtonLook : uint8
{
	Royal,
	Secondary,
	Sacrifice,
	Off,
};

/** What a click on the settings screen lands on. */
enum class EWizardSettingsHitKind : uint8
{
	None,
	Tab,
	Back,
	Reset,
	Choice,
	Slider,
	// The Controls tab: a control's key (Row is its place in FWizardKeyMap::Changeable).
	Key,
	// The Sound tab: a channel's mute button or volume slider (Row is the channel), the background choice (Index), the test button.
	Mute,
	Volume,
	Background,
	TestSound,
};

/** Row is the Display tab's row (or the tab, the key or the sound channel); Index is the choice in a row of choices. */
struct FWizardSettingsHit
{
	EWizardSettingsHitKind Kind = EWizardSettingsHitKind::None;
	int32 Row = 0;
	int32 Index = 0;
};

/** How strongly each screen edge shows the Overwhelm pressure, 0 to 1. */
struct FWizardEdgeWeights
{
	float Left = 0.0f;
	float Right = 0.0f;
	float Top = 0.0f;
	float Bottom = 0.0f;
};

/** The final cut's ledger: of the skeletons raised, who stands at the gate, who was sacrificed, who fell. */
struct FWizardLedger
{
	int32 Kept = 0;
	int32 Cut = 0;
	int32 Fell = 0;
	// Each part's share of the bar; they add up to 1, or to 0 when there is nobody to count.
	float KeptShare = 0.0f;
	float CutShare = 0.0f;
	float FellShare = 0.0f;
};

/** Canvas HUD: army count, Overwhelm bar, the edge that leans in from the pressure side, and the debug overlay. */
UCLASS()
class AWizardHUD : public AHUD
{
	GENERATED_BODY()

public:
	AWizardHUD();

	/** Maps a pressure direction in the world to screen edges, through the camera yaw. */
	static FWizardEdgeWeights EdgeWeights(FVector2D WorldDirection, float CameraYawDegrees);

	/**
	 * How hard each screen edge closes in, from the enemies on the wizard: each presses on the side it is most on, and
	 * the busiest side is 1. Surrounded evenly, every side is 1.
	 */
	static FWizardEdgeWeights SideWeights(const TArray<FVector2D>& TouchingOffsets, float CameraYawDegrees);

	/**
	 * How much the HUD is scaled for a screen size: 1 at 1920x1080, never below MinLayoutScale so text stays readable,
	 * times the player's "HUD and menu size" setting.
	 */
	static float LayoutScale(float Width, float Height);

	/** The player's "HUD and menu size", as a factor (1 is 100 %). Set by the settings subsystem. */
	static void SetSizeFactor(float Factor);
	static float GetSizeFactor();

	/** LayoutScale for the screens that fill the window's height (pause, settings): the player's size, held to what still fits. */
	static float MenuScale(float Width, float Height);

	/** The settings screen: its panel, tabs (0 Controls, 1 Sound, 2 Display), and the footer's Back and Reset buttons. */
	static FBox2D SettingsPanelBox(float Width, float Height);
	static FBox2D SettingsTabBox(int32 Tab, float Width, float Height);
	static FBox2D SettingsBackBox(float Width, float Height);
	static FBox2D SettingsResetBox(float Width, float Height);

	/** A row of the Display tab, one of its choices, and its slider's track (the handle's middle runs from its left to its right). */
	static FBox2D SettingsRowBox(int32 Row, float Width, float Height);
	static FBox2D SettingsChoiceBox(int32 Row, int32 Index, float Width, float Height);
	static FBox2D SettingsSliderBox(int32 Row, float Width, float Height);

	/** The value a slider row takes with the mouse at this X: on the row's steps, inside its range. */
	static int32 SettingsSliderValue(int32 Row, float X, float Width, float Height);

	/** The Controls tab: the box of a control's key (by its place in FWizardKeyMap::Changeable), and of "Mouse" beside Aim, which cannot change. */
	static FBox2D SettingsKeyBox(int32 Index, float Width, float Height);
	static FBox2D SettingsFixedKeyBox(float Width, float Height);

	/** The Sound tab: a channel's mute button and its volume slider's track, the value at a mouse X, the two background choices and the test button. */
	static FBox2D SettingsMuteBox(int32 Channel, float Width, float Height);
	static FBox2D SettingsVolumeBox(int32 Channel, float Width, float Height);
	static int32 SettingsVolumeValue(int32 Channel, float X, float Width, float Height);
	static FBox2D SettingsBackgroundBox(int32 Index, float Width, float Height);
	static FBox2D SettingsTestSoundBox(float Width, float Height);

	/** What is under a screen point on the settings screen with this tab open. Rows that do nothing yet are never hit. */
	static FWizardSettingsHit SettingsHitAt(FVector2D ScreenPoint, EWizardSettingsTab Tab, float Width, float Height);

	/** The shrine menu's box on a screen of this size: at the right, level with the middle, scaled with the screen, with room around it to click outside. */
	static FBox2D ShrineMenuBox(float Width, float Height);

	/** True when a screen point is inside the shrine menu's box. */
	static bool IsInShrineMenu(FVector2D ScreenPoint, float Width, float Height);

	/** Rows in the shrine menu: one for each thing it sells, in the order of ECrowdShopItem. */
	static constexpr int32 ShrineMenuRows = 6;

	/** Buttons in a row: one for research, which is bought once; three (one, five, as many as can be paid for) for a unit. */
	static int32 ShrineMenuButtonCount(int32 Row);

	/** How many a button buys: 1, 5, or a number so large it means all that can be paid for. */
	static int32 ShrineMenuButtonAmount(int32 Row, int32 Button);

	/** A button of the shrine menu on a screen of this size. */
	static FBox2D ShrineMenuButtonBox(int32 Row, int32 Button, float Width, float Height);

	/** True, with the row and button, when a screen point is on a button of the shrine menu. */
	static bool ShrineMenuButtonAt(FVector2D ScreenPoint, float Width, float Height, int32& OutRow, int32& OutButton);

	/** The start menu's buttons on a screen of this size: 0 "March", 1 "Settings". */
	static FBox2D StartMenuButtonBox(int32 Index, float Width, float Height);

	/** The start menu's button under a screen point, or INDEX_NONE. */
	static int32 StartMenuButtonAt(FVector2D ScreenPoint, float Width, float Height);

	/** The controls panel, under the pause menu's buttons. */
	static FBox2D ControlsPanelBox(float Width, float Height);

	/** The pause menu's buttons on a screen of this size: 0 "Resume", 1 "Settings", 2 "Restart the run". */
	static FBox2D PauseMenuButtonBox(int32 Index, float Width, float Height);

	/** The pause menu's button under a screen point, or INDEX_NONE. */
	static int32 PauseMenuButtonAt(FVector2D ScreenPoint, float Width, float Height);

	/** Kept is the army at the gate, cut the sacrificed, fell the rest of the raised (never below zero). */
	static FWizardLedger FinalCutLedger(int32 Raised, int32 Sacrificed, int32 Army);

	/** The final cut's parts come in one after another: 0 "FINAL CUT", 1 the title, 2 the time, 3 the tally, 4 the buttons. 0 to 1. */
	static float FinalCutPartAlpha(int32 Part, float WonSeconds);

	/** A tally number as it counts up to its value. */
	static int32 FinalCutCount(int32 Value, float WonSeconds);

	/** The final cut's buttons: 0 "March again", 1 "Enter the kingdom" (locked). */
	static FBox2D FinalCutButtonBox(int32 Index, float Width, float Height);
	static int32 FinalCutButtonAt(FVector2D ScreenPoint, float Width, float Height);

	static constexpr int32 FinalCutParts = 5;
	static constexpr int32 FinalCutButtons = 2;

	/** The commander has died, this long ago: how much the banner, and then the line that says where to go, show. 0 to 1. */
	static float GateBannerAlpha(float Seconds);
	static float GateObjectiveAlpha(float Seconds);

	/**
	 * Where the arrow to the gate goes when the gate is out of view: on a box inset from the screen's edges, on the
	 * line from the middle of the screen toward the gate. False when the gate is in view and needs no arrow.
	 */
	static bool GateEdgeArrow(FVector2D Projected, bool bInFront, float Width, float Height, FVector2D& OutPoint, FVector2D& OutDirection);

	static constexpr int32 StartMenuButtons = 2;
	static constexpr int32 PauseMenuButtons = 3;

	/** True when the mouse should be the plain pointer (a menu is up) and not the crosshair of the game. */
	static bool WantsMenuPointer(bool bStartMenu, bool bPaused, bool bShrineOpen, bool bRunWon);

	/** How solid a health bar is: 1 while the unit was just hit, fading to 0 over the last HealthBarFadeSeconds. */
	static float HealthBarAlpha(float SecondsLeft);

	static constexpr float HealthBarFadeSeconds = 0.5f;

	void SetDebugVisible(bool bVisible) { bDebugVisible = bVisible; }

	UFUNCTION(BlueprintPure, Category = "Wizard")
	bool IsDebugVisible() const { return bDebugVisible; }

	/** Health bars over enemies that were just hit. Off draws none. */
	void SetHealthBarsVisible(bool bVisible) { bHealthBarsVisible = bVisible; }

	UFUNCTION(BlueprintPure, Category = "Wizard")
	bool AreHealthBarsVisible() const { return bHealthBarsVisible; }

	virtual void DrawHUD() override;

	/** The edge fades in and out over about this long. */
	static constexpr float EdgeFadeSeconds = 0.3f;

	static constexpr float MinLayoutScale = 0.5f;

	/** The shrine menu's size on a 1920x1080 screen. */
	static constexpr float ShrineMenuWidth = 600.0f;
	static constexpr float ShrineMenuMargin = 60.0f;
	static constexpr float ShrineMenuHeight = 800.0f;

private:
	/** Text drawn from a real font at exactly PixelHeight, so it is sharp at any resolution. */
	/** Spacing is extra room between letters, as a share of the text's height (headings are spaced out). */
	void DrawSharpText(const FString& Text, const FLinearColor& Color, float X, float Y, float PixelHeight, bool bBold, bool bCentered = false, float Spacing = 0.0f, bool bShadow = true);
	/** A title or label, in Cinzel Bold, its letters spaced out by a share of the text's height. Returns its width. */
	float DrawTitleText(const FString& Text, const FLinearColor& Color, float X, float Y, float PixelHeight, bool bCentered = false, float Spacing = 0.0f);
	/** How wide DrawSharpText draws a text, in pixels. */
	float TextWidth(const FString& Text, float PixelHeight, bool bBold, float Spacing = 0.0f) const;
	/** A menu panel: dark fill, parchment frame, a faint line inside it. */
	void DrawPanel(const FBox2D& Box, float Scale);
	/** A frame of this thickness just inside a box. */
	void DrawFrame(const FBox2D& Box, float Thickness, const FLinearColor& Color);
	/** A menu button. With a key, the key's cap is drawn after the label (or at the right end, the label at the left). */
	void DrawMenuButton(const FBox2D& Box, const FString& Label, EWizardButtonLook Look, float TextHeight, float Scale, const TCHAR* Key = nullptr, bool bLabelLeft = false);
	float KeyCapWidth(const FString& Key, float Scale, bool bNarrow) const;
	/** A key's cap, its left edge at X (or its right edge, when right aligned). Returns its width. */
	float DrawKeyCap(const FString& Key, float X, float Y, float Scale, bool bRightAligned, bool bNarrow = false);
	void DrawRecallArrow(float X, float Y, float Size, float Scale);
	void DrawControlsPanel(const FBox2D& Box, float Scale);
	void DrawStartMenu(float Width, float Height, float Scale);
	void DrawSettings(const AWizardPawn& Wizard, const FWizardSettings& Settings, float Width, float Height);
	void DrawSettingsControls(const AWizardPawn& Wizard, const FBox2D& Panel, float Width, float Height, float Scale);
	void DrawSettingsSound(const FWizardSettings& Settings, float Width, float Height, float Scale);
	void DrawSettingsDisplay(const AWizardPawn& Wizard, const FWizardSettings& Settings, float Width, float Height, float Scale);
	/** A slider on its track box: Share is how far along the handle is, 0 to 1. Disabled, it is grey and has no fill. */
	void DrawSettingsSlider(const FBox2D& Track, float Share, bool bEnabled, float Scale, bool bGreyFill = false);
	/** A row of choices, the chosen one lit. Chosen is INDEX_NONE when none is. */
	void DrawSettingsChoices(const FBox2D* Boxes, const TCHAR* const* Labels, int32 Count, int32 Chosen, bool bEnabled, float Scale);
	/** A key on the Controls tab. State: 0 as it is, 1 under the mouse, 2 waiting for its new key, 3 one that cannot change (dashed). */
	void DrawSettingsKeyBox(const FBox2D& Box, const FString& Key, int32 State, float Scale);
	void DrawGateMessage(const UCrowdSubsystem& Crowd, const AWizardPawn* Wizard, float Width, float Height, float Scale);
	/** The crown of the commander's icon, on a 34 by 34 grid of Unit pixels. */
	void DrawCrown(float Left, float Top, float Unit, const FLinearColor& Fill, const FLinearColor& Rim);
	/** A line that fades to nothing at its far end; Length is negative to run left. */
	void DrawFadingLine(float X, float Y, float Length, const FLinearColor& Color);
	void DrawEdges(float Width, float Height);
	void DrawRings(const UCrowdSubsystem& Crowd, const AWizardPawn& Wizard);
	void DrawGroundRing(FVector2D Middle, float Radius, float GroundZ, const FLinearColor& Color, float Thickness);
	void DrawCommand(const UCrowdSubsystem& Crowd, const AWizardPawn& Wizard, float Width, float Height, float Scale);
	void DrawHealthBars(const UCrowdSubsystem& Crowd, const AWizardPawn& Wizard, float Scale);
	void DrawShrine(const UCrowdSubsystem& Crowd, const AWizardPawn& Wizard, float Width, float Height, float Scale);
	void DrawPauseMenu(float Width, float Height, float Scale);
	void DrawFinalCut(const UCrowdSubsystem& Crowd, float Width, float Height, float Scale);
	void DrawAbilityBar(const UCrowdSubsystem& Crowd, float Width, float Height, float Scale);
	void DrawCommanderIcon(float CenterX, float CenterY, float Scale);
	/** A triangle with a level base, drawn as thin strips: the canvas has no triangle of its own that needs no texture. */
	void DrawSpike(FVector2D Apex, float BaseLeft, float BaseRight, float BaseY, const FLinearColor& Color);
	void DrawDebug(float Width, float Scale, float BarRight, float BelowBar);

	/** The engine's Roboto font. Unlike the small bitmap fonts, it is drawn from outlines at any size. */
	UPROPERTY()
	TObjectPtr<const UFont> TextFont;

	/** Cinzel Bold (/Game/UI/Fonts/FF_Cinzel_Bold), for titles and labels, and the typeface made from it. */
	UPROPERTY()
	TObjectPtr<const UFontFace> TitleFace;

	UPROPERTY(Transient)
	TObjectPtr<UFont> TitleFont;

	// The ability and bone icons (/Game/UI). A missing one leaves its slot a plain box.
	UPROPERTY()
	TObjectPtr<UTexture2D> SummonIcon;

	UPROPERTY()
	TObjectPtr<UTexture2D> SpearIcon;

	UPROPERTY()
	TObjectPtr<UTexture2D> BlinkIcon;

	UPROPERTY()
	TObjectPtr<UTexture2D> CommandIcon;

	UPROPERTY()
	TObjectPtr<UTexture2D> BoneIcon;

	// The shrine's rows and the start menu's picture.
	UPROPERTY()
	TObjectPtr<UTexture2D> ArmorIcon;

	UPROPERTY()
	TObjectPtr<UTexture2D> BladesIcon;

	UPROPERTY()
	TObjectPtr<UTexture2D> FocusIcon;

	UPROPERTY()
	TObjectPtr<UTexture2D> GuardIcon;

	UPROPERTY()
	TObjectPtr<UTexture2D> MarksmanIcon;

	UPROPERTY()
	TObjectPtr<UTexture2D> StartBackground;

	// Menu panels, buttons and key caps are drawn this solid: the final cut fades them in.
	float MenuAlpha = 1.0f;
	// How much of the locked button's hover text shows.
	float LockedHover = 0.0f;

	FWizardEdgeWeights ShownEdges;
	// The player's "Danger edges" setting: the edges are drawn this strong.
	float EdgeFactor = 1.0f;
	// For the frame-rate number in the corner.
	float ShownFrameMs = 16.7f;
	float SmoothedFrameMs = 16.7f;
	bool bDebugVisible = false;
	bool bHealthBarsVisible = true;
};
