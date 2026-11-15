#pragma once

#include "CoreMinimal.h"
#include "Sim/CrowdTypes.h"

/** Every sound effect the game plays, by what happens. (Direction of 2026-11-14 22:30 PST.) */
enum class EWizardCue : uint8
{
	// Something the player asked for was refused.
	Denied,
	Blink,
	// A skeleton's blow on a soldier, an archer or an assassin; and on a brute or the commander.
	SwordHitsEnemy,
	SwordHitsArmor,
	Summoned,
	SkeletonDies,
	// A brute's slam that lands on nobody.
	HammerHitsGround,
	FireThrow,
	FireImpact,
	DaggerMiss,
	BowShoot,
	BodyFallDirt1,
	BodyFallDirt2,
	BodyFallGrass1,
	BodyFallGrass2,
	// A pile of bones picked up; the kingdom gate opening; the commander's spear leaving his hand.
	BonePickup,
	GateOpen,
	CommanderSpearThrow,
	Count,
};

/** Which music plays: the start menu's, the game's, the end's. */
enum class EWizardMusic : uint8
{
	None,
	Menu,
	Game,
	End,
};

struct FWizardCueInfo
{
	// The name a tester uses for it.
	const TCHAR* Name;
	const TCHAR* Path;
	// Where in the sound it starts, and how much of it plays (0: all of it).
	float StartSeconds;
	float PlaySeconds;
	// The same cue is not started again within this long: thirty blows in one frame are one sound, not thirty.
	float MinGap;
	// Seconds of quiet at the front of the sound before anything is heard (measured on the file). A sound that has
	// to be heard at a moment is started this much before it.
	float HeardFrom = 0.0f;
};

/** The rules for sound, with no engine objects, so they can be tested without a world. */
struct FWizardSoundRules
{
	/** Music fades in over this long and out over this long: a change of music is the two together. */
	static constexpr float MusicFadeSeconds = 2.5f;

	/** A sound this near the wizard is at full loudness; from there it falls away to nothing at FarDistance. */
	static constexpr double NearDistance = 900.0;
	static constexpr double FarDistance = 3200.0;
	/**
	 * A sound that happens at a place is heard from that side (director, 2026-11-15 01:00 PST: "play at location so
	 * there is 3d to the audio"). The ears are where the wizard stands, turned the way the camera looks. Nearer than
	 * this to him a sound stops being all on one side and comes to the middle.
	 */
	static constexpr float MiddleFrom = 350.0f;
	static constexpr float MiddleAt = 120.0f;

	/**
	 * True for a sound made by the player himself or the interface: it is not placed in the world. A pile of bones is
	 * picked up under his feet. The gate's opening is here too: it tells him the way is open, so it is heard at full
	 * loudness wherever he stands.
	 */
	static bool IsPlayersOwn(EWizardCue Cue)
	{
		return Cue == EWizardCue::Denied || Cue == EWizardCue::Blink || Cue == EWizardCue::Summoned || Cue == EWizardCue::FireThrow
			|| Cue == EWizardCue::BonePickup || Cue == EWizardCue::GateOpen;
	}

	/** The settings channel a sound is on: 3 the interface (the refused click), 2 effects. */
	static int32 ChannelOf(EWizardCue Cue) { return Cue == EWizardCue::Denied ? 3 : 2; }

