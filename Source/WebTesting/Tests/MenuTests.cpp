#include "Components/LightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Font.h"
#include "Engine/FontFace.h"
#include "Engine/Level.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"
#include "Game/WizardHUD.h"
#include "Game/WizardMenuState.h"
#include "Tests/WizTestFlags.h"

#if WITH_DEV_AUTOMATION_TESTS

// Direction of 2026-11-14 14:00 PST: "Change the lighting to give a more night time feel while still being able to
// see in-game. Add all the UI elements from the UI thread as more is finished": the start menu ("March", "Controls"),
// the pause menu with its two buttons and the controls panel, and the shrine menu's new panel.

namespace MenuTest
{
	const float Sizes[][2] = { { 1920.0f, 1080.0f }, { 1280.0f, 720.0f }, { 640.0f, 480.0f }, { 3840.0f, 2160.0f } };

	bool OnScreen(const FBox2D& Box, float Width, float Height)
	{
		return Box.Min.X >= 0.0 && Box.Min.Y >= 0.0 && Box.Max.X <= Width && Box.Max.Y <= Height;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMenuStartFlow, "WebTesting.Menu.TheStartMenuStartsTheRun", WIZ_TEST_FLAGS)
bool FMenuStartFlow::RunTest(const FString& Parameters)
{
	FWizardMenuState Menu;
	TestFalse(TEXT("no menu until it is shown"), Menu.IsOpen());
	TestFalse(TEXT("nothing to start with no menu up"), Menu.March());
	Menu.ToggleControls();
	TestFalse(TEXT("the controls key does nothing with no menu up"), Menu.IsOpen());

	Menu.Show();
	TestEqual(TEXT("shown: the start screen"), FString(Menu.GetName()), FString(TEXT("start")));
	Menu.ToggleControls();
	TestEqual(TEXT("Controls opens the controls"), FString(Menu.GetName()), FString(TEXT("controls")));
	TestTrue(TEXT("the menu is still up"), Menu.IsOpen());
	Menu.ToggleControls();
	TestEqual(TEXT("Back returns to the start screen"), FString(Menu.GetName()), FString(TEXT("start")));
	TestTrue(TEXT("March starts the run"), Menu.March());
	TestFalse(TEXT("and the menu is gone"), Menu.IsOpen());

	Menu.Show();
	Menu.ToggleControls();
	TestTrue(TEXT("March also starts the run from the controls"), Menu.March());
	TestEqual(TEXT("no menu after it"), FString(Menu.GetName()), FString(TEXT("none")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMenuStartButtons, "WebTesting.Hud.StartMenuButtons", WIZ_TEST_FLAGS)
bool FMenuStartButtons::RunTest(const FString& Parameters)
{
	for (const float* Size : MenuTest::Sizes)
	{
		const float Width = Size[0];
		const float Height = Size[1];
		const FBox2D March = AWizardHUD::StartMenuButtonBox(0, Width, Height);
		// The second button was "Controls"; since the settings hand-off it is "Settings", whose first tab is the controls.
		const FBox2D Controls = AWizardHUD::StartMenuButtonBox(1, Width, Height);
		TestTrue(TEXT("March is on the screen"), MenuTest::OnScreen(March, Width, Height));
		TestTrue(TEXT("Settings is on the screen"), MenuTest::OnScreen(Controls, Width, Height));
		TestTrue(TEXT("Settings is under March, not on it"), Controls.Min.Y >= March.Max.Y);
		TestTrue(TEXT("the buttons are on the left half, clear of the picture's middle"), March.Max.X <= Width * 0.6);
		TestTrue(TEXT("big enough to click"), March.GetSize().X >= 100.0 && March.GetSize().Y >= 30.0);
		TestEqual(TEXT("a click on March"), AWizardHUD::StartMenuButtonAt(March.GetCenter(), Width, Height), 0);
		TestEqual(TEXT("a click on Settings"), AWizardHUD::StartMenuButtonAt(Controls.GetCenter(), Width, Height), 1);
		TestEqual(TEXT("a click on the picture hits nothing"), AWizardHUD::StartMenuButtonAt(FVector2D(Width - 10.0, 10.0), Width, Height), static_cast<int32>(INDEX_NONE));
	}
	const FBox2D Normal = AWizardHUD::StartMenuButtonBox(0, 1920.0f, 1080.0f);
	const FBox2D Large = AWizardHUD::StartMenuButtonBox(0, 3840.0f, 2160.0f);
	TestTrue(TEXT("at 4K a button is twice the size"), FMath::IsNearlyEqual(Large.GetSize().X, Normal.GetSize().X * 2.0, 2.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMenuPauseButtons, "WebTesting.Hud.PauseMenuButtons", WIZ_TEST_FLAGS)
bool FMenuPauseButtons::RunTest(const FString& Parameters)
{
	for (const float* Size : MenuTest::Sizes)
	{
		const float Width = Size[0];
		const float Height = Size[1];
		const FBox2D Resume = AWizardHUD::PauseMenuButtonBox(0, Width, Height);
		const FBox2D Settings = AWizardHUD::PauseMenuButtonBox(1, Width, Height);
		const FBox2D Restart = AWizardHUD::PauseMenuButtonBox(2, Width, Height);
		const FBox2D Panel = AWizardHUD::ControlsPanelBox(Width, Height);
		TestTrue(TEXT("Resume is on the screen"), MenuTest::OnScreen(Resume, Width, Height));
		TestTrue(TEXT("Restart is on the screen"), MenuTest::OnScreen(Restart, Width, Height));
		TestTrue(TEXT("the controls panel is on the screen"), MenuTest::OnScreen(Panel, Width, Height));
		TestTrue(TEXT("Settings is on the screen"), MenuTest::OnScreen(Settings, Width, Height));
		TestTrue(TEXT("Settings is between Resume and Restart"), Settings.Min.Y >= Resume.Max.Y && Restart.Min.Y >= Settings.Max.Y);
		TestTrue(TEXT("the controls are under the buttons"), Panel.Min.Y >= Restart.Max.Y);
		TestTrue(TEXT("the buttons are in the middle"), FMath::IsNearlyEqual(Resume.GetCenter().X, Width * 0.5, 1.0));
		TestEqual(TEXT("a click on Resume"), AWizardHUD::PauseMenuButtonAt(Resume.GetCenter(), Width, Height), 0);
		TestEqual(TEXT("a click on Settings"), AWizardHUD::PauseMenuButtonAt(Settings.GetCenter(), Width, Height), 1);
		TestEqual(TEXT("a click on Restart"), AWizardHUD::PauseMenuButtonAt(Restart.GetCenter(), Width, Height), 2);
		TestEqual(TEXT("a click on the controls hits no button"), AWizardHUD::PauseMenuButtonAt(Panel.GetCenter(), Width, Height), static_cast<int32>(INDEX_NONE));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMenuTitleFont, "WebTesting.Hud.TheTitleFontIsCinzelBold", WIZ_TEST_FLAGS)
bool FMenuTitleFont::RunTest(const FString& Parameters)
{
	// Director, 2026-11-14 14:17 PST: Cinzel may be downloaded for the titles.
	// Changed before it ever passed: it asked for a font asset; the HUD now makes its typeface from the face itself.
	const UFontFace* Face = LoadObject<UFontFace>(nullptr, TEXT("/Game/UI/Fonts/FF_Cinzel_Bold.FF_Cinzel_Bold"));
	if (!TestNotNull(TEXT("the title font's face"), Face))
	{
		return false;
	}
	TestTrue(TEXT("the face is inside its asset, so a package needs no loose file"), Face->LoadingPolicy == EFontLoadingPolicy::Inline);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLevelIsNight, "WebTesting.Level.TheOutskirtsIsLitAsNight", WIZ_TEST_FLAGS)
bool FLevelIsNight::RunTest(const FString& Parameters)
{
	// Fails on the level as it was (a white sun of 6 lux, sky light 1, no post process volume): seen in the level
	// before Tools/Level/night_lighting.py ran.
	const UWorld* Map = LoadObject<UWorld>(nullptr, TEXT("/Game/Maps/Outskirts.Outskirts"));
	if (!TestNotNull(TEXT("the Outskirts level"), Map) || !TestNotNull(TEXT("its level"), Map->PersistentLevel.Get()))
	{
		return false;
	}
	const ADirectionalLight* Moon = nullptr;
	const ASkyLight* Sky = nullptr;
	const APostProcessVolume* Look = nullptr;
	for (const AActor* Actor : Map->PersistentLevel->Actors)
	{
		Moon = Moon ? Moon : Cast<ADirectionalLight>(Actor);
		Sky = Sky ? Sky : Cast<ASkyLight>(Actor);
		const APostProcessVolume* Volume = Cast<APostProcessVolume>(Actor);
		if (Volume && Volume->bUnbound)
		{
			Look = Volume;
		}
	}
	if (!TestNotNull(TEXT("a moon"), Moon) || !TestNotNull(TEXT("a sky light"), Sky) || !TestNotNull(TEXT("a look for the whole level"), Look))
	{
		return false;
	}
	const ULightComponent* MoonLight = Moon->GetLightComponent();
	TestTrue(TEXT("the moon is far dimmer than the sun was (6)"), MoonLight->Intensity <= 4.0f);
	TestTrue(TEXT("but it still lights the ground"), MoonLight->Intensity >= 1.0f);
	TestTrue(TEXT("it still casts the shadows"), MoonLight->CastShadows);
	TestTrue(TEXT("its light is cold: more blue than red"), MoonLight->LightColor.B > MoonLight->LightColor.R);
	TestTrue(TEXT("the sky light is raised, so shadows are never black"), Sky->GetLightComponent()->Intensity >= 2.0f);
	TestTrue(TEXT("exposure is fixed: the night is not brightened back to day"),
		Look->Settings.bOverride_AutoExposureMethod && Look->Settings.AutoExposureMethod == EAutoExposureMethod::AEM_Manual
		&& Look->Settings.bOverride_AutoExposureApplyPhysicalCameraExposure && !Look->Settings.AutoExposureApplyPhysicalCameraExposure);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
