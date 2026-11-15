#pragma once

#include "CoreMinimal.h"
#include "Game/WizardSounds.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WizardAudioSubsystem.generated.h"

class UAudioComponent;
class USoundBase;

/**
 * Plays the game's music and sound effects. It lives with the game instance, so the music carries on through a
 * restart (which loads the level again). Music changes by fading the old piece out while the new one fades in.
 */
UCLASS()
class UWizardAudioSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** The music that should be playing. A change fades the old out and the new in over FWizardSoundRules::MusicFadeSeconds. */
	void SetMusic(EWizardMusic Music);

	/** For the console: "menu", "game", "end" or "none". False for any other word. */
	UFUNCTION(BlueprintCallable, Category = "Wizard|Audio")
	bool SetMusicByName(const FString& Name);

	EWizardMusic GetMusic() const { return Music; }

	UFUNCTION(BlueprintPure, Category = "Wizard|Audio")
	FString GetMusicName() const { return FWizardSoundRules::MusicName(Music); }

	/** How many times the music has changed since the game started. */
	UFUNCTION(BlueprintPure, Category = "Wizard|Audio")
	int32 GetMusicChanges() const { return MusicChanges; }

	/** Plays a sound effect at full loudness. False when the same one started too short a time ago, or it is missing. */
	bool PlayCue(EWizardCue Cue, float Volume = 1.0f, float SkipSeconds = 0.0f);

	/**
	 * How loud a settings channel is now (0 master, 1 music, 2 effects, 3 interface): its own volume times the
	 * master's, nothing when either is muted. Every sound the game starts is scaled by this here, so the sliders
	 * do not depend on the engine's sound mixes.
	 */
	float GetChannelVolume(int32 Channel) const;

	/** The settings changed: the music takes its new loudness. */
	void RefreshVolumes();

	/**
	 * A piece of music goes on playing while its volume is nothing, so it is there when the volume comes back
	 * (the engine otherwise lets go of a sound nobody can hear).
	 */
	static void KeepPlayingWhenSilent(USoundBase* Sound);

	/** What the music's player is given for a channel volume: never quite nothing (see KeepPlayingWhenSilent). */
	static float MusicPlayerVolume(float ChannelVolume) { return FMath::Max(ChannelVolume, 0.0001f); }

	/** Sounds started at a place in the world (in 3D), and in all. */
	UFUNCTION(BlueprintPure, Category = "Wizard|Audio")
	int32 GetPlacedCuePlays() const { return PlacedPlays; }

	/** Plays a sound effect that happens at a place: heard from that side, quieter the farther it is from the listener. */
	bool PlayCueAt(EWizardCue Cue, FVector2D Position, FVector2D Listener, float SkipSeconds = 0.0f);

	/**
	 * As PlayCueAt, for a sound whose source moves (the commander's spear): gives back its player, to be moved with
	 * MoveFollowed. Null when nothing is heard (too far, silent, or a run with nobody at it).
	 */
	UAudioComponent* PlayCueFollowed(EWizardCue Cue, FVector2D Position, FVector2D Listener);

	/** Moves a sound started with PlayCueFollowed to where its source is now. Counted even when there is no player. */
	void MoveFollowed(UAudioComponent* Player, FVector2D Position);

	/** Sounds started that follow what makes them, and how many times one has been moved. */
	UFUNCTION(BlueprintPure, Category = "Wizard|Audio")
	int32 GetFollowedCuePlays() const { return FollowedPlays; }

	UFUNCTION(BlueprintPure, Category = "Wizard|Audio")
	int32 GetFollowedMoves() const { return FollowedMoves; }

	/** How many times a sound effect has started. */
	int32 GetCuePlays(EWizardCue Cue) const;

	UFUNCTION(BlueprintPure, Category = "Wizard|Audio")
	int32 GetTotalCuePlays() const { return TotalPlays; }

	/** The name of the last sound effect that started, or "none". */
	UFUNCTION(BlueprintPure, Category = "Wizard|Audio")
	FString GetLastCueName() const { return TotalPlays > 0 ? FWizardSoundRules::Info(LastCue).Name : TEXT("none"); }

	/** For the console: plays a cue by name. */
	UFUNCTION(BlueprintCallable, Category = "Wizard|Audio")
	bool PlayCueByName(const FString& Name);

private:
	USoundBase* LoadCue(EWizardCue Cue);

	EWizardMusic Music = EWizardMusic::None;
	int32 MusicChanges = 0;

	// The piece that is playing, and those still fading out.
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> MusicPlayer;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UAudioComponent>> FadingOut;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> Cues[static_cast<int32>(EWizardCue::Count)];

	bool bCueTried[static_cast<int32>(EWizardCue::Count)] = {};
	int32 CuePlays[static_cast<int32>(EWizardCue::Count)] = {};
	int32 TotalPlays = 0;
	int32 PlacedPlays = 0;

	int32 FollowedPlays = 0;
	int32 FollowedMoves = 0;

	bool Start(EWizardCue Cue, float Volume, float SkipSeconds, const FVector* Place, UAudioComponent** OutPlayer = nullptr);
	FVector PlaceOf(FVector2D Position) const;

	UPROPERTY(Transient)
	TObjectPtr<class USoundAttenuation> Placed;
	EWizardCue LastCue = EWizardCue::Denied;
	FWizardCueThrottle Throttle;
};
