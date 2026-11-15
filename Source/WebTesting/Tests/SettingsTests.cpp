#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Game/WizardHUD.h"
#include "Game/WizardKeys.h"
#include "Game/WizardSettings.h"
#include "Game/WizardSettingsSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/WizTestFlags.h"

#if WITH_DEV_AUTOMATION_TESTS

// Direction of 2026-11-14 21:10 PST: "implement the new settings UI that the other thread made" (the UI thread's
// settings hand-off): a panel with Controls (read-only), Sound (shown, not usable yet) and Display tabs, opened from
// the start menu and the pause menu, every change applied at once and kept between runs.

namespace SettingsTest
{
	const float Sizes[][2] = { { 1920.0f, 1080.0f }, { 1280.0f, 720.0f }, { 640.0f, 480.0f }, { 3840.0f, 2160.0f }, { 1000.0f, 1400.0f } };

	bool Inside(const FBox2D& Inner, const FBox2D& Outer)
	{
		return Inner.Min.X >= Outer.Min.X && Inner.Min.Y >= Outer.Min.Y && Inner.Max.X <= Outer.Max.X && Inner.Max.Y <= Outer.Max.Y;
	}

	int32 Row(EWizardDisplayRow DisplayRow)
	{
		return static_cast<int32>(DisplayRow);
	}

