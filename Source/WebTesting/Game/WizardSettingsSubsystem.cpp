#include "Game/WizardSettingsSubsystem.h"
#include "Game/WizardAudioSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Game/WizardHUD.h"
#include "Game/WizardKeys.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Scalability.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

const TCHAR* UWizardSettingsSubsystem::SaveSlot = TEXT("WizardSettings");

void UWizardSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Settings = FWizardSettings();
	bGraphicsTouched = false;
	LoadChannels();
	if (!FApp::IsUnattended() && LoadFrom(SaveSlot))
	{
		// What was saved is how the player wants the game drawn: it is put in force, quality and all.
		bGraphicsTouched = true;
	}
	Apply(bGraphicsTouched);
}

bool UWizardSettingsSubsystem::Set(EWizardDisplayRow Row, int32 Value)
{
	if (!Settings.Set(Row, Value))
	{
		return false;
	}
	const bool bGraphics = Row == EWizardDisplayRow::Quality || Row == EWizardDisplayRow::RenderScale;
	bGraphicsTouched |= bGraphics;
	Apply(bGraphics);
	SaveNow();
	return true;
}

void UWizardSettingsSubsystem::SaveNow() const
{
	if (bKeep && !FApp::IsUnattended())
	{
		SaveTo(SaveSlot);
	}
}

void UWizardSettingsSubsystem::LoadChannels()
{
	// Made by Tools/Audio/import_sounds.py. A sound is on a channel when its Sound Class is that channel's.
	const TCHAR* Paths[] = {
		TEXT("/Game/Audio/Mix/SC_Master.SC_Master"), TEXT("/Game/Audio/Mix/SC_Music.SC_Music"),
		TEXT("/Game/Audio/Mix/SC_Effects.SC_Effects"), TEXT("/Game/Audio/Mix/SC_Interface.SC_Interface"),
	};
	ChannelClasses.Reset();
	for (const TCHAR* Path : Paths)
	{
		ChannelClasses.Add(LoadObject<USoundClass>(nullptr, Path));
	}
	ChannelMix = LoadObject<USoundMix>(nullptr, TEXT("/Game/Audio/Mix/MIX_Settings.MIX_Settings"));
}

bool UWizardSettingsSubsystem::AreChannelsReady() const
{
	return ChannelMix && ChannelClasses.Num() == FWizardSettings::Channels && !ChannelClasses.Contains(nullptr);
}

void UWizardSettingsSubsystem::ApplySound()
{
	// Silent, or not, while the window or tab is not in front: the engine's own switch.
	// Not in the editor: there it is the editor's own setting, and it would outlast the game and silence the editor
	// whenever another window is in front.
	if (!GIsEditor)
	{
		FApp::SetUnfocusedVolumeMultiplier(Settings.BackgroundMute != 0 ? 0.0f : 1.0f);
	}
	// The volumes are applied where the game starts each sound (UWizardAudioSubsystem), and the music is told now.
	// (They went through an engine sound mix before; in the editor the sliders then did nothing that could be heard.)
	if (UWizardAudioSubsystem* Audio = GetGameInstance() ? GetGameInstance()->GetSubsystem<UWizardAudioSubsystem>() : nullptr)
	{
		Audio->RefreshVolumes();
	}
}

bool UWizardSettingsSubsystem::PlayTestSound(int32 Channel)
{
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	const TCHAR* Path = Channel == static_cast<int32>(EWizardSoundChannel::Interface)
		? TEXT("/Game/Audio/SFX/SFX_Ui_Sci_Fi_Click_Stutter_Soft_02.SFX_Ui_Sci_Fi_Click_Stutter_Soft_02")
		: TEXT("/Game/Audio/SFX/SFX_Fire_Impact.SFX_Fire_Impact");
	USoundBase* Sound = LoadObject<USoundBase>(nullptr, Path);
	if (!World || !Sound)
	{
		return false;
	}
	// A "UI sound": it plays while the game is paused, which is when the settings are open.
	const UWizardAudioSubsystem* Audio = GetGameInstance()->GetSubsystem<UWizardAudioSubsystem>();
	const float Loud = Audio ? Audio->GetChannelVolume(Channel == static_cast<int32>(EWizardSoundChannel::Interface) ? 3 : 2) : 1.0f;
	if (Loud > 0.001f)
	{
		UGameplayStatics::PlaySound2D(World, Sound, Loud, 1.0f, 0.0f, nullptr, nullptr, true);
	}
	++TestSoundsPlayed;
	return true;
}

