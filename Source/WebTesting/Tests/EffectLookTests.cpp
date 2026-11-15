#include "Game/WizardAudioSubsystem.h"
#include "Game/WizardSounds.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "Tests/WizTestFlags.h"

#if WITH_DEV_AUTOMATION_TESTS

// Direction of 2026-11-15 01:50 PST:
// - "Change the bolt to be like a fireball like material that pulses."
// - "Replace the red warning effects of damage to look a little more on theme rather than a basic red material. Do
//   the same for when the player throws the spear and it shows the location. Change it to purple".
// - "If you turn the master/music volume down to 0 causes the music to completely stop. It should continue with no
//   volume so if it's put back up, its heard."

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEffectMaterials, "WebTesting.Effects.TheBoltAndTheGroundMarksHaveTheirOwnMaterials", WIZ_TEST_FLAGS)
bool FEffectMaterials::RunTest(const FString& Parameters)
{
	for (const TCHAR* Path : { TEXT("/Game/Level/M_FireBolt.M_FireBolt"), TEXT("/Game/Level/M_GroundMark.M_GroundMark") })
	{
		const UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, Path);
		TestNotNull(Path, Material);
		if (!Material)
		{
			continue;
		}
		// They glow by themselves and are see-through at their edges, and are drawn many at once.
		TestEqual(TEXT("see-through"), static_cast<int32>(Material->GetBlendMode()), static_cast<int32>(BLEND_Translucent));
		TestTrue(TEXT("unlit"), Material->GetShadingModels().HasShadingModel(MSM_Unlit));
		const UMaterial* Base = Material->GetMaterial();
		TestTrue(TEXT("usable on instanced meshes"), Base && Base->GetUsageByFlag(MATUSAGE_InstancedStaticMeshes));
	}
	const UMaterialInterface* Bolt = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Level/M_FireBolt.M_FireBolt"));
	float Pulse = 0.0f;
	TestTrue(TEXT("the Bolt pulses"), Bolt && Bolt->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("Pulse")), Pulse) && Pulse > 0.0f);
	const UMaterialInterface* Mark = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Level/M_GroundMark.M_GroundMark"));
	for (const TCHAR* Name : { TEXT("Round"), TEXT("Fill"), TEXT("Pulse"), TEXT("Spin"), TEXT("Reps"), TEXT("Arc") })
	{
		float Value = 0.0f;
		TestTrue(Name, Mark && Mark->GetScalarParameterValue(FHashedMaterialParameterInfo(Name), Value));
	}
	for (const TCHAR* Name : { TEXT("Color"), TEXT("Glow") })
	{
		FLinearColor Value;
		TestTrue(Name, Mark && Mark->GetVectorParameterValue(FHashedMaterialParameterInfo(Name), Value));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMusicWhenSilent, "WebTesting.Sound.MusicPlaysOnWhenItsVolumeIsNothing", WIZ_TEST_FLAGS)
bool FMusicWhenSilent::RunTest(const FString& Parameters)
{
	for (const EWizardMusic Music : { EWizardMusic::Menu, EWizardMusic::Game, EWizardMusic::End })
	{
		USoundBase* Sound = LoadObject<USoundBase>(nullptr, FWizardSoundRules::MusicPath(Music));
		TestNotNull(FWizardSoundRules::MusicName(Music), Sound);
		if (!Sound)
		{
			continue;
		}
		const EVirtualizationMode Before = Sound->VirtualizationMode;
		UWizardAudioSubsystem::KeepPlayingWhenSilent(Sound);
		TestEqual(FWizardSoundRules::MusicName(Music), static_cast<int32>(Sound->VirtualizationMode), static_cast<int32>(EVirtualizationMode::PlayWhenSilent));
		Sound->VirtualizationMode = Before;
	}
	// The slider's nothing is given to the player as next to nothing (far below hearing), never as nothing: a
	// piece at exactly nothing is one the engine may let go of.
	const float Least = UWizardAudioSubsystem::MusicPlayerVolume(0.0f);
	TestTrue(TEXT("nothing is unheard, and not nothing"), Least > 0.0f && Least <= 0.0002f);
	TestEqual(TEXT("a volume is itself"), UWizardAudioSubsystem::MusicPlayerVolume(0.48f), 0.48f);
	return true;
}

#endif
