#include "Game/WizardHUD.h"
#include "Game/WizardKeys.h"
#include "Game/WizardSettings.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"
#include "Sound/SoundWave.h"
#include "Tests/WizTestFlags.h"

#if WITH_DEV_AUTOMATION_TESTS

// Direction of 2026-11-14 22:10 PST: "For settings, make the necessary channels made for the settings page. I am
// some sounds prepared here to import ... Also make controls changeable and storable in the cache so the browser
// remembers."

namespace KeysSoundTest
{
	const float Sizes[][2] = { { 1920.0f, 1080.0f }, { 1280.0f, 720.0f }, { 640.0f, 480.0f }, { 3840.0f, 2160.0f } };

	bool Inside(const FBox2D& Inner, const FBox2D& Outer)
	{
		return Inner.Min.X >= Outer.Min.X && Inner.Min.Y >= Outer.Min.Y && Inner.Max.X <= Outer.Max.X && Inner.Max.Y <= Outer.Max.Y;
	}

	bool Apart(const FBox2D& A, const FBox2D& B)
	{
		return A.Max.X <= B.Min.X || B.Max.X <= A.Min.X || A.Max.Y <= B.Min.Y || B.Max.Y <= A.Min.Y;
	}

	/** True when every control has its own key. */
	bool OneKeyEach(const FWizardKeyMap& Keys)
	{
		TSet<FKey> Seen;
		for (const FWizardKeyBinding& Binding : Keys.All())
		{
			if (!Binding.Key.IsValid() || Seen.Contains(Binding.Key))
			{
				return false;
			}
			Seen.Add(Binding.Key);
		}
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKeysRebind, "WebTesting.Keys.AControlCanBeMovedAndTwoSwap", WIZ_TEST_FLAGS)
bool FKeysRebind::RunTest(const FString& Parameters)
{
	using namespace KeysSoundTest;
	FWizardKeyMap Keys;
	FName Other;
	TestTrue(TEXT("nothing is changed to begin with"), Keys.IsDefault());
	TestEqual(TEXT("twelve controls can be moved"), FWizardKeyMap::Changeable().Num(), 12);

	TestTrue(TEXT("Blink moves to a free key"), Keys.Rebind(TEXT("Blink"), EKeys::LeftShift, &Other) == EWizardRebind::Set);
	TestTrue(TEXT("Blink is on Shift"), Keys.Get(TEXT("Blink")) == EKeys::LeftShift);
	TestTrue(TEXT("no other control was involved"), Other.IsNone());
	TestTrue(TEXT("Space is free now"), Keys.ActionOn(EKeys::SpaceBar).IsNone());
	TestTrue(TEXT("the same key again changes nothing"), Keys.Rebind(TEXT("Blink"), EKeys::LeftShift) == EWizardRebind::Same);

	TestTrue(TEXT("Summon onto the key of Command: the two swap"), Keys.Rebind(TEXT("Summon"), EKeys::F, &Other) == EWizardRebind::Swapped);
	TestTrue(TEXT("the other control is named"), Other == FName(TEXT("Command")));
	TestTrue(TEXT("Summon is on F"), Keys.Get(TEXT("Summon")) == EKeys::F);
	TestTrue(TEXT("Command is on Q, where Summon was"), Keys.Get(TEXT("Command")) == EKeys::Q);

	TestTrue(TEXT("Bolt can leave the mouse"), Keys.Rebind(TEXT("Bolt"), EKeys::G) == EWizardRebind::Set);
	TestTrue(TEXT("a control can go on a mouse button"), Keys.Rebind(TEXT("Recall"), EKeys::MiddleMouseButton) == EWizardRebind::Set);
	TestTrue(TEXT("and on the left button Bolt left"), Keys.Rebind(TEXT("Shrine"), EKeys::LeftMouseButton) == EWizardRebind::Set);

	TestTrue(TEXT("R is kept for restarting"), Keys.Rebind(TEXT("Blink"), EKeys::R, &Other) == EWizardRebind::Refused);
	TestTrue(TEXT("and the refusal names it"), Other == FName(TEXT("Restart")));
	TestTrue(TEXT("Blink stayed where it was"), Keys.Get(TEXT("Blink")) == EKeys::LeftShift);
	TestTrue(TEXT("a testing key is refused"), Keys.Rebind(TEXT("Blink"), EKeys::H) == EWizardRebind::Refused);
	TestTrue(TEXT("Escape is refused: the browser keeps it"), Keys.Rebind(TEXT("Blink"), EKeys::Escape) == EWizardRebind::Refused);
	TestTrue(TEXT("the wheel is refused"), Keys.Rebind(TEXT("Blink"), EKeys::MouseWheelAxis) == EWizardRebind::Refused);
	TestTrue(TEXT("a wheel click is refused"), Keys.Rebind(TEXT("Blink"), EKeys::MouseScrollUp) == EWizardRebind::Refused);
	TestTrue(TEXT("a gamepad button is refused"), Keys.Rebind(TEXT("Blink"), EKeys::Gamepad_FaceButton_Bottom) == EWizardRebind::Refused);
	TestTrue(TEXT("a control that is not in the list cannot move"), Keys.Rebind(TEXT("Restart"), EKeys::T) == EWizardRebind::Refused);
	TestTrue(TEXT("nor one that does not exist"), Keys.Rebind(TEXT("Dance"), EKeys::T) == EWizardRebind::Refused);
	TestTrue(TEXT("after all that, every control still has its own key"), OneKeyEach(Keys));

	// Kept between runs as text.
	const TArray<FString> Saved = Keys.ToStrings();
	FWizardKeyMap Loaded;
	Loaded.FromStrings(Saved);
	for (const FName& Action : FWizardKeyMap::Changeable())
	{
		TestTrue(*FString::Printf(TEXT("%s comes back from the save on the same key"), *Action.ToString()), Loaded.Get(Action) == Keys.Get(Action));
	}
	FWizardKeyMap Broken;
	Broken.FromStrings({ TEXT("Blink=Q") });
	TestTrue(TEXT("a save that would put two controls on one key is thrown away"), Broken.IsDefault());
	Broken.FromStrings({ TEXT("Blink=Escape"), TEXT("Restart=T"), TEXT("nonsense"), TEXT("Bolt=NotAKey") });
	TestTrue(TEXT("a save with keys that are not allowed changes nothing"), Broken.IsDefault());

	Keys.Reset();
	TestTrue(TEXT("Reset puts every key back"), Keys.IsDefault() && Keys.Get(TEXT("Blink")) == EKeys::SpaceBar);

	TestEqual(TEXT("the left mouse button reads LMB"), FWizardKeyMap::KeyLabel(EKeys::LeftMouseButton), FString(TEXT("LMB")));
	TestEqual(TEXT("the space bar reads Space"), FWizardKeyMap::KeyLabel(EKeys::SpaceBar), FString(TEXT("Space")));
	TestEqual(TEXT("a letter reads as itself"), FWizardKeyMap::KeyLabel(EKeys::G), FString(TEXT("G")));
	TestEqual(TEXT("a side button reads Mouse 4"), FWizardKeyMap::KeyLabel(EKeys::ThumbMouseButton), FString(TEXT("Mouse 4")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoundVolumes, "WebTesting.Settings.SoundVolumesAndMutes", WIZ_TEST_FLAGS)
bool FSoundVolumes::RunTest(const FString& Parameters)
{
	FWizardSettings Settings;
	TestEqual(TEXT("master starts at 80"), Settings.Volume[0], 80);
	TestEqual(TEXT("music at 60"), Settings.Volume[1], 60);
	TestEqual(TEXT("effects at 90"), Settings.Volume[2], 90);
	TestEqual(TEXT("interface at 70"), Settings.Volume[3], 70);
	TestEqual(TEXT("a background tab is silent to begin with"), Settings.BackgroundMute, 1);
	TestTrue(TEXT("the loudness of the master is its volume"), FMath::IsNearlyEqual(Settings.Loudness(0), 0.8f));

	TestTrue(TEXT("a volume changes"), Settings.SetVolume(1, 33));
	TestEqual(TEXT("in steps of 5"), Settings.Volume[1], 35);
	Settings.SetVolume(1, 400);
	TestEqual(TEXT("up to 100"), Settings.Volume[1], 100);
	Settings.SetVolume(1, -3);
	TestEqual(TEXT("down to 0"), Settings.Volume[1], 0);
	TestFalse(TEXT("a channel that does not exist changes nothing"), Settings.SetVolume(9, 50));

	TestTrue(TEXT("a channel can be muted"), Settings.SetMuted(2, true));
	TestTrue(TEXT("muted, it is silent"), Settings.Loudness(2) == 0.0f);
	TestEqual(TEXT("and keeps its volume"), Settings.Volume[2], 90);
	TestFalse(TEXT("muting it again changes nothing"), Settings.SetMuted(2, true));
	TestTrue(TEXT("moving its volume takes the mute off"), Settings.SetVolume(2, 90));
	TestTrue(TEXT("and it is heard again"), FMath::IsNearlyEqual(Settings.Loudness(2), 0.9f));

	Settings.SetBackgroundMute(false);
	Settings.Set(EWizardDisplayRow::HudSize, 120);
	Settings.ResetSound();
	TestTrue(TEXT("Reset sound puts the volumes back"), Settings.Volume[1] == 60 && Settings.Muted[2] == 0 && Settings.BackgroundMute == 1);
	TestEqual(TEXT("and leaves the Display tab alone"), Settings.HudSize, 120);
	Settings.SetVolume(0, 10);
	Settings.ResetDisplay();
	TestTrue(TEXT("Reset display leaves the sound alone"), Settings.Volume[0] == 10 && Settings.HudSize == 100);
	TestEqual(TEXT("the names used in the console find their channels"), FWizardSettings::ChannelByName(TEXT("Effects")), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoundChannelAssets, "WebTesting.Settings.TheSoundChannelsExist", WIZ_TEST_FLAGS)
bool FSoundChannelAssets::RunTest(const FString& Parameters)
{
	USoundClass* Master = LoadObject<USoundClass>(nullptr, TEXT("/Game/Audio/Mix/SC_Master.SC_Master"));
	USoundMix* Mix = LoadObject<USoundMix>(nullptr, TEXT("/Game/Audio/Mix/MIX_Settings.MIX_Settings"));
	TestNotNull(TEXT("the master channel"), Master);
	TestNotNull(TEXT("the mix the volumes are set through"), Mix);
	const TCHAR* Children[] = { TEXT("SC_Music"), TEXT("SC_Effects"), TEXT("SC_Interface") };
	for (const TCHAR* Name : Children)
	{
		USoundClass* Child = LoadObject<USoundClass>(nullptr, *FString::Printf(TEXT("/Game/Audio/Mix/%s.%s"), Name, Name));
		TestNotNull(*FString::Printf(TEXT("the channel %s"), Name), Child);
		TestTrue(*FString::Printf(TEXT("%s is under the master, so the master scales it"), Name), Master && Child && Master->ChildClasses.Contains(Child) && Child->ParentClass == Master);
	}
	USoundWave* Effect = LoadObject<USoundWave>(nullptr, TEXT("/Game/Audio/SFX/SFX_Fire_Impact.SFX_Fire_Impact"));
	USoundWave* Click = LoadObject<USoundWave>(nullptr, TEXT("/Game/Audio/SFX/SFX_Ui_Sci_Fi_Click_Stutter_Soft_02.SFX_Ui_Sci_Fi_Click_Stutter_Soft_02"));
	USoundWave* Music = LoadObject<USoundWave>(nullptr, TEXT("/Game/Audio/Music/MUS_01_Menu_Darkness_Rises.MUS_01_Menu_Darkness_Rises"));
	TestTrue(TEXT("an effect is on the effects channel"), Effect && Effect->GetSoundClass() && Effect->GetSoundClass()->GetName() == TEXT("SC_Effects"));
	TestTrue(TEXT("the click is on the interface channel"), Click && Click->GetSoundClass() && Click->GetSoundClass()->GetName() == TEXT("SC_Interface"));
	TestTrue(TEXT("the music is on the music channel, and loops"), Music && Music->GetSoundClass() && Music->GetSoundClass()->GetName() == TEXT("SC_Music") && Music->bLooping);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKeysSoundLayout, "WebTesting.Hud.TheControlsAndSoundTabsAreClicked", WIZ_TEST_FLAGS)
bool FKeysSoundLayout::RunTest(const FString& Parameters)
{
	using namespace KeysSoundTest;
	for (const float* Size : Sizes)
	{
		const float Width = Size[0];
		const float Height = Size[1];
		const FBox2D Panel = AWizardHUD::SettingsPanelBox(Width, Height);
		const FBox2D Back = AWizardHUD::SettingsBackBox(Width, Height);
		TArray<FBox2D> KeyBoxes;
		for (int32 Index = 0; Index < FWizardKeyMap::Changeable().Num(); ++Index)
		{
			const FBox2D Box = AWizardHUD::SettingsKeyBox(Index, Width, Height);
			TestTrue(TEXT("a key is in the panel, over the footer"), Inside(Box, Panel) && Box.Max.Y <= Back.Min.Y);
			for (const FBox2D& Earlier : KeyBoxes)
			{
				TestTrue(TEXT("two keys do not overlap"), Apart(Box, Earlier));
			}
			KeyBoxes.Add(Box);
			const FWizardSettingsHit Hit = AWizardHUD::SettingsHitAt(Box.GetCenter(), EWizardSettingsTab::Controls, Width, Height);
			TestTrue(TEXT("a click on a key picks that key"), Hit.Kind == EWizardSettingsHitKind::Key && Hit.Row == Index);
			TestTrue(TEXT("keys are not there on the Display tab"), AWizardHUD::SettingsHitAt(Box.GetCenter(), EWizardSettingsTab::Display, Width, Height).Kind != EWizardSettingsHitKind::Key);
		}
		const FBox2D Fixed = AWizardHUD::SettingsFixedKeyBox(Width, Height);
		TestTrue(TEXT("Mouse, beside Aim, cannot be clicked"), AWizardHUD::SettingsHitAt(Fixed.GetCenter(), EWizardSettingsTab::Controls, Width, Height).Kind == EWizardSettingsHitKind::None);

		for (int32 Channel = 0; Channel < FWizardSettings::Channels; ++Channel)
		{
			const FBox2D Mute = AWizardHUD::SettingsMuteBox(Channel, Width, Height);
			const FBox2D Track = AWizardHUD::SettingsVolumeBox(Channel, Width, Height);
			TestTrue(TEXT("a mute button and its slider are in the panel"), Inside(Mute, Panel) && Inside(Track, Panel));
			TestTrue(TEXT("the slider is right of the mute button"), Track.Min.X >= Mute.Max.X);
			TestTrue(TEXT("the slider is long enough to drag"), Track.GetSize().X >= 80.0);
			FWizardSettingsHit Hit = AWizardHUD::SettingsHitAt(Mute.GetCenter(), EWizardSettingsTab::Sound, Width, Height);
			TestTrue(TEXT("a click on a mute button"), Hit.Kind == EWizardSettingsHitKind::Mute && Hit.Row == Channel);
			Hit = AWizardHUD::SettingsHitAt(Track.GetCenter(), EWizardSettingsTab::Sound, Width, Height);
			TestTrue(TEXT("a click on a volume slider takes hold of it"), Hit.Kind == EWizardSettingsHitKind::Volume && Hit.Row == Channel);
			TestEqual(TEXT("the left end is silent"), AWizardHUD::SettingsVolumeValue(Channel, static_cast<float>(Track.Min.X), Width, Height), 0);
			TestEqual(TEXT("the right end is full"), AWizardHUD::SettingsVolumeValue(Channel, static_cast<float>(Track.Max.X), Width, Height), 100);
			TestEqual(TEXT("the middle is half"), AWizardHUD::SettingsVolumeValue(Channel, static_cast<float>(Track.GetCenter().X), Width, Height), 50);
			TestTrue(TEXT("volume sliders are not there on the Controls tab"), AWizardHUD::SettingsHitAt(Track.GetCenter(), EWizardSettingsTab::Controls, Width, Height).Kind != EWizardSettingsHitKind::Volume);
		}
		for (int32 Index = 0; Index < 2; ++Index)
		{
			const FBox2D Box = AWizardHUD::SettingsBackgroundBox(Index, Width, Height);
			const FWizardSettingsHit Hit = AWizardHUD::SettingsHitAt(Box.GetCenter(), EWizardSettingsTab::Sound, Width, Height);
			TestTrue(TEXT("a click on a background choice"), Inside(Box, Panel) && Hit.Kind == EWizardSettingsHitKind::Background && Hit.Index == Index);
		}
		const FBox2D Test = AWizardHUD::SettingsTestSoundBox(Width, Height);
		TestTrue(TEXT("the test sound button is in the panel, clear of the choices"), Inside(Test, Panel) && Test.Min.X >= AWizardHUD::SettingsBackgroundBox(1, Width, Height).Max.X && Test.Max.Y <= Back.Min.Y);
		TestTrue(TEXT("a click on the test sound button"), AWizardHUD::SettingsHitAt(Test.GetCenter(), EWizardSettingsTab::Sound, Width, Height).Kind == EWizardSettingsHitKind::TestSound);
	}
	return true;
}

#endif