	static const FWizardCueInfo& Info(EWizardCue Cue)
	{
		static const FWizardCueInfo Infos[] = {
			{ TEXT("denied"), TEXT("/Game/Audio/SFX/SFX_Ui_Sci_Fi_Click_Stutter_Soft_02.SFX_Ui_Sci_Fi_Click_Stutter_Soft_02"), 0.0f, 0.0f, 0.2f },
			{ TEXT("blink"), TEXT("/Game/Audio/SFX/SFX_Wizard_Blink.SFX_Wizard_Blink"), 0.0f, 0.0f, 0.1f },
			{ TEXT("swordhitsenemy"), TEXT("/Game/Audio/SFX/SFX_Sword_Hits_Enemy.SFX_Sword_Hits_Enemy"), 0.0f, 0.0f, 0.09f },
			{ TEXT("swordhitsarmor"), TEXT("/Game/Audio/SFX/SFX_Sword_Hit_Armor.SFX_Sword_Hit_Armor"), 0.0f, 0.0f, 0.09f },
			{ TEXT("summoned"), TEXT("/Game/Audio/SFX/SFX_Skeleton_Summoned.SFX_Skeleton_Summoned"), 0.0f, 0.0f, 0.15f },
			// The sound opens with a small first rattle; the director wants only the second, which starts 0.3 s in.
			{ TEXT("skeletondies"), TEXT("/Game/Audio/SFX/SFX_Skeleton_Dies.SFX_Skeleton_Dies"), 0.3f, 0.0f, 0.12f },
			{ TEXT("hammerhitsground"), TEXT("/Game/Audio/SFX/SFX_Hammer_Hits_Ground.SFX_Hammer_Hits_Ground"), 0.0f, 0.0f, 0.15f },
			{ TEXT("firethrow"), TEXT("/Game/Audio/SFX/SFX_Fire_Throw.SFX_Fire_Throw"), 0.0f, 0.0f, 0.05f },
			{ TEXT("fireimpact"), TEXT("/Game/Audio/SFX/SFX_Fire_Impact.SFX_Fire_Impact"), 0.0f, 0.0f, 0.05f },
			// Only its first half second: the first of the three swishes in the file.
			{ TEXT("daggermiss"), TEXT("/Game/Audio/SFX/SFX_Daggar_Miss.SFX_Daggar_Miss"), 0.0f, 0.5f, 0.15f },
			{ TEXT("bowshoot"), TEXT("/Game/Audio/SFX/SFX_Bow_Shoot.SFX_Bow_Shoot"), 0.0f, 0.0f, 0.08f },
			// The thud in each file comes after a stretch of quiet: 0.545, 0.54, 0.225 and 0.195 s in.
			{ TEXT("bodyfalldirt1"), TEXT("/Game/Audio/SFX/SFX_Body_Fall_Dirt_01.SFX_Body_Fall_Dirt_01"), 0.0f, 0.0f, 0.1f, 0.545f },
			{ TEXT("bodyfalldirt2"), TEXT("/Game/Audio/SFX/SFX_Body_Fall_Dirt_02.SFX_Body_Fall_Dirt_02"), 0.0f, 0.0f, 0.1f, 0.54f },
			{ TEXT("bodyfallgrass1"), TEXT("/Game/Audio/SFX/SFX_Body_Fall_Grass_01.SFX_Body_Fall_Grass_01"), 0.0f, 0.0f, 0.1f, 0.225f },
			{ TEXT("bodyfallgrass2"), TEXT("/Game/Audio/SFX/SFX_Body_Fall_Grass_02.SFX_Body_Fall_Grass_02"), 0.0f, 0.0f, 0.1f, 0.195f },
			// A short click of bones: piles walked over one after another are each heard.
			{ TEXT("bonepickup"), TEXT("/Game/Audio/SFX/SFX_Bone_Pickup.SFX_Bone_Pickup"), 0.0f, 0.0f, 0.07f },
			{ TEXT("gateopen"), TEXT("/Game/Audio/SFX/SFX_Gate_Open.SFX_Gate_Open"), 0.0f, 0.0f, 1.0f },
			{ TEXT("commanderspearthrow"), TEXT("/Game/Audio/SFX/SFX_Commander_Spear_Throw.SFX_Commander_Spear_Throw"), 0.0f, 0.0f, 0.2f },
		};
		static_assert(UE_ARRAY_COUNT(Infos) == static_cast<int32>(EWizardCue::Count), "one row for each cue");
		return Infos[FMath::Clamp(static_cast<int32>(Cue), 0, static_cast<int32>(EWizardCue::Count) - 1)];
	}

	/** The cue a name stands for in the console, or Count. */
	static EWizardCue CueByName(const FString& Name)
	{
		for (int32 Index = 0; Index < static_cast<int32>(EWizardCue::Count); ++Index)
		{
			if (Name.Equals(Info(static_cast<EWizardCue>(Index)).Name, ESearchCase::IgnoreCase))
			{
				return static_cast<EWizardCue>(Index);
			}
		}
		return EWizardCue::Count;
	}

	/** A skeleton's blow on a brute or the commander rings on armor; on anything else it cuts. */
	static EWizardCue ForArmyBlow(ECrowdKind Target)
	{
		return Target == ECrowdKind::Brute || Target == ECrowdKind::Commander ? EWizardCue::SwordHitsArmor : EWizardCue::SwordHitsEnemy;
	}

	/** A body meeting the ground: one of two sounds for dirt, one of two for grass (Variant: any number). */
	static EWizardCue ForBodyFall(bool bDirt, int32 Variant)
	{
		const bool bSecond = (Variant & 1) != 0;
		return bDirt ? (bSecond ? EWizardCue::BodyFallDirt2 : EWizardCue::BodyFallDirt1) : (bSecond ? EWizardCue::BodyFallGrass2 : EWizardCue::BodyFallGrass1);
	}