bool UWizardSettingsSubsystem::SetVolume(int32 Channel, int32 Value)
{
	if (!Settings.SetVolume(Channel, Value))
	{
		return false;
	}
	ApplySound();
	SaveNow();
	return true;
}

bool UWizardSettingsSubsystem::SetMuted(int32 Channel, bool bMuted)
{
	if (!Settings.SetMuted(Channel, bMuted))
	{
		return false;
	}
	ApplySound();
	SaveNow();
	return true;
}

bool UWizardSettingsSubsystem::SetBackgroundMute(bool bMute)
{
	if (!Settings.SetBackgroundMute(bMute))
	{
		return false;
	}
	ApplySound();
	SaveNow();
	return true;
}

void UWizardSettingsSubsystem::ResetSound()
{
	Settings.ResetSound();
	ApplySound();
	SaveNow();
}

void UWizardSettingsSubsystem::ResetDisplay()
{
	FWizardSettings Defaults = Settings;
	Defaults.ResetDisplay();
	if (Settings == Defaults)
	{
		return;
	}
	const bool bGraphics = Settings.Quality != Defaults.Quality || Settings.RenderScale != Defaults.RenderScale;
	Settings = Defaults;
	bGraphicsTouched |= bGraphics;
	Apply(bGraphics);
	SaveNow();
}

void UWizardSettingsSubsystem::Apply(bool bGraphics)
{
	AWizardHUD::SetSizeFactor(Settings.HudSize / 100.0f);
	ApplySound();
	if (bGraphics)
	{
		// Low, medium, high are the engine's levels 1, 2 and 3: 3 is what the game ran at before it had this screen.
		// The engine ties a render scale to each level; the player's own is put back over it.
		Scalability::FQualityLevels Levels = Scalability::GetQualityLevels();
		Levels.SetFromSingleQualityLevel(Settings.Quality + 1);
		Levels.ResolutionQuality = static_cast<float>(Settings.RenderScale);
		Scalability::SetQualityLevels(Levels);
	}
}

bool UWizardSettingsSubsystem::SaveTo(const FString& Slot) const
{
	UWizardSettingsSave* Save = Cast<UWizardSettingsSave>(UGameplayStatics::CreateSaveGameObject(UWizardSettingsSave::StaticClass()));
	if (!Save)
	{
		return false;
	}
	Save->Quality = Settings.Quality;
	Save->RenderScale = Settings.RenderScale;
	Save->HudSize = Settings.HudSize;
	Save->DangerEdges = Settings.DangerEdges;
	Save->FrameRate = Settings.FrameRate;
	Save->Volumes.Append(Settings.Volume, FWizardSettings::Channels);
	Save->Muted.Append(Settings.Muted, FWizardSettings::Channels);
	Save->BackgroundMute = Settings.BackgroundMute;
	Save->Keys = WizardKeys().ToStrings();
	return UGameplayStatics::SaveGameToSlot(Save, Slot, 0);
}

bool UWizardSettingsSubsystem::LoadFrom(const FString& Slot)
{
	if (!UGameplayStatics::DoesSaveGameExist(Slot, 0))
	{
		return false;
	}
	const UWizardSettingsSave* Save = Cast<UWizardSettingsSave>(UGameplayStatics::LoadGameFromSlot(Slot, 0));
	if (!Save)
	{
		return false;
	}
	Settings.Quality = Save->Quality;
	Settings.RenderScale = Save->RenderScale;
	Settings.HudSize = Save->HudSize;
	Settings.DangerEdges = Save->DangerEdges;
	Settings.FrameRate = Save->FrameRate;
	// A save from before the game had sound settings or changeable keys holds none: the defaults stay.
	if (Save->Volumes.Num() == FWizardSettings::Channels && Save->Muted.Num() == FWizardSettings::Channels)
	{
		for (int32 Channel = 0; Channel < FWizardSettings::Channels; ++Channel)
		{
			Settings.Volume[Channel] = Save->Volumes[Channel];
			Settings.Muted[Channel] = Save->Muted[Channel];
		}
		Settings.BackgroundMute = Save->BackgroundMute;
	}
	WizardKeys().FromStrings(Save->Keys);
	Settings.Clamp();
	return true;
}
