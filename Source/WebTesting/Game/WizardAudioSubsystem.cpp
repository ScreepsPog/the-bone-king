#include "Game/WizardAudioSubsystem.h"

#include "Components/AudioComponent.h"
#include "Engine/GameInstance.h"
#include "Game/WizardSettingsSubsystem.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Sound/SoundAttenuation.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Sound/SoundBase.h"

void UWizardAudioSubsystem::SetMusic(EWizardMusic NewMusic)
{
	if (NewMusic == Music)
	{
		return;
	}
	Music = NewMusic;
	++MusicChanges;

	// The old piece fades out, softly; it is let go once it is silent.
	FadingOut.RemoveAll([](const TObjectPtr<UAudioComponent>& Player) { return !IsValid(Player) || !Player->IsPlaying(); });
	if (IsValid(MusicPlayer))
	{
		MusicPlayer->FadeOut(FWizardSoundRules::MusicFadeSeconds, 0.0f);
		FadingOut.Add(MusicPlayer);
	}
	MusicPlayer = nullptr;

	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	const TCHAR* Path = FWizardSoundRules::MusicPath(Music);
	// A run with nobody at it (the tests) keeps count of what it would play and stays silent.
	USoundBase* Sound = Path && !FApp::IsUnattended() ? LoadObject<USoundBase>(nullptr, Path) : nullptr;
	if (!World || !Sound)
	{
		return;
	}
	// It outlives the level (a restart loads the level again) and plays while the game is paused (the menus pause it).
	KeepPlayingWhenSilent(Sound);
	MusicPlayer = UGameplayStatics::CreateSound2D(World, Sound, 1.0f, 1.0f, 0.0f, nullptr, true, false);
	if (MusicPlayer)
	{
		MusicPlayer->bIsUISound = true;
		MusicPlayer->bAutoDestroy = false;
		MusicPlayer->FadeIn(FWizardSoundRules::MusicFadeSeconds, MusicPlayerVolume(GetChannelVolume(1)));
	}
}

USoundBase* UWizardAudioSubsystem::LoadCue(EWizardCue Cue)
{
	const int32 Index = static_cast<int32>(Cue);
	if (Index < 0 || Index >= static_cast<int32>(EWizardCue::Count))
	{
		return nullptr;
	}
	if (!bCueTried[Index])
	{
		bCueTried[Index] = true;
		Cues[Index] = LoadObject<USoundBase>(nullptr, FWizardSoundRules::Info(Cue).Path);
	}
	return Cues[Index];
}

float UWizardAudioSubsystem::GetChannelVolume(int32 Channel) const
{
	const UWizardSettingsSubsystem* Store = GetGameInstance() ? GetGameInstance()->GetSubsystem<UWizardSettingsSubsystem>() : nullptr;
	if (!Store)
	{
		return 1.0f;
	}
	return Channel == 0 ? Store->GetLoudness(0) : Store->GetLoudness(0) * Store->GetLoudness(Channel);
}

void UWizardAudioSubsystem::RefreshVolumes()
{
	if (IsValid(MusicPlayer))
	{
		MusicPlayer->AdjustVolume(0.1f, MusicPlayerVolume(GetChannelVolume(1)));
	}
}

void UWizardAudioSubsystem::KeepPlayingWhenSilent(USoundBase* Sound)
{
	if (Sound)
	{
		// Only for this run: the asset is not marked changed and is not saved.
		Sound->VirtualizationMode = EVirtualizationMode::PlayWhenSilent;
	}
}