	/**
	 * For a sound that must be HEARD at a moment Until seconds from now: how long to wait before starting it, and how
	 * much of its front to skip when the moment is nearer than its quiet lead-in is long.
	 */
	static void GetLead(EWizardCue Cue, float Until, float& OutWait, float& OutSkip)
	{
		const float Lead = Info(Cue).HeardFrom;
		OutWait = FMath::Max(Until - Lead, 0.0f);
		OutSkip = FMath::Max(Lead - FMath::Max(Until, 0.0f), 0.0f);
	}

	/** True for the army's own that are made of bones: they rattle apart when they die. */
	static bool DiesAsSkeleton(ECrowdTeam Team, ECrowdKind Kind)
	{
		return Team == ECrowdTeam::Army && (Kind == ECrowdKind::Skeleton || Kind == ECrowdKind::Guard || Kind == ECrowdKind::Marksman);
	}

	/** How loud a sound is by how far from the wizard it happens: 1 near him, falling to 0 far away. */
	static float DistanceVolume(double Distance)
	{
		return static_cast<float>(FMath::Clamp((FarDistance - Distance) / (FarDistance - NearDistance), 0.0, 1.0));
	}

	/** The music for where the game is: the start menu (and the settings opened from it), the end, or the game. */
	static EWizardMusic MusicFor(bool bStartMenu, bool bRunWon)
	{
		return bStartMenu ? EWizardMusic::Menu : bRunWon ? EWizardMusic::End : EWizardMusic::Game;
	}

	static const TCHAR* MusicPath(EWizardMusic Music)
	{
		switch (Music)
		{
		case EWizardMusic::Menu: return TEXT("/Game/Audio/Music/MUS_01_Menu_Darkness_Rises.MUS_01_Menu_Darkness_Rises");
		case EWizardMusic::Game: return TEXT("/Game/Audio/Music/MUS_03_Forgotten_Pleasure.MUS_03_Forgotten_Pleasure");
		case EWizardMusic::End: return TEXT("/Game/Audio/Music/MUS_02_Start_Beyond_The_Horizon.MUS_02_Start_Beyond_The_Horizon");
		default: return nullptr;
		}
	}

	static const TCHAR* MusicName(EWizardMusic Music)
	{
		return Music == EWizardMusic::Menu ? TEXT("menu") : Music == EWizardMusic::Game ? TEXT("game") : Music == EWizardMusic::End ? TEXT("end") : TEXT("none");
	}
};

/** Keeps one cue from starting many times at once. */
struct FWizardCueThrottle
{
	/** True when the cue may start at this time (seconds, any clock that only goes forward); it is then counted as started. */
	bool Allow(EWizardCue Cue, double Now)
	{
		const int32 Index = static_cast<int32>(Cue);
		if (Index < 0 || Index >= static_cast<int32>(EWizardCue::Count))
		{
			return false;
		}
		if (bPlayed[Index] && Now - LastPlayed[Index] < FWizardSoundRules::Info(Cue).MinGap && Now >= LastPlayed[Index])
		{
			return false;
		}
		bPlayed[Index] = true;
		LastPlayed[Index] = Now;
		return true;
	}

private:
	double LastPlayed[static_cast<int32>(EWizardCue::Count)] = {};
	bool bPlayed[static_cast<int32>(EWizardCue::Count)] = {};
};

/**
 * Where the ground is painted as dirt (or stone) and where as grass: a grid over the landscape, baked from its
 * paint by Tools/Level/bake_ground_paint.py into the game's settings (the paint itself cannot be read in a
 * packaged game). Run the tool again after repainting.
 */
struct FGroundPaint
{
	int32 Width = 0;
	int32 Height = 0;
	FVector2D Min = FVector2D::ZeroVector;
	FVector2D Max = FVector2D::ZeroVector;
	// One character a cell, row after row from Min: '1' dirt, '0' grass.
	FString Cells;

	bool IsValid() const { return Width > 0 && Height > 0 && Cells.Len() == Width * Height && Max.X > Min.X && Max.Y > Min.Y; }

	/** True where the ground is dirt. Off the grid, or with no grid, it is grass. */
	bool IsDirt(FVector2D Position) const
	{
		if (!IsValid() || Position.X < Min.X || Position.Y < Min.Y || Position.X >= Max.X || Position.Y >= Max.Y)
		{
			return false;
		}
		const int32 X = FMath::Clamp(static_cast<int32>((Position.X - Min.X) / (Max.X - Min.X) * Width), 0, Width - 1);
		const int32 Y = FMath::Clamp(static_cast<int32>((Position.Y - Min.Y) / (Max.Y - Min.Y) * Height), 0, Height - 1);
		return Cells[Y * Width + X] == TEXT('1');
	}
};