	/** Puts the HUD's size back when a test ends, however it ends. */
	struct FSizeGuard
	{
		~FSizeGuard() { AWizardHUD::SetSizeFactor(1.0f); }
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSettingsValues, "WebTesting.Settings.ValuesStayInRangeAndOnTheirSteps", WIZ_TEST_FLAGS)
bool FSettingsValues::RunTest(const FString& Parameters)
{
	FWizardSettings Settings;
	TestEqual(TEXT("quality starts at high: how the game looked before it had settings"), Settings.Quality, 2);
	TestEqual(TEXT("render scale starts at 100"), Settings.RenderScale, 100);
	TestEqual(TEXT("HUD size starts at 100"), Settings.HudSize, 100);
	TestEqual(TEXT("danger edges start at 100"), Settings.DangerEdges, 100);
	TestEqual(TEXT("the frame rate starts hidden"), Settings.FrameRate, 0);

	TestTrue(TEXT("a new value changes the setting"), Settings.Set(EWizardDisplayRow::HudSize, 115));
	TestEqual(TEXT("and is kept"), Settings.HudSize, 115);
	TestFalse(TEXT("the same value again changes nothing"), Settings.Set(EWizardDisplayRow::HudSize, 115));
	Settings.Set(EWizardDisplayRow::HudSize, 500);
	TestEqual(TEXT("HUD size stops at 130"), Settings.HudSize, 130);
	Settings.Set(EWizardDisplayRow::HudSize, 10);
	TestEqual(TEXT("and at 80"), Settings.HudSize, 80);
	Settings.Set(EWizardDisplayRow::HudSize, 97);
	TestEqual(TEXT("HUD size moves in steps of 5"), Settings.HudSize, 95);
	Settings.Set(EWizardDisplayRow::RenderScale, 74);
	TestEqual(TEXT("render scale moves in steps of 10"), Settings.RenderScale, 70);
	Settings.Set(EWizardDisplayRow::RenderScale, 0);
	TestEqual(TEXT("render scale stops at 50"), Settings.RenderScale, 50);
	Settings.Set(EWizardDisplayRow::DangerEdges, -20);
	TestEqual(TEXT("danger edges go down to 0, which is off"), Settings.DangerEdges, 0);
	Settings.Set(EWizardDisplayRow::Quality, 7);
	TestEqual(TEXT("quality stops at high"), Settings.Quality, 2);
	Settings.Set(EWizardDisplayRow::FrameRate, 1);
	TestEqual(TEXT("the frame rate can be shown"), Settings.FrameRate, 1);

	TestFalse(TEXT("screen shake does nothing yet: the game has no shake"), FWizardSettings::IsEnabled(EWizardDisplayRow::ScreenShake));
	TestFalse(TEXT("full screen does nothing yet"), FWizardSettings::IsEnabled(EWizardDisplayRow::FullScreen));
	TestFalse(TEXT("a row that does nothing cannot be set"), Settings.Set(EWizardDisplayRow::ScreenShake, 1));

	FWizardSettings Broken;
	Broken.Quality = -4;
	Broken.RenderScale = 1000;
	Broken.HudSize = 3;
	Broken.DangerEdges = 52;
	Broken.FrameRate = 9;
	Broken.Clamp();
	TestEqual(TEXT("a saved quality out of range is brought in"), Broken.Quality, 0);
	TestEqual(TEXT("a saved render scale out of range is brought in"), Broken.RenderScale, 100);
	TestEqual(TEXT("a saved HUD size out of range is brought in"), Broken.HudSize, 80);
	TestEqual(TEXT("a saved value off its steps is put on one"), Broken.DangerEdges, 50);
	TestEqual(TEXT("a saved switch out of range is brought in"), Broken.FrameRate, 1);

	TestTrue(TEXT("the console's names find their rows"), FWizardSettings::RowByName(TEXT("HudSize")) == EWizardDisplayRow::HudSize
		&& FWizardSettings::RowByName(TEXT("edges")) == EWizardDisplayRow::DangerEdges && FWizardSettings::RowByName(TEXT("nothing")) == EWizardDisplayRow::Count);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSettingsScreenTabs, "WebTesting.Settings.TheScreenOpensOnTheRightTab", WIZ_TEST_FLAGS)
bool FSettingsScreenTabs::RunTest(const FString& Parameters)
{
	FWizardSettingsScreen Screen;
	TestFalse(TEXT("closed to begin with"), Screen.IsOpen());
	Screen.SetTab(EWizardSettingsTab::Sound);
	TestEqual(TEXT("a tab cannot be picked while it is closed"), FString(Screen.GetName()), FString(TEXT("closed")));

	Screen.OpenFromPause();
	TestEqual(TEXT("from the pause menu it opens on Display the first time"), FString(Screen.GetName()), FString(TEXT("display")));
	Screen.SetTab(EWizardSettingsTab::Sound);
	Screen.Close();
	TestFalse(TEXT("Back closes it"), Screen.IsOpen());
	Screen.OpenFromPause();
	TestEqual(TEXT("from the pause menu it opens on the tab last used there"), FString(Screen.GetName()), FString(TEXT("sound")));
	Screen.Close();

	Screen.OpenFromStart();
	TestEqual(TEXT("from the start menu it opens on Controls"), FString(Screen.GetName()), FString(TEXT("controls")));
	TestTrue(TEXT("and remembers where it came from"), Screen.IsFromStart());
	Screen.SetTab(EWizardSettingsTab::Display);
	Screen.Close();
	Screen.OpenFromStart();
	TestEqual(TEXT("from the start menu it is Controls every time"), FString(Screen.GetName()), FString(TEXT("controls")));
	Screen.Close();
	Screen.OpenFromPause();
	TestEqual(TEXT("a tab picked from the start menu does not change the pause menu's"), FString(Screen.GetName()), FString(TEXT("sound")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSettingsLayout, "WebTesting.Hud.TheSettingsScreenFitsAndIsClicked", WIZ_TEST_FLAGS)
bool FSettingsLayout::RunTest(const FString& Parameters)
{
	using namespace SettingsTest;
	FSizeGuard Guard;
	for (const float Factor : { 1.0f, 0.8f, 1.3f })
	{
		AWizardHUD::SetSizeFactor(Factor);
		for (const float* Size : Sizes)
		{
			const float Width = Size[0];
			const float Height = Size[1];
			const FBox2D Screen(FVector2D(0.0, 0.0), FVector2D(Width, Height));
			const FBox2D Panel = AWizardHUD::SettingsPanelBox(Width, Height);
			TestTrue(FString::Printf(TEXT("the panel is on a %.0f x %.0f screen at size %.0f %%"), Width, Height, Factor * 100.0f), Inside(Panel, Screen));
			TestTrue(TEXT("the panel is in the middle"), FMath::IsNearlyEqual(Panel.GetCenter().X, Width * 0.5, 1.0) && FMath::IsNearlyEqual(Panel.GetCenter().Y, Height * 0.5, 1.0));

			const FBox2D Back = AWizardHUD::SettingsBackBox(Width, Height);
			const FBox2D Reset = AWizardHUD::SettingsResetBox(Width, Height);
			TestTrue(TEXT("Back and Reset are in the panel"), Inside(Back, Panel) && Inside(Reset, Panel));
			TestTrue(TEXT("Reset is left of Back, not on it"), Reset.Max.X <= Back.Min.X);
			double TabsBottom = 0.0;
			for (int32 Tab = 0; Tab < 3; ++Tab)
			{
				const FBox2D Box = AWizardHUD::SettingsTabBox(Tab, Width, Height);
				TestTrue(TEXT("a tab is in the panel"), Inside(Box, Panel));
				TestTrue(TEXT("tabs are side by side"), Tab == 0 || Box.Min.X >= AWizardHUD::SettingsTabBox(Tab - 1, Width, Height).Max.X);
				TabsBottom = Box.Max.Y;
				for (const EWizardSettingsTab Open : { EWizardSettingsTab::Controls, EWizardSettingsTab::Sound, EWizardSettingsTab::Display })
				{
					const FWizardSettingsHit Hit = AWizardHUD::SettingsHitAt(Box.GetCenter(), Open, Width, Height);
					TestTrue(TEXT("a click on a tab picks it, from any tab"), Hit.Kind == EWizardSettingsHitKind::Tab && Hit.Row == Tab);
				}
			}
			TestTrue(TEXT("a click on Back"), AWizardHUD::SettingsHitAt(Back.GetCenter(), EWizardSettingsTab::Controls, Width, Height).Kind == EWizardSettingsHitKind::Back);
			TestTrue(TEXT("a click on Reset on the Display tab"), AWizardHUD::SettingsHitAt(Reset.GetCenter(), EWizardSettingsTab::Display, Width, Height).Kind == EWizardSettingsHitKind::Reset);
			// Changed 2026-11-14 22:30 PST: keys can be changed and sound has its channels, so every tab has its own reset.
			TestTrue(TEXT("Reset is there on the Controls tab"), AWizardHUD::SettingsHitAt(Reset.GetCenter(), EWizardSettingsTab::Controls, Width, Height).Kind == EWizardSettingsHitKind::Reset);
			TestTrue(TEXT("and on the Sound tab"), AWizardHUD::SettingsHitAt(Reset.GetCenter(), EWizardSettingsTab::Sound, Width, Height).Kind == EWizardSettingsHitKind::Reset);

			for (int32 Index = 0; Index < Row(EWizardDisplayRow::Count); ++Index)
			{
				const EWizardDisplayRow DisplayRow = static_cast<EWizardDisplayRow>(Index);
				const FBox2D RowBox = AWizardHUD::SettingsRowBox(Index, Width, Height);
				TestTrue(TEXT("a row is in the panel, under the tabs and over the footer's buttons"), Inside(RowBox, Panel) && RowBox.Min.Y >= TabsBottom && RowBox.Max.Y <= Back.Min.Y);
				TestTrue(TEXT("rows do not overlap"), Index == 0 || RowBox.Min.Y >= AWizardHUD::SettingsRowBox(Index - 1, Width, Height).Max.Y);
				if (FWizardSettings::IsSlider(DisplayRow))
				{
					const FBox2D Track = AWizardHUD::SettingsSliderBox(Index, Width, Height);
					int32 Min = 0;
					int32 Max = 0;
					int32 Step = 1;
					FWizardSettings::Range(DisplayRow, Min, Max, Step);
					TestTrue(TEXT("a slider is in its row"), Inside(Track, RowBox));
					TestTrue(TEXT("a slider is long enough to drag"), Track.GetSize().X >= 80.0);
					TestEqual(TEXT("the left end is the lowest value"), AWizardHUD::SettingsSliderValue(Index, static_cast<float>(Track.Min.X), Width, Height), Min);
					TestEqual(TEXT("the right end is the highest"), AWizardHUD::SettingsSliderValue(Index, static_cast<float>(Track.Max.X), Width, Height), Max);
					TestEqual(TEXT("far past the left end is still the lowest"), AWizardHUD::SettingsSliderValue(Index, -5000.0f, Width, Height), Min);
					TestEqual(TEXT("far past the right end is still the highest"), AWizardHUD::SettingsSliderValue(Index, 50000.0f, Width, Height), Max);
					const int32 Middle = AWizardHUD::SettingsSliderValue(Index, static_cast<float>(Track.GetCenter().X), Width, Height);
					TestTrue(TEXT("the middle is between them, on a step"), Middle > Min && Middle < Max && (Middle - Min) % Step == 0);
					const FWizardSettingsHit Hit = AWizardHUD::SettingsHitAt(Track.GetCenter(), EWizardSettingsTab::Display, Width, Height);
					TestTrue(TEXT("a click on a slider takes hold of it"), Hit.Kind == EWizardSettingsHitKind::Slider && Hit.Row == Index);
					TestTrue(TEXT("a Display slider is not there on the Sound tab"), AWizardHUD::SettingsHitAt(Track.GetCenter(), EWizardSettingsTab::Sound, Width, Height).Kind != EWizardSettingsHitKind::Slider);
					continue;
				}
				for (int32 Choice = 0; Choice < FWizardSettings::Choices(DisplayRow); ++Choice)
				{
					const FBox2D Box = AWizardHUD::SettingsChoiceBox(Index, Choice, Width, Height);
					TestTrue(TEXT("a choice is in its row"), Inside(Box, RowBox));
					const FWizardSettingsHit Hit = AWizardHUD::SettingsHitAt(Box.GetCenter(), EWizardSettingsTab::Display, Width, Height);
					if (FWizardSettings::IsEnabled(DisplayRow))
					{
						TestTrue(TEXT("a click on a choice picks it"), Hit.Kind == EWizardSettingsHitKind::Choice && Hit.Row == Index && Hit.Index == Choice);
					}
					else
					{
						TestTrue(TEXT("a choice that does nothing yet is not clicked"), Hit.Kind == EWizardSettingsHitKind::None);
					}
					TestTrue(TEXT("a choice is not there on the Controls tab"), AWizardHUD::SettingsHitAt(Box.GetCenter(), EWizardSettingsTab::Controls, Width, Height).Kind != EWizardSettingsHitKind::Choice);
				}
			}
			const FBox2D FullScreen = AWizardHUD::SettingsChoiceBox(Row(EWizardDisplayRow::FullScreen), 0, Width, Height);
			TestTrue(TEXT("the full screen button is not clicked yet"), AWizardHUD::SettingsHitAt(FullScreen.GetCenter(), EWizardSettingsTab::Display, Width, Height).Kind == EWizardSettingsHitKind::None);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSettingsHudSize, "WebTesting.Hud.TheSizeSettingScalesTheHudAndTheMenusStillFit", WIZ_TEST_FLAGS)
bool FSettingsHudSize::RunTest(const FString& Parameters)
{
	using namespace SettingsTest;
	FSizeGuard Guard;
	AWizardHUD::SetSizeFactor(1.0f);
	const float Normal = AWizardHUD::LayoutScale(1920.0f, 1080.0f);
	const FBox2D NormalMarch = AWizardHUD::StartMenuButtonBox(0, 1920.0f, 1080.0f);
	TestTrue(TEXT("at 100 % the HUD is its designed size on a 1920 x 1080 screen"), FMath::IsNearlyEqual(Normal, 1.0f, 0.001f));
	TestTrue(TEXT("and the menus are too"), FMath::IsNearlyEqual(AWizardHUD::MenuScale(1920.0f, 1080.0f), 1.0f, 0.001f));

	AWizardHUD::SetSizeFactor(1.3f);
	TestTrue(TEXT("at 130 % the HUD is 1.3 times the size"), FMath::IsNearlyEqual(AWizardHUD::LayoutScale(1920.0f, 1080.0f), 1.3f, 0.001f));
	TestTrue(TEXT("and so is a start menu button"), FMath::IsNearlyEqual(AWizardHUD::StartMenuButtonBox(0, 1920.0f, 1080.0f).GetSize().X, NormalMarch.GetSize().X * 1.3, 2.0));
	TestTrue(TEXT("the tall menus grow, but less: only as far as still fits"), AWizardHUD::MenuScale(1920.0f, 1080.0f) > 1.0f && AWizardHUD::MenuScale(1920.0f, 1080.0f) < 1.3f);
	for (const float* Size : Sizes)
	{
		const FBox2D Screen(FVector2D(0.0, 0.0), FVector2D(Size[0], Size[1]));
		// A screen narrower than the menus were made for (640 wide) never fitted the controls panel's width: its height is what the size setting may not break.
		const FBox2D Controls = AWizardHUD::ControlsPanelBox(Size[0], Size[1]);
		TestTrue(TEXT("at 130 % the pause menu's controls still end on the screen"), Controls.Max.Y <= Size[1]);
		for (int32 Button = 0; Button < AWizardHUD::PauseMenuButtons; ++Button)
		{
			TestTrue(TEXT("at 130 % the pause menu's buttons are on the screen"), Inside(AWizardHUD::PauseMenuButtonBox(Button, Size[0], Size[1]), Screen));
		}
	}

	AWizardHUD::SetSizeFactor(0.8f);
	TestTrue(TEXT("at 80 % the HUD is 0.8 times the size"), FMath::IsNearlyEqual(AWizardHUD::LayoutScale(1920.0f, 1080.0f), 0.8f, 0.001f));
	TestTrue(TEXT("and the menus are too"), FMath::IsNearlyEqual(AWizardHUD::MenuScale(1920.0f, 1080.0f), 0.8f, 0.001f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSettingsKept, "WebTesting.Settings.SettingsAreKeptBetweenRuns", WIZ_TEST_FLAGS)
bool FSettingsKept::RunTest(const FString& Parameters)
{
	SettingsTest::FSizeGuard Guard;
	const FString Slot(TEXT("WizardSettingsTest"));
	UGameplayStatics::DeleteGameInSlot(Slot, 0);
	UGameInstance* Game = NewObject<UGameInstance>(GEngine);
	UWizardSettingsSubsystem* Store = NewObject<UWizardSettingsSubsystem>(Game);
	// The test's values go to its own slot, never the player's.
	Store->SetKeptBetweenRuns(false);
	TestFalse(TEXT("nothing to load before anything was saved"), Store->LoadFrom(Slot));

	TestTrue(TEXT("a new HUD size is taken"), Store->Set(EWizardDisplayRow::HudSize, 115));
	TestTrue(TEXT("and is in force at once"), FMath::IsNearlyEqual(AWizardHUD::GetSizeFactor(), 1.15f, 0.001f));
	Store->Set(EWizardDisplayRow::DangerEdges, 40);
	Store->Set(EWizardDisplayRow::FrameRate, 1);
	Store->SetVolume(1, 35);
	Store->SetMuted(2, true);
	Store->SetBackgroundMute(false);
	FName Other;
	WizardKeys().Reset();
	WizardKeys().Rebind(TEXT("Blink"), EKeys::LeftShift, &Other);
	WizardKeys().Rebind(TEXT("Summon"), EKeys::F, &Other);
	TestEqual(TEXT("the getters report it"), Store->GetHudSize(), 115);
	TestEqual(TEXT("danger edges"), Store->GetDangerEdges(), 40);
	TestTrue(TEXT("frame rate"), Store->IsFrameRateShown());
	TestFalse(TEXT("quality and render scale were not touched: the engine's own are left alone"), Store->AreGraphicsApplied());
	TestTrue(TEXT("the settings are written"), Store->SaveTo(Slot));

	UWizardSettingsSubsystem* Next = NewObject<UWizardSettingsSubsystem>(Game);
	Next->SetKeptBetweenRuns(false);
	TestTrue(TEXT("and read back in a later run"), Next->LoadFrom(Slot));
	TestTrue(TEXT("with every value as it was left"), Next->Get() == Store->Get());
	TestEqual(TEXT("the music volume among them"), Next->GetVolume(1), 35);
	TestTrue(TEXT("and the mute on effects"), Next->IsMuted(2));
	TestFalse(TEXT("and the background choice"), Next->IsBackgroundMuted());

	// The keys are in the same save: wipe them, read the save again, and they are back.
	WizardKeys().Reset();
	TestTrue(TEXT("with the keys wiped, Blink is on Space"), WizardKeys().Get(TEXT("Blink")) == EKeys::SpaceBar);
	TestTrue(TEXT("the save is read again"), Next->LoadFrom(Slot));
	TestTrue(TEXT("and Blink is back on Shift"), WizardKeys().Get(TEXT("Blink")) == EKeys::LeftShift);
	TestTrue(TEXT("Summon on F"), WizardKeys().Get(TEXT("Summon")) == EKeys::F);
	TestTrue(TEXT("and Command, which swapped with it, on Q"), WizardKeys().Get(TEXT("Command")) == EKeys::Q);
	WizardKeys().Reset();

	Store->ResetDisplay();
	TestTrue(TEXT("Reset display puts the Display tab back"), Store->GetHudSize() == 100 && Store->GetDangerEdges() == 100 && !Store->IsFrameRateShown());
	TestEqual(TEXT("and leaves the sound as it was"), Store->GetVolume(1), 35);
	Store->ResetSound();
	TestTrue(TEXT("with Reset sound as well, every value is back"), Store->Get() == FWizardSettings());
	TestTrue(TEXT("and the HUD's size with them"), FMath::IsNearlyEqual(AWizardHUD::GetSizeFactor(), 1.0f, 0.001f));
	UGameplayStatics::DeleteGameInSlot(Slot, 0);
	return true;
}

#endif