bool UWizardAudioSubsystem::Start(EWizardCue Cue, float Volume, float SkipSeconds, const FVector* Place, UAudioComponent** OutPlayer)
{
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!World || Cue == EWizardCue::Count)
	{
		return false;
	}
	// Real time, so a sound in a menu (the game paused) is held apart the same way.
	if (!Throttle.Allow(Cue, World->GetRealTimeSeconds()))
	{
		return false;
	}
	const int32 Index = static_cast<int32>(Cue);
	++CuePlays[Index];
	++TotalPlays;
	PlacedPlays += Place ? 1 : 0;
	LastCue = Cue;
	USoundBase* Sound = LoadCue(Cue);
	if (!Sound)
	{
		return false;
	}
	if (FApp::IsUnattended())
	{
		// A run with nobody at it (the tests): counted, not heard.
		return true;
	}
	const float Loud = Volume * GetChannelVolume(FWizardSoundRules::ChannelOf(Cue));
	if (Loud <= 0.001f)
	{
		return true;
	}
	const FWizardCueInfo& Info = FWizardSoundRules::Info(Cue);
	const float From = Info.StartSeconds + SkipSeconds;
	if (Place)
	{
		if (!Placed)
		{
			// Full within NearDistance of the ears, falling to nothing at FarDistance, and heard from its side.
			Placed = NewObject<USoundAttenuation>(this);
			FSoundAttenuationSettings& Rules = Placed->Attenuation;
			Rules.bAttenuate = true;
			Rules.bSpatialize = true;
			Rules.AttenuationShape = EAttenuationShape::Sphere;
			Rules.AttenuationShapeExtents = FVector(FWizardSoundRules::NearDistance);
			Rules.FalloffDistance = static_cast<float>(FWizardSoundRules::FarDistance - FWizardSoundRules::NearDistance);
			Rules.NonSpatializedRadiusStart = FWizardSoundRules::MiddleFrom;
			Rules.NonSpatializedRadiusEnd = FWizardSoundRules::MiddleAt;
		}
		if (Info.PlaySeconds > 0.0f || OutPlayer)
		{
			if (UAudioComponent* Player = UGameplayStatics::SpawnSoundAtLocation(World, Sound, *Place, FRotator::ZeroRotator, Loud, 1.0f, From, Placed))
			{
				if (Info.PlaySeconds > 0.0f)
				{
					Player->StopDelayed(Info.PlaySeconds);
				}
				if (OutPlayer)
				{
					*OutPlayer = Player;
				}
			}
		}
		else
		{
			UGameplayStatics::PlaySoundAtLocation(World, Sound, *Place, FRotator::ZeroRotator, Loud, 1.0f, From, Placed);
		}
	}
	else if (Info.PlaySeconds > 0.0f)
	{
		// Only part of the sound: it is stopped when that part has played.
		if (UAudioComponent* Player = UGameplayStatics::SpawnSound2D(World, Sound, Loud, 1.0f, From, nullptr, false, true))
		{
			Player->bIsUISound = true;
			Player->StopDelayed(Info.PlaySeconds);
		}
	}
	else
	{
		UGameplayStatics::PlaySound2D(World, Sound, Loud, 1.0f, From, nullptr, nullptr, true);
	}
	return true;
}

bool UWizardAudioSubsystem::PlayCue(EWizardCue Cue, float Volume, float SkipSeconds)
{
	return Volume > 0.01f && Start(Cue, Volume, SkipSeconds, nullptr);
}

bool UWizardAudioSubsystem::PlayCueAt(EWizardCue Cue, FVector2D Position, FVector2D Listener, float SkipSeconds)
{
	// Too far to be heard: not started at all.
	if (FWizardSoundRules::DistanceVolume(FVector2D::Distance(Position, Listener)) <= 0.0f)
	{
		return false;
	}
	if (FWizardSoundRules::IsPlayersOwn(Cue))
	{
		return Start(Cue, 1.0f, SkipSeconds, nullptr);
	}
	const FVector Place = PlaceOf(Position);
	return Start(Cue, 1.0f, SkipSeconds, &Place);
}

FVector UWizardAudioSubsystem::PlaceOf(FVector2D Position) const
{
	// At its place on the ground, level with the ears (which are where the wizard stands).
	const UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	const APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
	const APawn* Pawn = Player ? Player->GetPawn() : nullptr;
	return FVector(Position.X, Position.Y, Pawn ? Pawn->GetActorLocation().Z : 0.0);
}

UAudioComponent* UWizardAudioSubsystem::PlayCueFollowed(EWizardCue Cue, FVector2D Position, FVector2D Listener)
{
	if (FWizardSoundRules::DistanceVolume(FVector2D::Distance(Position, Listener)) <= 0.0f)
	{
		return nullptr;
	}
	const FVector Place = PlaceOf(Position);
	UAudioComponent* Player = nullptr;
	if (Start(Cue, 1.0f, 0.0f, &Place, &Player))
	{
		++FollowedPlays;
	}
	return Player;
}

void UWizardAudioSubsystem::MoveFollowed(UAudioComponent* Player, FVector2D Position)
{
	++FollowedMoves;
	if (Player)
	{
		Player->SetWorldLocation(PlaceOf(Position));
	}
}

int32 UWizardAudioSubsystem::GetCuePlays(EWizardCue Cue) const
{
	const int32 Index = static_cast<int32>(Cue);
	return Index >= 0 && Index < static_cast<int32>(EWizardCue::Count) ? CuePlays[Index] : 0;
}

bool UWizardAudioSubsystem::SetMusicByName(const FString& Name)
{
	for (const EWizardMusic Each : { EWizardMusic::None, EWizardMusic::Menu, EWizardMusic::Game, EWizardMusic::End })
	{
		if (Name.Equals(FWizardSoundRules::MusicName(Each), ESearchCase::IgnoreCase))
		{
			SetMusic(Each);
			return true;
		}
	}
	return false;
}

bool UWizardAudioSubsystem::PlayCueByName(const FString& Name)
{
	const EWizardCue Cue = FWizardSoundRules::CueByName(Name);
	return Cue != EWizardCue::Count && PlayCue(Cue);
}
