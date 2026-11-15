#pragma once

#include "CoreMinimal.h"
#include "Game/WizardSettings.h"
#include "GameFramework/SaveGame.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WizardSettingsSubsystem.generated.h"

/** The settings as they are written to the save slot. */
UCLASS()
class UWizardSettingsSave : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	int32 Quality = FWizardSettings::DefaultQuality;

	UPROPERTY()
	int32 RenderScale = 100;

	UPROPERTY()
	int32 HudSize = 100;

	UPROPERTY()
	int32 DangerEdges = 100;

	UPROPERTY()
	int32 FrameRate = 0;

	// One for each sound channel: master, music, effects, interface.
	UPROPERTY()
	TArray<int32> Volumes;

	UPROPERTY()
	TArray<int32> Muted;

	UPROPERTY()
	int32 BackgroundMute = 1;

	// The keys the player moved, as "Control=Key".
	UPROPERTY()
	TArray<FString> Keys;
};

/**
 * Holds the player's settings for as long as the game runs, applies each change at once, and keeps them in a save
 * slot (in a browser: the browser's storage for this page) so they are back on the next visit.
 * Unattended runs (tests, scenarios) neither load nor save, so a tester's own settings never change a test.
 */
UCLASS()
class UWizardSettingsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	const FWizardSettings& Get() const { return Settings; }

	/** Sets one row, applies it and saves. True when the value changed. */
	bool Set(EWizardDisplayRow Row, int32 Value);

	/** The Display tab back to its defaults, applied and saved. */
	UFUNCTION(BlueprintCallable, Category = "Wizard|Settings")
	void ResetDisplay();

	/** A sound channel's volume (0 master, 1 music, 2 effects, 3 interface), 0 to 100; applied and saved. */
	UFUNCTION(BlueprintCallable, Category = "Wizard|Settings")
	bool SetVolume(int32 Channel, int32 Value);

	UFUNCTION(BlueprintCallable, Category = "Wizard|Settings")
	bool SetMuted(int32 Channel, bool bMuted);

	/** True: silent while the game's window or browser tab is not in front. */
	UFUNCTION(BlueprintCallable, Category = "Wizard|Settings")
	bool SetBackgroundMute(bool bMute);

	/** The Sound tab back to its defaults, applied and saved. */
	UFUNCTION(BlueprintCallable, Category = "Wizard|Settings")
	void ResetSound();

	UFUNCTION(BlueprintPure, Category = "Wizard|Settings")
	int32 GetVolume(int32 Channel) const { return Channel >= 0 && Channel < FWizardSettings::Channels ? Settings.Volume[Channel] : 0; }

	UFUNCTION(BlueprintPure, Category = "Wizard|Settings")
	bool IsMuted(int32 Channel) const { return Channel >= 0 && Channel < FWizardSettings::Channels && Settings.Muted[Channel] != 0; }

	/** What the channel's sound class is set to, 0 to 1. */
	UFUNCTION(BlueprintPure, Category = "Wizard|Settings")
	float GetLoudness(int32 Channel) const { return Settings.Loudness(Channel); }

	UFUNCTION(BlueprintPure, Category = "Wizard|Settings")
	bool IsBackgroundMuted() const { return Settings.BackgroundMute != 0; }

	/** True when the four sound classes and the mix were found: without them the volumes change nothing. */
	UFUNCTION(BlueprintPure, Category = "Wizard|Settings")
	bool AreChannelsReady() const;

	/** Puts the volumes in force in the world that is running. Called when a level starts, and on every change. */
	void ApplySound();

	/** Plays one short sound on a channel (effects or interface), also while paused, so a volume can be judged. False when there is no sound to play. */
	UFUNCTION(BlueprintCallable, Category = "Wizard|Settings")
	bool PlayTestSound(int32 Channel);

	/** How many test sounds were played: for tests, which cannot hear. */
	UFUNCTION(BlueprintPure, Category = "Wizard|Settings")
	int32 GetTestSoundsPlayed() const { return TestSoundsPlayed; }

	/** Writes everything (the settings and the keys) to the player's save slot, unless this run keeps nothing. */
	void SaveNow() const;

	/** 0 low, 1 medium, 2 high. */
	UFUNCTION(BlueprintPure, Category = "Wizard|Settings")
	int32 GetQuality() const { return Settings.Quality; }

	/** Percent of the window's pixels the game is drawn at, 50 to 100. */
	UFUNCTION(BlueprintPure, Category = "Wizard|Settings")
	int32 GetRenderScale() const { return Settings.RenderScale; }

	/** Percent, 80 to 130, on top of the size the window gives the HUD and the menus. */
	UFUNCTION(BlueprintPure, Category = "Wizard|Settings")
	int32 GetHudSize() const { return Settings.HudSize; }

	/** Percent of the red edges' strength, 0 to 100. */
	UFUNCTION(BlueprintPure, Category = "Wizard|Settings")
	int32 GetDangerEdges() const { return Settings.DangerEdges; }

	UFUNCTION(BlueprintPure, Category = "Wizard|Settings")
	bool IsFrameRateShown() const { return Settings.FrameRate != 0; }

	/** True once quality or render scale were changed or loaded: until then the engine's own are left as they are. */
	UFUNCTION(BlueprintPure, Category = "Wizard|Settings")
	bool AreGraphicsApplied() const { return bGraphicsTouched; }

	/** Writes the settings to a save slot, and reads them back. False when the slot could not be written or holds none. */
	bool SaveTo(const FString& Slot) const;
	bool LoadFrom(const FString& Slot);

	/** Off, a change is not written to the player's save slot (tests use a slot of their own). */
	void SetKeptBetweenRuns(bool bKept) { bKeep = bKept; }

	static const TCHAR* SaveSlot;

private:
	void Apply(bool bGraphics);
	void LoadChannels();

	// The four sound classes, in EWizardSoundChannel's order, and the mix their volumes are set through.
	UPROPERTY()
	TArray<TObjectPtr<class USoundClass>> ChannelClasses;

	UPROPERTY()
	TObjectPtr<class USoundMix> ChannelMix;

	TWeakObjectPtr<UWorld> MixPushedFor;
	int32 TestSoundsPlayed = 0;

	FWizardSettings Settings;
	bool bGraphicsTouched = false;
	bool bKeep = true;
};
