#include "Game/CrowdRenderer.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialParameterCollection.h"
#include "Misc/PackageName.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Engine/StaticMesh.h"
#include "Game/LevelActors.h"
#include "Game/CrowdSubsystem.h"
#include "Game/LevelPieceActor.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/PlayerStart.h"
#include "PhysicsEngine/BodySetup.h"
#include "StaticMeshResources.h"
#if WITH_EDITOR
#include "LandscapeComponent.h"
#include "LandscapeInfo.h"
#include "LandscapeLayerInfoObject.h"
#include "LandscapeProxy.h"
#endif
#include "Game/LevelPieceLook.h"
#include "Game/WizardShapes.h"
#include "Sim/CrowdSim.h"

namespace
{
	const FLinearColor SkeletonColor(0.85f, 0.83f, 0.74f);
	const FLinearColor GruntColor(0.7f, 0.06f, 0.05f);
	// Only when the fire material is missing: the Bolt as a plain orange ball.
	const FLinearColor BoltColor(1.0f, 0.45f, 0.08f);
	const FLinearColor GroundColor(0.09f, 0.08f, 0.07f);
	const FLinearColor WallColor(0.34f, 0.3f, 0.26f);
	const double WallHeight = 160.0;
	// With a landscape in the level the wide ground lies this far under the layout actor; the landscape lies 2 above it.
	const double GroundUnderLandscape = 3.0;

	// How wide each kind is, as a share of its height. The heights come from the simulation's size shares.
	const double SkeletonWidth = 0.45;
	const double GruntWidth = 0.5;
	const double GuardWidth = 0.45;
	const double BruteWidth = 0.75;
	const double CommanderWidth = 0.55;
	const double ArcherWidth = 0.5;
	const double AssassinWidth = 0.45;

	// The archer is a green cone, the assassin a thin violet one. An arrow is a short pale shaft.
	const FLinearColor ArcherColor(0.2f, 0.42f, 0.08f);
	const FLinearColor AssassinColor(0.5f, 0.02f, 0.6f);
	const FLinearColor ArrowColor(0.95f, 0.85f, 0.45f);
	const FVector ArrowScale(0.07, 0.07, 0.7);
	const double ArrowHeight = 70.0;
	// The commander's spear is drawn as an arrow this many times the size, at his shoulder.
	const double JavelinScale = 2.6;
	const double JavelinHeight = 150.0;
	// An assassin coming out of its cloak grows from this share of its size.
	const double AssassinStartShare = 0.15;

	// Basic shapes are 100 units across with the pivot in the middle.
	// The Bolt is a ball of fire, longer along its flight (the sphere's Z) than it is across.
	const FVector BoltScale(0.42, 0.42, 0.8);
	const double BoltHeight = 90.0;

	// The Brute is a large dark block. The Guard is a taller, darker skeleton. A brute's bones are a bigger, redder pile.
	const FLinearColor BruteColor(0.28f, 0.02f, 0.06f);
	const FLinearColor GuardColor(0.62f, 0.55f, 0.3f);
	const FVector BruteBoneScale(0.55, 0.16, 0.16);

	// The commander: the largest block on the field, black and gold.
	const FLinearColor CommanderColor(0.42f, 0.3f, 0.03f);

	// The spear is a thin enchanted shaft. The blast is a flat disc that spreads to the size of the area it hit.
	const FLinearColor SpearColor(0.45f, 1.0f, 0.6f);
	const FVector SpearScale(0.16, 0.16, 1.5);
	const double SpearHeight = 110.0;
	const double BlastHeight = 5.0;
	const double BlastThickness = 0.03;
	// The disc starts at this share of its full size, so the blast is visible from the first frame.
	const double BlastStartShare = 0.35;

	const FLinearColor ShrineBaseColor(0.1f, 0.09f, 0.13f);
	const FLinearColor ShrinePillarColor(0.15f, 0.9f, 0.75f);
	const FVector ShrineBaseScale(1.9, 1.9, 0.1);
	const FVector ShrinePillarScale(0.4, 0.4, 1.9);

	// How each kind of level piece is drawn: shared with the piece actors placed in a level.
	using namespace LevelPieceLook;
	const double WaterBlockerHeight = 200.0;
	// A level piece this tall or taller can hide the fight, so it is cut away around the cursor.
	const double SeeThroughFromHeight = 150.0;

	// Bones lie flat on the ground: a thin pale bar, turned a different way for each pile.
	const FLinearColor BoneColor(0.95f, 0.92f, 0.78f);
	const FVector BoneScale(0.3, 0.09, 0.09);
	// The bone pile model, for what a brute or a Guard leaves.
	const double BonePileBigScale = 1.6;
	const double BoneHeight = 8.0;

	// The models, each on the mannequin's skeleton, moved by the mannequin's animation blueprint and its unarmed swing
	// for now. In the order of ACrowdRenderer's model kinds.
	struct FAttackLook
	{
		// The attack animation, or null. A montage beside it named AM_ in place of A_ is used when there is one.
		const TCHAR* Path;
		// How far through it the blow lands when it has no "Fire" notify: measured on the animation.
		float HitShare;
	};
	struct FModelKind
	{
		ECrowdKind Kind;
		const TCHAR* Name;
		const TCHAR* MeshPath;
		// False for a kind that never trades blows: it gets no swing figures.
		bool bSwings;
		// Its attacks: one, or two. With two, a kind that strikes again and again takes one or the other for each
		// blow; the brute's are its slam and its sweep, the commander's his slash and his throw. With none, the unarmed swing.
		FAttackLook Attacks[ACrowdRenderer::ModelSwingSets];
		// What it holds, and the socket it is held by; or null.
		const TCHAR* WeaponPath;
		const TCHAR* WeaponSocket;
		// What it holds only while it throws it, in place of the first; or null.
		const TCHAR* SecondWeaponPath;
		const TCHAR* SecondWeaponSocket;
	};
	const TCHAR* ShootAnimPath = TEXT("/Game/Animations/Archer/Anim_Archer_Shoot.Anim_Archer_Shoot");
	const TCHAR* BowSocket = TEXT("Bow_Socket");
	// The director's sockets (2026-11-15 00:20 PST): the sword's for the soldier, the skeleton and the Guard; the
	// dagger's for the assassin; the weapon's for the brute and the commander; the commander's own two.
	const TCHAR* SwordSocket = TEXT("Sword_Socket");
	const TCHAR* WeaponSocketName = TEXT("Weapon_Socket");
	const TCHAR* IronSword = TEXT("/Game/Weapons/SM_Iron_Longsword.SM_Iron_Longsword");
	const TCHAR* SteelSword = TEXT("/Game/Weapons/SM_Steel_Sword.SM_Steel_Sword");
	// The director's picks from the Paragon animations (2026-11-14 22:30 PST), with where each one's blow lands when
	// the animation says nothing. The montage beside each (Anim_ before its name) says it with a notify.
	const FAttackLook SwordA = { TEXT("/Game/Paragon_Picks/03_Attack_Skeleton/A_Greystone_Attack_A_Med.A_Greystone_Attack_A_Med"), 0.33f };
	const FAttackLook SwordB = { TEXT("/Game/Paragon_Picks/03_Attack_Skeleton/A_Minions_Attack_B_SetB.A_Minions_Attack_B_SetB"), 0.24f };
	const FAttackLook BruteSlam = { TEXT("/Game/Animations/Brute/Anim_Brute_Hammer_Slam.Anim_Brute_Hammer_Slam"), 0.39f };
	const FAttackLook BruteSweep = { TEXT("/Game/Paragon_Picks/04_Attack_Brute/A_Terra_Primary_Melee_D_slow.A_Terra_Primary_Melee_D_slow"), 0.3f };
	const FAttackLook AssassinAttack = { TEXT("/Game/Paragon_Picks/05_Attack_Assassin/A_Kallari_Attack_GuardBreak_A.A_Kallari_Attack_GuardBreak_A"), 0.53f };
	const FAttackLook CommanderSlash = { TEXT("/Game/Paragon_Picks/07_Attack_Commander/A_Greystone_Attack_PrimaryA.A_Greystone_Attack_PrimaryA"), 0.23f };
	const FAttackLook CommanderThrow = { TEXT("/Game/Paragon_Picks/06_Spear_Throw_Commander/A_Narbash_RMB_Ability_Throw.A_Narbash_RMB_Ability_Throw"), 0.17f };
	const FAttackLook Shoot = { ShootAnimPath, 0.45f };
	const FAttackLook NoAttack = { nullptr, 0.45f };
	const FModelKind ModelKinds[] = {
		// The army's skeletons, the Guards and the enemy's soldiers swing one of two sword blows, a new pick for each
		// blow, sword in hand: the steel sword for the army, the iron longsword for the soldiers (director).
		{ ECrowdKind::Skeleton, TEXT("Skeleton"), TEXT("/Game/Army/SKM_Skeleton.SKM_Skeleton"), true, { SwordA, SwordB }, SteelSword, SwordSocket, nullptr, nullptr },
		{ ECrowdKind::Guard, TEXT("Guard"), TEXT("/Game/Army/SKM_Guard.SKM_Guard"), true, { SwordA, SwordB }, SteelSword, SwordSocket, nullptr, nullptr },
		{ ECrowdKind::Grunt, TEXT("Grunt"), TEXT("/Game/Enemy/SKM_Grunt.SKM_Grunt"), true, { SwordA, SwordB }, IronSword, SwordSocket, nullptr, nullptr },
		// The assassin stabs at the wizard while it is on him, dagger in hand.
		{ ECrowdKind::Assassin, TEXT("Assassin"), TEXT("/Game/Enemy/SKM_Assassin.SKM_Assassin"), true, { AssassinAttack, NoAttack }, TEXT("/Game/Weapons/SM_Dagger.SM_Dagger"), TEXT("Dagger_Socket"), nullptr, nullptr },
		// The archer draws and looses with the director's shooting animation, the wooden bow in its left hand.
		{ ECrowdKind::Archer, TEXT("Archer"), TEXT("/Game/Enemy/SKM_Archer.SKM_Archer"), true, { Shoot, NoAttack }, TEXT("/Game/Weapons/SM_Wood_Bow.SM_Wood_Bow"), BowSocket, nullptr, nullptr },
		// The brute: its slam (the heavy one, with its own animation) and its sweep, the steel hammer in its right hand.
		{ ECrowdKind::Brute, TEXT("Brute"), TEXT("/Game/Enemy/SKM_Brute.SKM_Brute"), true, { BruteSlam, BruteSweep }, TEXT("/Game/Weapons/SM_Steel_Hammer.SM_Steel_Hammer"), WeaponSocketName, nullptr, nullptr },
		// The Marksman shoots as the archer does (it has no animation of its own), with the skeleton bow.
		{ ECrowdKind::Marksman, TEXT("Marksman"), TEXT("/Game/Army/SKM_Marksman.SKM_Marksman"), true, { Shoot, NoAttack }, TEXT("/Game/Weapons/SM_Skeleton_Bow.SM_Skeleton_Bow"), BowSocket, nullptr, nullptr },
		// The commander: his slash with his sword, and his spear, which is in his hand only while he draws back to throw it.
		{ ECrowdKind::Commander, TEXT("Commander"), TEXT("/Game/Enemy/SKM_Commander.SKM_Commander"), true, { CommanderSlash, CommanderThrow },
			TEXT("/Game/Weapons/SM_Commander_Sword.SM_Commander_Sword"), TEXT("Commander_Attack_Sword"), TEXT("/Game/Weapons/SM_Commander_Spear.SM_Commander_Spear"), TEXT("Commander_Spear_Socket") },
	};
	static_assert(UE_ARRAY_COUNT(ModelKinds) == ACrowdRenderer::ModelKindCount, "one model for each model kind");
	const TCHAR* ArmyAnimPath = TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C");
	const TCHAR* SwingAnimPath = TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01.MM_Attack_01");
	// How far through the swing the fist is farthest out: measured on MM_Attack_01 (0.45 to 0.5 of its second).
	const float SwingHitShare = 0.45f;
	// The speed each gait is animated at: standing, strolling, running.
	const float ArmyGaitSpeeds[] = { 0.0f, 150.0f, 500.0f };
	// A figure strolls from this speed and runs from this one.
	const float ArmyWalkFrom = 25.0f;
	const float ArmyRunFrom = 260.0f;
	const float ArmyTurnDegreesPerSecond = 720.0f;
	const float ArmySpeedEase = 8.0f;
	// The models face +Y, like the mannequin.
	const float ArmyModelYaw = -90.0f;

	// The brute's back was marked with a glowing knot; the director had the mark taken off (2026-11-14). The color is kept for the unused shape.
	const FLinearColor BruteBackColor(1.0f, 0.55f, 0.05f);
	// An unseen box this tall stops the wizard at a building placed in the level.
	const double BuildingBlockerHeight = 600.0;
	// A brute's slam: a dark red disc that grows, bright red once it is full, and a hot flash where it lands.
	// The enemy's warnings: dried blood with an ember rim while they grow, burning for their last moment.
	const FLinearColor SlamRingColor(0.32f, 0.015f, 0.03f);
	const FLinearColor SlamRingGlow(1.0f, 0.3f, 0.06f);
	const FLinearColor SlamFullColor(0.6f, 0.03f, 0.015f);
	const FLinearColor SlamFullGlow(1.0f, 0.45f, 0.08f);
	const FLinearColor SlamFlashColor(0.9f, 0.22f, 0.03f);
	const FLinearColor SlamFlashGlow(1.0f, 0.6f, 0.18f);
	// Where the wizard's spear lands: his own violet.
	const FLinearColor SpearMarkColor(0.2f, 0.025f, 0.55f);
	const FLinearColor SpearMarkGlow(0.55f, 0.25f, 1.0f);
	const double SlamRingHeight = 11.0;
	const double SlamFullHeight = 12.0;
	// The spear throw's lane starts this share of its reach long, and is this thick (a cube 100 units tall, scaled).
	const double ThrowLaneStartShare = 0.15;
	const double ThrowLaneThickness = 0.02;
	// A brute's sweep is shown as this many bars fanned across its arc, each this wide.
	const double SlamFlashHeight = 13.0;
	// The flash spreads this much past the circle as it fades.
	const double SlamFlashSpread = 0.2;
	// The cut where a skeleton was taken for the spear: a thin pale bar, tilted, that stretches as it fades.
	const FLinearColor SlashColor(0.75f, 1.0f, 0.95f);

	int32 FindModelKind(ECrowdKind Kind)
	{
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(ModelKinds); ++Index)
		{
			if (ModelKinds[Index].Kind == Kind)
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}

	/** The animation blueprint reads a Character's movement. These figures are not Characters, so its values are written here. */
	void WriteGait(USkeletalMeshComponent* Leader, float Speed)
	{
		UAnimInstance* Anim = Leader ? Leader->GetAnimInstance() : nullptr;
		if (!Anim)
		{
			return;
		}
		const UClass* Class = Anim->GetClass();
		if (const FBoolProperty* ShouldMove = CastField<FBoolProperty>(Class->FindPropertyByName(TEXT("ShouldMove"))))
		{
			ShouldMove->SetPropertyValue_InContainer(Anim, Speed > 1.0f);
		}
		const FProperty* GroundSpeed = Class->FindPropertyByName(TEXT("GroundSpeed"));
		if (const FDoubleProperty* AsDouble = CastField<FDoubleProperty>(GroundSpeed))
		{
			AsDouble->SetPropertyValue_InContainer(Anim, Speed);
		}
		else if (const FFloatProperty* AsFloat = CastField<FFloatProperty>(GroundSpeed))
		{
			AsFloat->SetPropertyValue_InContainer(Anim, Speed);
		}
		const FStructProperty* Velocity = CastField<FStructProperty>(Class->FindPropertyByName(TEXT("Velocity")));
		if (Velocity && Velocity->Struct == TBaseStructure<FVector>::Get())
		{
			*Velocity->ContainerPtrToValuePtr<FVector>(Anim) = FVector(Speed, 0.0, 0.0);
		}
	}
}

void ACrowdRenderer::SetSeeThrough(FVector2D CursorUV, float Aspect, float RadiusShare)
{
	const FLinearColor Value(static_cast<float>(CursorUV.X), static_cast<float>(CursorUV.Y), Aspect, FMath::Max(RadiusShare, 0.0f));
	if (Value == SeeThroughCursor)
	{
		return;
	}
	SeeThroughCursor = Value;
	if (!SeeThroughCollection)
	{
		SeeThroughCollection = LoadObject<UMaterialParameterCollection>(nullptr, TEXT("/Game/Level/MPC_SeeThrough.MPC_SeeThrough"));
	}
	if (SeeThroughCollection && GetWorld())
	{
		UKismetMaterialLibrary::SetVectorParameterValue(this, SeeThroughCollection, TEXT("Cursor"), Value);
	}
	for (UMaterialInstanceDynamic* Material : SeeThroughMaterials)
	{
		Material->SetVectorParameterValue(TEXT("Cursor"), Value);
	}
}

int32 ACrowdRenderer::PickSwing(double Clock, float TimeToBlow, float Interval, float HitShare, int32 SwingCount)
{
	if (Interval <= 0.0f || SwingCount <= 0)
	{
		return 0;
	}
	// Where in its swing this unit should be now, for the blow to land at HitShare: the blow is TimeToBlow away.
	const double Wanted = HitShare - TimeToBlow / Interval;
	// Swing k is at (Clock / Interval + k / SwingCount) through its swing.
	const double Offset = (Wanted - Clock / Interval) * SwingCount;
	const int32 Swing = FMath::RoundToInt32(Offset) % SwingCount;
	return Swing < 0 ? Swing + SwingCount : Swing;
}

float ACrowdRenderer::GetSwingShare(double Clock, float Interval, int32 Swing, int32 SwingCount)
{
	if (Interval <= 0.0f || SwingCount <= 0)
	{
		return 0.0f;
	}
	return static_cast<float>(FMath::Frac(Clock / Interval + static_cast<double>(Swing) / SwingCount));
}

float ACrowdRenderer::FSwingTiming::PositionAt(float Seconds) const
{
	if (!bOneShot)
	{
		return Cycle > 0.0f ? Length * FMath::Clamp(Seconds / Cycle, 0.0f, 1.0f) : 0.0f;
	}
	if (Seconds <= BlowAt)
	{
		// The wind-up: the animation up to its blow, over however long the simulation winds up for.
		return BlowAt > 0.0f ? HitTime * (Seconds / BlowAt) : HitTime;
	}
	// The follow-through, at the animation's own speed.
	return FMath::Min(HitTime + (Seconds - BlowAt), Length);
}

ACrowdRenderer::FSwingTiming ACrowdRenderer::MakeSwingTiming(const FCrowdTuning& Tuning, ECrowdKind Kind, int32 Set, float HitShare, float Length)
{
	FSwingTiming Timing;
	Timing.Length = FMath::Max(Length, 0.01f);
	Timing.HitTime = FMath::Clamp(HitShare, 0.0f, 1.0f) * Timing.Length;
	float WindUp = 0.0f;
	if (Kind == ECrowdKind::Brute)
	{
		WindUp = Set == 1 ? Tuning.BruteSweepWindup : Tuning.BruteSlamGrow + Tuning.BruteSlamHold;
	}
	else if (Kind == ECrowdKind::Commander)
	{
		WindUp = Set == 1 ? Tuning.CommanderSpearWindup : Tuning.CommanderSlashGrow + Tuning.CommanderSlashHold;
	}
	if (WindUp > 0.0f)
	{
		Timing.bOneShot = true;
		Timing.BlowAt = WindUp;
		// After the blow the rest of the animation plays once; a little is always left so the turn never has no end.
		Timing.Cycle = WindUp + FMath::Max(Timing.Length - Timing.HitTime, 0.2f);
		return Timing;
	}
	Timing.Cycle = FMath::Max(GetStrikeInterval(Tuning, Kind, HitShare), 0.01f);
	Timing.BlowAt = FMath::Clamp(HitShare, 0.0f, 1.0f) * Timing.Cycle;
	return Timing;
}

UAnimSequenceBase* ACrowdRenderer::LoadAttack(const TCHAR* Path)
{
	if (!Path)
	{
		return nullptr;
	}
	// "/Game/Folder/A_Name.A_Name": the montage, if the director made one, is "/Game/Folder/Anim_A_Name.Anim_A_Name".
	const FString Whole(Path);
	FString Package;
	FString Name;
	if (Whole.Split(TEXT("."), &Package, &Name))
	{
		// The director's montage of an animation has "Anim_" before its name ("Anim_A_Grux_Death_B" for "A_Grux_Death_B").
		const FString MontageName = TEXT("Anim_") + Name;
		const FString MontagePackage = FPaths::GetPath(Package) / MontageName;
		if (FPackageName::DoesPackageExist(MontagePackage))
		{
			if (UAnimSequenceBase* Montage = LoadObject<UAnimSequenceBase>(nullptr, *(MontagePackage + TEXT(".") + MontageName)))
			{
				return Montage;
			}
		}
	}
	return FPackageName::DoesPackageExist(Package.IsEmpty() ? Whole : Package) ? LoadObject<UAnimSequenceBase>(nullptr, Path) : nullptr;
}

void ACrowdRenderer::FindFloors()
{
	FloorActors.Reset();
	FloorBoxes.Reset();
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
	{
		const UStaticMeshComponent* Component = It->GetStaticMeshComponent();
		const UStaticMesh* Mesh = Component ? Component->GetStaticMesh() : nullptr;
		if (Mesh && (It->ActorHasTag(TEXT("Walkable")) || Mesh->GetPathName().StartsWith(TEXT("/Game/Props/SM_Bridge"))))
		{
			const FBox Bounds = It->GetComponentsBoundingBox();
			FloorActors.Add(*It);
			FloorBoxes.Emplace(FVector2D(Bounds.Min.X, Bounds.Min.Y), FVector2D(Bounds.Max.X, Bounds.Max.Y));
		}
	}
}

double ACrowdRenderer::GetFloorZ(FVector2D Position, double GroundZ) const
{
	for (int32 Index = 0; Index < FloorBoxes.Num(); ++Index)
	{
		if (!FloorBoxes[Index].IsInside(Position) || !FloorActors[Index].IsValid())
		{
			continue;
		}
		// Straight down onto the bridge's own collision: the deck's height here.
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(CrowdFloor), false);
		const FVector From(Position.X, Position.Y, GroundZ + 800.0);
		const FVector To(Position.X, Position.Y, GroundZ - 100.0);
		if (GetWorld()->LineTraceSingleByChannel(Hit, From, To, ECC_WorldStatic, Params) && Hit.GetActor() == FloorActors[Index].Get())
		{
			return FMath::Max(Hit.Location.Z, GroundZ);
		}
	}
	return GroundZ;
}

float ACrowdRenderer::GetNotifySeconds(const UAnimSequenceBase* Animation, TConstArrayView<FName> Names)
{
	if (!Animation)
	{
		return -1.0f;
	}
	float First = -1.0f;
	for (const FAnimNotifyEvent& Event : Animation->Notifies)
	{
		// A plain notify is named on the event. A montage notify (the kind the director uses) carries its name in the
		// notify object, read here by name so the game needs no more of the engine for it.
		FName Name = Event.NotifyName;
		if (Event.Notify)
		{
			if (const FNameProperty* Property = CastField<FNameProperty>(Event.Notify->GetClass()->FindPropertyByName(TEXT("NotifyName"))))
			{
				Name = Property->GetPropertyValue_InContainer(Event.Notify);
			}
		}
		if (Names.Contains(Name) && (First < 0.0f || Event.GetTriggerTime() < First))
		{
			First = Event.GetTriggerTime();
		}
	}
	return First;
}

float ACrowdRenderer::GetFireShare(const UAnimSequenceBase* Animation, float Fallback)
{
	const float Length = Animation ? Animation->GetPlayLength() : 0.0f;
	if (Length <= 0.0f)
	{
		return Fallback;
	}
	// The moment the blow lands, the arrow leaves or the spear is let go: "Fire" on the first animations, "Attack"
	// and "ThrowSpear" on the director's montages of 2026-11-15.
	const FName Names[] = { FName(TEXT("Fire")), FName(TEXT("Attack")), FName(TEXT("ThrowSpear")) };
	const float Seconds = GetNotifySeconds(Animation, Names);
	return Seconds >= 0.0f ? FMath::Clamp(Seconds / Length, 0.0f, 1.0f) : Fallback;
}

FString ACrowdRenderer::GetEffectSummary() const
{
	const auto Part = [](const TCHAR* Name, const UInstancedStaticMeshComponent* Instances)
	{
		const UMaterialInterface* Material = Instances ? Instances->GetMaterial(0) : nullptr;
		const UMaterial* Base = Material ? Material->GetMaterial() : nullptr;
		const UStaticMesh* Mesh = Instances ? Instances->GetStaticMesh() : nullptr;
		return FString::Printf(TEXT("%s:%s/%s"), Name, Base ? *Base->GetName() : TEXT("none"), Mesh ? *Mesh->GetName() : TEXT("none"));
	};
	return Part(TEXT("Bolt"), Bolts) + TEXT(" ") + Part(TEXT("Slam"), SlamRings) + TEXT(" ") + Part(TEXT("SlamFull"), SlamFulls) + TEXT(" ")
		+ Part(TEXT("SlamFlash"), SlamFlashes) + TEXT(" ") + Part(TEXT("Lane"), ThrowLanes) + TEXT(" ") + Part(TEXT("LaneLocked"), ThrowLocks) + TEXT(" ") + Part(TEXT("Sweep"), SweepFans) + TEXT(" ")
		+ Part(TEXT("SpearBlast"), Blasts);
}

FString ACrowdRenderer::GetWeaponSummary() const
{
	FString Line;
	for (int32 Kind = 0; Kind < ModelKindCount; ++Kind)
	{
		if (WeaponMeshes[Kind])
		{
			Line += FString::Printf(TEXT("%s%s:%s@%s"), Line.IsEmpty() ? TEXT("") : TEXT(" "), ModelKinds[Kind].Name, *WeaponMeshes[Kind]->GetName(), *WeaponSockets[Kind].ToString());
		}
		if (SecondWeaponMeshes[Kind])
		{
			Line += FString::Printf(TEXT(" %s:%s@%s"), ModelKinds[Kind].Name, *SecondWeaponMeshes[Kind]->GetName(), *SecondWeaponSockets[Kind].ToString());
		}
	}
	return Line;
}

float ACrowdRenderer::GetStrikeInterval(const FCrowdTuning& Tuning, ECrowdKind Kind, float HitShare)
{
	switch (Kind)
	{
	case ECrowdKind::Archer:
		return Tuning.ArcherInterval;
	case ECrowdKind::Marksman:
		return Tuning.MarksmanInterval;
	case ECrowdKind::Brute:
		// The animation starts when the circle does, and its Fire moment is when the slam lands.
		return (Tuning.BruteSlamGrow + Tuning.BruteSlamHold) / FMath::Max(HitShare, 0.05f);
	default:
		return Tuning.MeleeInterval;
	}
}

int32 ACrowdRenderer::GetSwingsReady() const
{
	int32 Count = 0;
	for (int32 Index = 0; Index < ModelLeaders.Num(); ++Index)
	{
		const USkeletalMeshComponent* Leader = ModelLeaders[Index];
		const int32 Swing = Index % ModelLeaderCount - ModelGaitCount;
		if (Leader && Swing >= 0)
		{
			const UAnimSingleNodeInstance* Player = Leader->GetSingleNodeInstance();
			const UAnimSequenceBase* Attack = AttackAnimations[(Index / ModelLeaderCount) * ModelSwingSets + Swing / ModelSwingCount];
			Count += Player && Attack && Player->GetAnimationAsset() == Attack ? 1 : 0;
		}
	}
	return Count;
}

void ACrowdRenderer::SetUpArmyModels()
{
	bArmyModelTried = true;
	FindFloors();
	UClass* AnimClass = LoadClass<UAnimInstance>(nullptr, ArmyAnimPath);
	UAnimSequenceBase* Swing = LoadObject<UAnimSequenceBase>(nullptr, SwingAnimPath);
	ModelLeaders.SetNum(ModelKindCount * ModelLeaderCount);
	for (int32 Kind = 0; Kind < ModelKindCount; ++Kind)
	{
		USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, ModelKinds[Kind].MeshPath);
		if (!Mesh || !AnimClass)
		{
			// No model for this kind: its simple shape stays.
			continue;
		}
		ModelMeshes[Kind] = Mesh;
		// Its own attacks, each timed by its "Fire" notify (or where its blow was measured to land); with none, the unarmed swing.
		for (int32 Set = 0; Set < ModelSwingSets; ++Set)
		{
			const FAttackLook& Look = ModelKinds[Kind].Attacks[Set];
			UAnimSequenceBase* Own = LoadAttack(Look.Path);
			if (!Own && Set == 0)
			{
				AttackAnimations[Kind * ModelSwingSets] = Swing;
				AttackHitShares[Kind * ModelSwingSets] = SwingHitShare;
				continue;
			}
			AttackAnimations[Kind * ModelSwingSets + Set] = Own;
			AttackHitShares[Kind * ModelSwingSets + Set] = Own ? GetFireShare(Own, Look.HitShare) : Look.HitShare;
		}
		WeaponMeshes[Kind] = ModelKinds[Kind].WeaponPath ? LoadObject<UStaticMesh>(nullptr, ModelKinds[Kind].WeaponPath) : nullptr;
		SecondWeaponMeshes[Kind] = ModelKinds[Kind].SecondWeaponPath ? LoadObject<UStaticMesh>(nullptr, ModelKinds[Kind].SecondWeaponPath) : nullptr;
		// A socket the model does not have falls back to the weapon socket every model has.
		const auto SocketOr = [Mesh](const TCHAR* Socket)
		{
			return Socket && Mesh->FindSocket(FName(Socket)) ? FName(Socket) : FName(WeaponSocketName);
		};
		WeaponSockets[Kind] = SocketOr(ModelKinds[Kind].WeaponSocket);
		SecondWeaponSockets[Kind] = SocketOr(ModelKinds[Kind].SecondWeaponSocket);
		ModelAssetHeights[Kind] = FMath::Max(static_cast<float>(Mesh->GetBounds().BoxExtent.Z * 2.0), 1.0f);
		for (int32 Index = 0; Index < ModelLeaderCount; ++Index)
		{
			UAnimSequenceBase* Attack = Index >= ModelGaitCount ? AttackAnimations[Kind * ModelSwingSets + (Index - ModelGaitCount) / ModelSwingCount].Get() : nullptr;
			const bool bSwing = Attack != nullptr;
			if (Index >= ModelGaitCount && (!ModelKinds[Kind].bSwings || !Attack))
			{
				// This kind never strikes, or has no second attack: no swing figures for it.
				continue;
			}
			USkeletalMeshComponent* Leader = NewObject<USkeletalMeshComponent>(this);
			Leader->SetupAttachment(GetRootComponent());
			Leader->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Leader->SetCastShadow(false);
			Leader->SetGenerateOverlapEvents(false);
			// Never drawn, and still animated: the figures on screen copy its pose.
			Leader->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
			Leader->SetSkeletalMeshAsset(Mesh);
			if (!bSwing)
			{
				Leader->SetAnimInstanceClass(AnimClass);
			}
			else
			{
				Leader->SetAnimationMode(EAnimationMode::AnimationSingleNode);
			}
			Leader->RegisterComponent();
			if (bSwing)
			{
				// A swing: it does not play by itself; its place in the swing is set from the simulation's clock.
				// The animation can be given only now: a figure has no animation player until it is registered, and
				// one given before that is dropped, which left every striking figure in its bind pose.
				Leader->SetAnimation(Attack);
				Leader->SetPlayRate(0.0f);
			}
			Leader->SetVisibility(false);
			ModelLeaders[Kind * ModelLeaderCount + Index] = Leader;
		}
		bModelLoaded[Kind] = true;
		bArmyModelLoaded = true;
	}
}

void ACrowdRenderer::RefreshArmyModels(const FCrowdSim& Sim, float GroundZ, float WizardHeight, FVector2D WizardPosition, TBitArray<>& OutModeled)
{
	if (!bArmyModelTried)
	{
		SetUpArmyModels();
	}
	const double CostStart = FPlatformTime::Seconds();
	if (LastModelTime > 0.0)
	{
		const double FrameMs = (CostStart - LastModelTime) * 1000.0;
		++ModelCost.Frames;
		ModelCost.FrameMs += FrameMs;
		ModelCost.FramePeakMs = FMath::Max(ModelCost.FramePeakMs, FrameMs);
		++Window.Frames;
		Window.FrameMs += FrameMs;
		Window.FramePeakMs = FMath::Max(Window.FramePeakMs, FrameMs);
	}
	if (CostStart - WindowStart >= 3.0)
	{
		Window.Creates = ModelCost.Creates - Window.Creates;
		Window.Switches = ModelCost.Switches - Window.Switches;
		Window.Flips = ModelCost.Flips - Window.Flips;
		WindowShown = Window;
		Window = FModelCost();
		Window.Creates = ModelCost.Creates;
		Window.Switches = ModelCost.Switches;
		Window.Flips = ModelCost.Flips;
		WindowStart = CostStart;
	}
	LastModelTime = CostStart;
	const TArray<FCrowdUnit>& Units = Sim.GetUnits();
	OutModeled.Init(false, Units.Num());
	int32 Used[ModelKindCount] = {};
	int32 Drawn = 0;
	if (IsArmyModelShown())
	{
		const float Delta = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.0f;
		const double Clock = Sim.GetClock();
		++ArmyStamp;
		FVector Scales[ModelKindCount];
		FSwingTiming Timings[ModelKindCount * ModelSwingSets];
		for (int32 Kind = 0; Kind < ModelKindCount; ++Kind)
		{
			Scales[Kind] = FVector(Sim.GetUnitHeight(ModelKinds[Kind].Kind, WizardHeight) / ModelAssetHeights[Kind]);
			for (int32 Set = 0; Set < ModelSwingSets; ++Set)
			{
				const UAnimSequenceBase* Attack = AttackAnimations[Kind * ModelSwingSets + Set];
				Timings[Kind * ModelSwingSets + Set] = MakeSwingTiming(Sim.Tuning, ModelKinds[Kind].Kind, Set, AttackHitShares[Kind * ModelSwingSets + Set], Attack ? Attack->GetPlayLength() : 1.0f);
			}
			if (!bModelLoaded[Kind])
			{
				continue;
			}
			for (int32 Index = 0; Index < ModelLeaderCount; ++Index)
			{
				USkeletalMeshComponent* Leader = ModelLeaders[Kind * ModelLeaderCount + Index];
				if (!Leader)
				{
					continue;
				}
				if (Index < ModelGaitCount)
				{
					WriteGait(Leader, ArmyGaitSpeeds[Index]);
				}
				else
				{
					const FSwingTiming& Timing = Timings[Kind * ModelSwingSets + (Index - ModelGaitCount) / ModelSwingCount];
					Leader->SetPosition(Timing.PositionAt(GetSwingShare(Clock, Timing.Cycle, (Index - ModelGaitCount) % ModelSwingCount, ModelSwingCount) * Timing.Cycle), false);
				}
				// The blueprint plants the feet on whatever ground is under the figure, so the hidden ones stand on
				// the ground too, where the wizard is, at the size the others are shown.
				Leader->SetWorldTransform(FTransform(FRotator(0.0, ArmyModelYaw, 0.0), FVector(WizardPosition.X, WizardPosition.Y, GroundZ), Scales[Kind]));
			}
		}

		// The army first; then enemies, those nearest the wizard first, until the cap is reached.
		TArray<int32, TInlineAllocator<256>> Order;
		TArray<int32, TInlineAllocator<256>> Enemies;
		for (int32 Index = 0; Index < Units.Num(); ++Index)
		{
			const int32 Kind = FindModelKind(Units[Index].Kind);
			// A cloaked assassin is not drawn at all.
			if (Kind != INDEX_NONE && bModelLoaded[Kind] && Sim.GetShown(Units[Index], WizardPosition) > 0.0f)
			{
				(Units[Index].Team == ECrowdTeam::Army ? Order : Enemies).Add(Index);
			}
		}
		if (Order.Num() + Enemies.Num() > ModelCap)
		{
			Enemies.Sort([&Units, WizardPosition](int32 Left, int32 Right)
			{
				return FVector2D::DistSquared(Units[Left].Position, WizardPosition) < FVector2D::DistSquared(Units[Right].Position, WizardPosition);
			});
		}
		Order.Append(Enemies);
		Order.SetNum(FMath::Min(Order.Num(), ModelCap), EAllowShrinking::No);

		for (const int32 UnitIndex : Order)
		{
			const FCrowdUnit& Unit = Units[UnitIndex];
			const int32 Kind = FindModelKind(Unit.Kind);
			FArmyModelState& State = ArmyStates.FindOrAdd(Unit.Id);
			const bool bNew = State.Stamp == 0;
			const float Speed = static_cast<float>(Unit.Velocity.Size());
			State.Speed = bNew ? Speed : FMath::FInterpTo(State.Speed, Speed, Delta, ArmySpeedEase);
			State.Stamp = ArmyStamp;

			// It faces the way the simulation says: the way it walks, or what it strikes.
			const float Wanted = Unit.Facing.IsNearlyZero() ? State.Yaw : FMath::RadiansToDegrees(static_cast<float>(FMath::Atan2(Unit.Facing.Y, Unit.Facing.X)));
			State.Yaw = bNew ? Wanted : FMath::FixedTurn(State.Yaw, Wanted, ArmyTurnDegreesPerSecond * Delta);

			if (!ModelSlots[Kind].IsValidIndex(Used[Kind]))
			{
				const double T0 = FPlatformTime::Seconds();
				ON_SCOPE_EXIT { ++ModelCost.Creates; ModelCost.CreateMs += (FPlatformTime::Seconds() - T0) * 1000.0; };
				USkeletalMeshComponent* Model = NewObject<USkeletalMeshComponent>(this);
				Model->SetupAttachment(GetRootComponent());
				Model->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				Model->SetCastShadow(false);
				Model->SetGenerateOverlapEvents(false);
				Model->SetCanEverAffectNavigation(false);
				Model->SetSkeletalMeshAsset(ModelMeshes[Kind]);
				Model->RegisterComponent();
				ModelSlots[Kind].Add(ModelPool.Add(Model));
				ModelPoolLeader.Add(INDEX_NONE);
				// What it holds goes with its hand: a child of the figure, on the socket.
				UStaticMeshComponent* Weapon = nullptr;
				if (WeaponMeshes[Kind])
				{
					Weapon = NewObject<UStaticMeshComponent>(this);
					Weapon->SetStaticMesh(WeaponMeshes[Kind]);
					Weapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
					Weapon->SetCastShadow(false);
					Weapon->SetGenerateOverlapEvents(false);
					Weapon->SetCanEverAffectNavigation(false);
					Weapon->SetupAttachment(Model, WeaponSockets[Kind]);
					Weapon->RegisterComponent();
				}
				ModelWeapons.Add(Weapon);
				UStaticMeshComponent* Second = nullptr;
				if (SecondWeaponMeshes[Kind])
				{
					Second = NewObject<UStaticMeshComponent>(this);
					Second->SetStaticMesh(SecondWeaponMeshes[Kind]);
					Second->SetCollisionEnabled(ECollisionEnabled::NoCollision);
					Second->SetCastShadow(false);
					Second->SetGenerateOverlapEvents(false);
					Second->SetCanEverAffectNavigation(false);
					Second->SetupAttachment(Model, SecondWeaponSockets[Kind]);
					Second->RegisterComponent();
				}
				ModelSecondWeapons.Add(Second);
			}
			const int32 Slot = ModelSlots[Kind][Used[Kind]++];
			USkeletalMeshComponent* Model = ModelPool[Slot];

			// Striking, it copies the swing whose blow lands when its own does. Otherwise a gait by its speed.
			int32 Leader = State.Speed >= ArmyRunFrom ? 2 : State.Speed >= ArmyWalkFrom ? 1 : 0;
			float TimeToBlow = 0.0f;
			bool bStrikes = false;
			int32 Set = 0;
			const bool bSecond = AttackAnimations[Kind * ModelSwingSets + 1] != nullptr;
			if (ModelKinds[Kind].bSwings && AttackAnimations[Kind * ModelSwingSets])
			{
				// The commander's second attack is his throw: shown while he draws back, and after it unless he is already in a slash.
				if (Unit.Kind == ECrowdKind::Commander && bSecond && Sim.GetThrow(Unit, TimeToBlow) && (TimeToBlow >= 0.0f || !Sim.IsSlamming(Unit)))
				{
					bStrikes = true;
					Set = 1;
				}
				else
				{
					bStrikes = Sim.GetStrike(Unit, TimeToBlow);
				}
			}
			if (bStrikes && Unit.Kind == ECrowdKind::Brute && bSecond)
			{
				// The brute's second attack is its sweep.
				Set = Unit.bSweep ? 1 : 0;
				State.LastPhase = -1.0f;
			}
			else if (bStrikes && Unit.Kind != ECrowdKind::Commander && bSecond)
			{
				// One or the other of its two blows, picked again as each new blow starts (where its turn comes round).
				const FSwingTiming& Now = Timings[Kind * ModelSwingSets + State.Set];
				const float Phase = FMath::Frac((Now.BlowAt - TimeToBlow) / Now.Cycle + 1.0f);
				if (State.LastPhase < 0.0f || Phase < State.LastPhase - 0.5f)
				{
					State.Set = FMath::RandRange(0, ModelSwingSets - 1);
				}
				State.LastPhase = Phase;
				Set = State.Set;
			}
			else
			{
				State.LastPhase = -1.0f;
			}
			if (bStrikes)
			{
				const FSwingTiming& Timing = Timings[Kind * ModelSwingSets + Set];
				// A wound-up attack that has played out: it stands again until the next.
				if (!Timing.bOneShot || -TimeToBlow < Timing.GetTail() - 0.05f)
				{
					// The attack whose "Fire" moment comes when this unit's blow lands (below zero: it has landed, and it follows through).
					Leader = ModelGaitCount + Set * ModelSwingCount + PickSwing(Clock, TimeToBlow, Timing.Cycle, Timing.BlowAt / Timing.Cycle, ModelSwingCount);
				}
			}
			if (PerfTest == 1)
			{
				Leader = 0;
			}
			Leader += Kind * ModelLeaderCount;
			if (ModelPoolLeader[Slot] != Leader)
			{
				const double T0 = FPlatformTime::Seconds();
				ModelPoolLeader[Slot] = Leader;
				Model->SetLeaderPoseComponent(ModelLeaders[Leader]);
				++ModelCost.Switches;
				ModelCost.SwitchMs += (FPlatformTime::Seconds() - T0) * 1000.0;
			}
			// An assassin coming out of its cloak grows into view.
			const double Shown = Unit.Kind == ECrowdKind::Assassin ? FMath::Lerp(AssassinStartShare, 1.0, static_cast<double>(Sim.GetShown(Unit, WizardPosition))) : 1.0;
			// On a bridge it stands on the deck, not on the ground under it.
			Model->SetWorldTransform(FTransform(FRotator(0.0, State.Yaw + ArmyModelYaw, 0.0), FVector(Unit.Position.X, Unit.Position.Y, GetFloorZ(Unit.Position, GroundZ)), Scales[Kind] * Shown));
			if (!Model->IsVisible())
			{
				const double T0 = FPlatformTime::Seconds();
				Model->SetVisibility(true, true);
				++ModelCost.Flips;
				ModelCost.FlipMs += (FPlatformTime::Seconds() - T0) * 1000.0;
			}
			if (UStaticMeshComponent* Second = ModelSecondWeapons[Slot])
			{
				// The spear is in his hand while he draws back to throw it and gone once it is thrown; his sword is
				// put up for the whole of the throw.
				float TimeToThrow = 0.0f;
				const bool bThrows = Sim.GetThrow(Unit, TimeToThrow);
				const bool bSpear = bThrows && TimeToThrow >= 0.0f;
				if (Second->IsVisible() != bSpear)
				{
					Second->SetVisibility(bSpear);
				}
				if (UStaticMeshComponent* First = ModelWeapons[Slot])
				{
					if (First->IsVisible() == bThrows)
					{
						First->SetVisibility(!bThrows);
					}
				}
			}
			OutModeled[UnitIndex] = true;
			++Drawn;
		}
		// Forget figures that are gone.
		for (auto It = ArmyStates.CreateIterator(); It; ++It)
		{
			if (It.Value().Stamp != ArmyStamp)
			{
				It.RemoveCurrent();
			}
		}
	}
	for (int32 Kind = 0; Kind < ModelKindCount; ++Kind)
	{
		for (int32 Index = Used[Kind]; Index < ModelSlots[Kind].Num(); ++Index)
		{
			USkeletalMeshComponent* Model = ModelPool[ModelSlots[Kind][Index]];
			if (PerfTest == 2)
			{
				Model->SetWorldLocation(FVector(0.0, 0.0, -100000.0));
				continue;
			}
			if (Model->IsVisible())
			{
				const double T0 = FPlatformTime::Seconds();
				Model->SetVisibility(false, true);
				++ModelCost.Flips;
				ModelCost.FlipMs += (FPlatformTime::Seconds() - T0) * 1000.0;
			}
		}
	}
	ArmyModelsDrawn = Drawn;
	const double CostMs = (FPlatformTime::Seconds() - CostStart) * 1000.0;
	ModelCost.ModelMs += CostMs;
	ModelCost.ModelPeakMs = FMath::Max(ModelCost.ModelPeakMs, CostMs);
}

void ACrowdRenderer::SetUpDeaths()
{
	bDeathsTried = true;
	for (int32 Index = 0; Index < static_cast<int32>(EDeathFall::Count); ++Index)
	{
		const EDeathFall Fall = static_cast<EDeathFall>(Index);
		DeathAnimations[Index] = LoadAttack(FDeathPick::AnimationPath(Fall));
		DeathFallSeconds[Index] = FDeathPick::FallSeconds(Fall);
		const UAnimSequenceBase* Animation = DeathAnimations[Index];
		if (!Animation)
		{
			continue;
		}
		// The director marks the moment the body meets the ground with a notify named "PlaySound" on the death's montage.
		const FName Names[] = { FName(TEXT("PlaySound")), FName(TEXT("Fall")) };
		const float Marked = GetNotifySeconds(Animation, Names);
		if (Marked >= 0.0f)
		{
			DeathFallSeconds[Index] = Marked;
		}
	}
}

float ACrowdRenderer::GetDeathFallSeconds(EDeathFall Fall) const
{
	const int32 Index = static_cast<int32>(Fall);
	return bDeathsTried && Index >= 0 && Index < static_cast<int32>(EDeathFall::Count) ? DeathFallSeconds[Index] : FDeathPick::FallSeconds(Fall);
}

void ACrowdRenderer::RefreshCorpses(const FCrowdSim& Sim, float GroundZ, float WizardHeight)
{
	if (!bDeathsTried)
	{
		SetUpDeaths();
	}
	const float Delta = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.0f;

	// Those that died in this step fall where they stood, facing as they faced.
	for (const FCrowdEvent& Event : Sim.GetEvents())
	{
		const int32 Kind = Event.Type == ECrowdEvent::Death ? FindModelKind(Event.Kind) : INDEX_NONE;
		if (Kind == INDEX_NONE || !bModelLoaded[Kind] || !IsArmyModelShown())
		{
			continue;
		}
		const EDeathFall Fall = FDeathPick::Pick(Event.Facing, Event.Position, Event.From);
		UAnimSequenceBase* Animation = DeathAnimations[static_cast<int32>(Fall)];
		if (!Animation)
		{
			continue;
		}
		if (Corpses.Num() >= CorpseCap)
		{
			// Full: the one that has lain longest gives up its place.
			int32 Oldest = 0;
			for (int32 Index = 1; Index < Corpses.Num(); ++Index)
			{
				Oldest = Corpses[Index].Age > Corpses[Oldest].Age ? Index : Oldest;
			}
			CorpseFigures[Corpses[Oldest].Figure]->SetVisibility(false, true);
			CorpseBusy[Corpses[Oldest].Figure] = false;
			Corpses.RemoveAtSwap(Oldest);
		}
		// A figure that is free: one that already wears this kind's model before any other, else a new one.
		int32 Figure = INDEX_NONE;
		for (int32 Index = 0; Index < CorpseFigures.Num(); ++Index)
		{
			if (!CorpseBusy[Index] && (Figure == INDEX_NONE || CorpseFigures[Index]->GetSkeletalMeshAsset() == ModelMeshes[Kind]))
			{
				Figure = Index;
				if (CorpseFigures[Index]->GetSkeletalMeshAsset() == ModelMeshes[Kind])
				{
					break;
				}
			}
		}
		if (Figure == INDEX_NONE)
		{
			USkeletalMeshComponent* Model = NewObject<USkeletalMeshComponent>(this);
			Model->SetupAttachment(GetRootComponent());
			Model->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Model->SetCastShadow(false);
			Model->SetGenerateOverlapEvents(false);
			Model->SetCanEverAffectNavigation(false);
			Model->SetSkeletalMeshAsset(ModelMeshes[Kind]);
			Model->SetAnimationMode(EAnimationMode::AnimationSingleNode);
			Model->RegisterComponent();
			UStaticMeshComponent* Weapon = NewObject<UStaticMeshComponent>(this);
			Weapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Weapon->SetCastShadow(false);
			Weapon->SetGenerateOverlapEvents(false);
			Weapon->SetCanEverAffectNavigation(false);
			Weapon->SetupAttachment(Model);
			Weapon->RegisterComponent();
			Figure = CorpseFigures.Add(Model);
			CorpseWeapons.Add(Weapon);
			CorpseBusy.Add(false);
		}
		USkeletalMeshComponent* Model = CorpseFigures[Figure];
		if (Model->GetSkeletalMeshAsset() != ModelMeshes[Kind])
		{
			Model->SetSkeletalMeshAsset(ModelMeshes[Kind]);
		}
		// It keeps what it held as it falls.
		UStaticMeshComponent* Weapon = CorpseWeapons[Figure];
		Weapon->SetStaticMesh(WeaponMeshes[Kind]);
		if (WeaponMeshes[Kind])
		{
			Weapon->AttachToComponent(Model, FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponSockets[Kind]);
		}
		Weapon->SetVisibility(WeaponMeshes[Kind] != nullptr);

		// It faces the way its figure faced (which lags the simulation's turn a little), or the way the simulation says.
		const FArmyModelState* State = ArmyStates.Find(Event.Id);
		const float Yaw = State ? State->Yaw : Event.Facing.IsNearlyZero() ? 0.0f : FMath::RadiansToDegrees(static_cast<float>(FMath::Atan2(Event.Facing.Y, Event.Facing.X)));
		FCorpse& Corpse = Corpses.AddDefaulted_GetRef();
		Corpse.Figure = Figure;
		Corpse.Length = Animation->GetPlayLength();
		Corpse.Height = Sim.GetUnitHeight(Event.Kind, WizardHeight);
		Corpse.Place = FVector(Event.Position.X, Event.Position.Y, GetFloorZ(Event.Position, GroundZ));
		Model->SetWorldTransform(FTransform(FRotator(0.0, Yaw + ArmyModelYaw, 0.0), Corpse.Place, FVector(Corpse.Height / ModelAssetHeights[Kind])));
		Model->SetVisibility(true, true);
		Weapon->SetVisibility(WeaponMeshes[Kind] != nullptr);
		// Its place in the death is set from its age every frame; it does not play by itself (see GetCorpseSeconds).
		Model->SetAnimation(Animation);
		Model->SetPlayRate(0.0f);
		Model->SetPosition(0.0f, false);
		CorpseBusy[Figure] = true;
		++DeathsShown;
		LastDeath = Fall;
	}

	// Each lies a while after its fall, then sinks into the ground, slowly at first, and is gone.
	for (int32 Index = Corpses.Num() - 1; Index >= 0; --Index)
	{
		FCorpse& Corpse = Corpses[Index];
		Corpse.Age += Delta;
		const float Sink = FMath::Clamp((Corpse.Age - Corpse.Length - CorpseRestSeconds) / CorpseSinkSeconds, 0.0f, 1.0f);
		USkeletalMeshComponent* Model = CorpseFigures[Corpse.Figure];
		if (Sink >= 1.0f || !IsArmyModelShown())
		{
			Model->SetVisibility(false, true);
			CorpseBusy[Corpse.Figure] = false;
			Corpses.RemoveAtSwap(Index);
			continue;
		}
		Model->SetPosition(GetCorpseSeconds(Corpse.Age, Corpse.Length), false);
		if (Sink > 0.0f)
		{
			Model->SetWorldLocation(Corpse.Place - FVector(0.0, 0.0, Corpse.Height * 0.6 * Sink * Sink));
		}
	}
}

ACrowdRenderer::ACrowdRenderer()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	const auto MakeInstances = [this](const TCHAR* Name)
	{
		UInstancedStaticMeshComponent* Component = CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
		Component->SetupAttachment(GetRootComponent());
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetCastShadow(false);
		Component->SetGenerateOverlapEvents(false);
		Component->SetCanEverAffectNavigation(false);
		return Component;
	};
	Skeletons = MakeInstances(TEXT("Skeletons"));
	Grunts = MakeInstances(TEXT("Grunts"));
	Bolts = MakeInstances(TEXT("Bolts"));
	Bones = MakeInstances(TEXT("Bones"));
	Brutes = MakeInstances(TEXT("Brutes"));
	Guards = MakeInstances(TEXT("Guards"));
	Commanders = MakeInstances(TEXT("Commanders"));
	Archers = MakeInstances(TEXT("Archers"));
	Assassins = MakeInstances(TEXT("Assassins"));
	Arrows = MakeInstances(TEXT("Arrows"));
	ArmyArrows = MakeInstances(TEXT("Army Arrows"));
	Javelins = MakeInstances(TEXT("Javelins"));
	Spears = MakeInstances(TEXT("Spears"));
	Blasts = MakeInstances(TEXT("Blasts"));
	Slashes = MakeInstances(TEXT("Slashes"));
	SlamRings = MakeInstances(TEXT("Slam Rings"));
	SlamFulls = MakeInstances(TEXT("Slam Fulls"));
	ThrowLanes = MakeInstances(TEXT("Throw Lanes"));
	SweepFans = MakeInstances(TEXT("Sweep Fans"));
	ThrowLocks = MakeInstances(TEXT("Throw Locks"));
	SlamFlashes = MakeInstances(TEXT("Slam Flashes"));
	BruteBacks = MakeInstances(TEXT("Brute Backs"));

	// Walls are the one crowd shape with collision: they stop the wizard. Units and bolts are stopped by the simulation.
	Walls = MakeInstances(TEXT("Walls"));
	Walls->SetCollisionProfileName(TEXT("BlockAll"));

	// The level: one instanced component for each kind of piece. Those that block also stop the wizard.
	for (int32 Index = 0; Index < static_cast<int32>(ELevelPiece::Count); ++Index)
	{
		const ELevelPiece Kind = static_cast<ELevelPiece>(Index);
		PieceMeshes[Index] = MakeInstances(*FString::Printf(TEXT("Level %s"), FLevelLayout::GetPieceName(Kind)));
		if (FLevelLayout::Blocks(Kind) && !FLevelLayout::IsLow(Kind))
		{
			PieceMeshes[Index]->SetCollisionProfileName(TEXT("BlockAll"));
		}
	}
	TreeTops = MakeInstances(TEXT("Level Tree Tops"));
	LampGlows = MakeInstances(TEXT("Level Lamp Glows"));
	WaterBlockers = MakeInstances(TEXT("Level Water Blockers"));
	WaterBlockers->SetCollisionProfileName(TEXT("BlockAll"));
	WaterBlockers->SetHiddenInGame(true);

	Ground = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ground"));
	Ground->SetupAttachment(GetRootComponent());
	// The wizard is a Character and walks on this plate; the map's own floor is far smaller than a level. Only pawns
	// and the feet's placement traces see it, so every other trace still finds what it found before.
	Ground->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Ground->SetCollisionObjectType(ECC_WorldStatic);
	Ground->SetCollisionResponseToAllChannels(ECR_Ignore);
	Ground->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	Ground->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Ground->SetCastShadow(false);
	Ground->SetRelativeScale3D(FVector(600.0, 600.0, 1.0));

	const auto MakeShape = [this](const TCHAR* Name, const FVector& Scale)
	{
		UStaticMeshComponent* Component = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Component->SetupAttachment(GetRootComponent());
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetCastShadow(false);
		Component->SetRelativeScale3D(Scale);
		return Component;
	};
	ShrineBase = MakeShape(TEXT("Shrine Base"), ShrineBaseScale);
	ShrinePillar = MakeShape(TEXT("Shrine Pillar"), ShrinePillarScale);
}

void ACrowdRenderer::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	const UWorld* World = GetWorld();
	if (!World || World->IsGameWorld())
	{
		return;
	}
	// In the editor: the Outskirts as the game will lay it out, standing on this actor's height.
	const float GroundZ = static_cast<float>(GetActorLocation().Z);
	SetUpShapes(GroundZ);
	// Under the level's own floor here, so the two do not flicker; the game puts it back.
	Ground->SetWorldLocation(FVector(0.0, 0.0, GroundZ - 1.0));
	const FLevelLayout Layout = FLevelLayout::MakeOutskirts();
	// Once the pieces are actors in the level, they are what is seen here: this actor draws none of its own.
	Pieces = bUsePlacedPieces ? TArray<FLevelPiece>() : Layout.Pieces;
	ShrinePosition = Layout.Shrine;
	SetShrineShown(!bUsePlacedPieces);
	RebuildPieces(GroundZ);
	ShrineBase->SetWorldLocation(FVector(ShrinePosition.X, ShrinePosition.Y, GroundZ + ShrineBaseScale.Z * 50.0 + 12.0));
	ShrinePillar->SetWorldLocation(FVector(ShrinePosition.X, ShrinePosition.Y, GroundZ + ShrinePillarScale.Z * 50.0));
}

void ACrowdRenderer::SetShrineShown(bool bShown)
{
	ShrineBase->SetVisibility(bShown);
	ShrinePillar->SetVisibility(bShown);
}

#if WITH_EDITOR
void ACrowdRenderer::PlaceSystemsAsActors()
{
	UWorld* World = GetWorld();
	if (!World || World->IsGameWorld())
	{
		return;
	}
	for (TActorIterator<AEnemyPostActor> It(World); It; ++It)
	{
		// Already done.
		return;
	}
	const FLevelLayout Layout = FLevelLayout::MakeOutskirts();
	const FVector Base = GetActorLocation();
	FActorSpawnParameters Params;
	Params.OverrideLevel = GetLevel();
	const float ArcherShare = FCrowdTuning().PostArcherShare;

	if (AShrineActor* Shrine = World->SpawnActor<AShrineActor>(FVector(Layout.Shrine.X, Layout.Shrine.Y, Base.Z + 2.0), FRotator::ZeroRotator, Params))
	{
		Shrine->GetMesh()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Buildings/SM_Shrine.SM_Shrine")));
		Shrine->SetActorLabel(TEXT("Shrine"));
		Shrine->SetFolderPath(TEXT("Systems"));
		Shrine->RerunConstructionScripts();
	}

	const auto MakePost = [&](FVector2D Where, int32 Grunts, int32 Brutes, bool bCommander, const FString& Label)
	{
		AEnemyPostActor* Post = World->SpawnActor<AEnemyPostActor>(FVector(Where.X, Where.Y, Base.Z + 2.0), FRotator::ZeroRotator, Params);
		if (!Post)
		{
			return;
		}
		// As the game did it: a share of a post's grunts are archers.
		Post->Archers = FMath::Clamp(FMath::RoundToInt32(Grunts * ArcherShare), 0, Grunts);
		Post->Grunts = Grunts - Post->Archers;
		Post->Brutes = Brutes;
		Post->bCommander = bCommander;
		Post->Radius = AEnemyPostActor::GetRoomFor(Post->GetCount());
		Post->SetActorLabel(Label);
		Post->SetFolderPath(TEXT("Systems/Posts"));
		Post->RerunConstructionScripts();
	};
	for (int32 Index = 0; Index < Layout.Posts.Num(); ++Index)
	{
		MakePost(Layout.Posts[Index].Position, Layout.Posts[Index].Grunts, Layout.Posts[Index].Brutes, false, FString::Printf(TEXT("Post %02d"), Index + 1));
	}
	if (Layout.bHasCommander)
	{
		MakePost(Layout.Commander, Layout.EscortGrunts, Layout.EscortBrutes, true, TEXT("Commander Post"));
	}

	for (int32 Index = 0; Index < Layout.Routes.Num(); ++Index)
	{
		if (APatrolRouteActor* Route = World->SpawnActor<APatrolRouteActor>(FVector(0.0, 0.0, Base.Z + 30.0), FRotator::ZeroRotator, Params))
		{
			Route->SetPoints(Layout.Routes[Index]);
			Route->PatrolSize = FMath::RoundToInt32(FCrowdTuning().RoadPatrolSize);
			Route->SetActorLabel(FString::Printf(TEXT("Patrol Road %02d"), Index + 1));
			Route->SetFolderPath(TEXT("Systems/Patrol Roads"));
		}
	}

	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		const FVector Location = It->GetActorLocation();
		It->SetActorLocation(FVector(Layout.Start.X, Layout.Start.Y, Location.Z));
		break;
	}
	RerunConstructionScripts();
}

void ACrowdRenderer::PlaceGateAsActor()
{
	UWorld* World = GetWorld();
	if (!World || World->IsGameWorld())
	{
		return;
	}
	for (TActorIterator<AKingdomGateActor> It(World); It; ++It)
	{
		// Already done.
		return;
	}
	const FLevelLayout Layout = FLevelLayout::MakeOutskirts();
	if (!Layout.bHasGate)
	{
		return;
	}
	FActorSpawnParameters Params;
	Params.OverrideLevel = GetLevel();
	const FVector Where(Layout.Gate.X, Layout.Gate.Y, GetActorLocation().Z + 2.0);
	if (AKingdomGateActor* Gate = World->SpawnActor<AKingdomGateActor>(Where, FRotator(0.0, Layout.GateYaw, 0.0), Params))
	{
		Gate->Depth = static_cast<float>(Layout.GateHalfSize.X * 2.0);
		Gate->Width = static_cast<float>(Layout.GateHalfSize.Y * 2.0);
		Gate->SetActorLabel(TEXT("Kingdom Gate"));
		Gate->SetFolderPath(TEXT("Systems"));
		Gate->RerunConstructionScripts();
	}
}

int32 ACrowdRenderer::TightenBuildingCollision()
{
	UWorld* World = GetWorld();
	if (!World || World->IsGameWorld())
	{
		return 0;
	}
	TSet<UStaticMesh*> Done;
	for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
	{
		UStaticMeshComponent* Component = It->GetStaticMeshComponent();
		UStaticMesh* Mesh = Component ? Component->GetStaticMesh() : nullptr;
		if (!Mesh || Done.Contains(Mesh) || !Mesh->GetPathName().StartsWith(TEXT("/Game/Buildings/")))
		{
			continue;
		}
		if (!Mesh->GetBodySetup())
		{
			Mesh->CreateBodySetup();
		}
		UBodySetup* Body = Mesh->GetBodySetup();
		if (!Body)
		{
			continue;
		}
		// One box: the building's walls, without its eaves. As tall as the building.
		const FBox Bounds = Mesh->GetBoundingBox();
		FKBoxElem Box(Bounds.GetSize().X * UCrowdSubsystem::BuildingBodyShare, Bounds.GetSize().Y * UCrowdSubsystem::BuildingBodyShare, Bounds.GetSize().Z);
		Box.Center = Bounds.GetCenter();
		Mesh->Modify();
		Body->Modify();
		Body->AggGeom.EmptyElements();
		Body->AggGeom.BoxElems.Add(Box);
		Body->InvalidatePhysicsData();
		Body->CreatePhysicsMeshes();
		Mesh->MarkPackageDirty();
		Done.Add(Mesh);
	}
	return Done.Num();
}

void ACrowdRenderer::RefreshPlacedPieces()
{
	UWorld* World = GetWorld();
	if (!World || World->IsGameWorld())
	{
		return;
	}
	for (TActorIterator<ALevelPieceActor> It(World); It; ++It)
	{
		It->RerunConstructionScripts();
	}
	for (TActorIterator<AWallActor> It(World); It; ++It)
	{
		It->RerunConstructionScripts();
	}
}

void ACrowdRenderer::PlaceWallsAsActors()
{
	UWorld* World = GetWorld();
	if (!World || World->IsGameWorld())
	{
		return;
	}
	for (TActorIterator<AWallActor> It(World); It; ++It)
	{
		// Already done.
		return;
	}
	const FLevelLayout Layout = FLevelLayout::MakeOutskirts();
	FActorSpawnParameters Params;
	Params.OverrideLevel = GetLevel();
	int32 Number = 0;
	for (const FCrowdObstacle& Wall : Layout.Walls)
	{
		if (Wall.bLow)
		{
			continue;
		}
		if (AWallActor* Placed = World->SpawnActor<AWallActor>(FVector(Wall.Center.X, Wall.Center.Y, GetActorLocation().Z), FRotator::ZeroRotator, Params))
		{
			Placed->Size = Wall.HalfSize * 2.0;
			Placed->SetActorLabel(FString::Printf(TEXT("Wall %02d"), ++Number));
			Placed->SetFolderPath(TEXT("Walls"));
			Placed->RerunConstructionScripts();
		}
	}
}

FString ACrowdRenderer::BakeGroundPaint(AActor* LandscapeActor, int32 Size, const TArray<FString>& DirtLayers)
{
	ALandscapeProxy* Land = Cast<ALandscapeProxy>(LandscapeActor);
	ULandscapeInfo* Info = Land ? Land->GetLandscapeInfo() : nullptr;
	if (!Info || Size <= 0 || Size > 512)
	{
		return FString();
	}
	TArray<ULandscapeLayerInfoObject*> Dirt;
	TArray<ULandscapeLayerInfoObject*> Others;
	for (const FLandscapeInfoLayerSettings& Layer : Info->Layers)
	{
		if (Layer.LayerInfoObj)
		{
			(DirtLayers.Contains(Layer.GetLayerName().ToString()) ? Dirt : Others).Add(Layer.LayerInfoObj);
		}
	}
	FBox Bounds(ForceInit);
	TArray<ULandscapeComponent*> Components;
	for (ULandscapeComponent* Component : Land->LandscapeComponents)
	{
		if (Component)
		{
			Components.Add(Component);
			Bounds += Component->Bounds.GetBox();
		}
	}
	if (Components.Num() == 0 || Dirt.Num() == 0)
	{
		return FString();
	}
	// Each layer of each component is read once and kept.
	TMap<TPair<ULandscapeComponent*, ULandscapeLayerInfoObject*>, TArray<uint8>> Caches;
	const auto WeightOf = [&](ULandscapeComponent* Component, const TArray<ULandscapeLayerInfoObject*>& PaintLayers, const FVector& Location)
	{
		float Sum = 0.0f;
		for (ULandscapeLayerInfoObject* Layer : PaintLayers)
		{
			Sum += Component->GetLayerWeightAtLocation(Location, Layer, &Caches.FindOrAdd(TPair<ULandscapeComponent*, ULandscapeLayerInfoObject*>(Component, Layer)));
		}
		return Sum;
	};
	FString Cells;
	Cells.Reserve(Size * Size);
	int32 DirtCells = 0;
	for (int32 Y = 0; Y < Size; ++Y)
	{
		for (int32 X = 0; X < Size; ++X)
		{
			const FVector Location(FMath::Lerp(Bounds.Min.X, Bounds.Max.X, (X + 0.5) / Size), FMath::Lerp(Bounds.Min.Y, Bounds.Max.Y, (Y + 0.5) / Size), Bounds.GetCenter().Z);
			bool bDirt = false;
			for (ULandscapeComponent* Component : Components)
			{
				const FBox Box = Component->Bounds.GetBox();
				if (Location.X >= Box.Min.X && Location.X <= Box.Max.X && Location.Y >= Box.Min.Y && Location.Y <= Box.Max.Y)
				{
					bDirt = WeightOf(Component, Dirt, Location) > WeightOf(Component, Others, Location);
					break;
				}
			}
			DirtCells += bDirt ? 1 : 0;
			Cells.AppendChar(bDirt ? TEXT('1') : TEXT('0'));
		}
	}
	UE_LOG(LogTemp, Display, TEXT("Ground paint: %d of %d cells are dirt (%d dirt layers, %d others)"), DirtCells, Size * Size, Dirt.Num(), Others.Num());
	return FString::Printf(TEXT("%.1f,%.1f,%.1f,%.1f|%s"), Bounds.Min.X, Bounds.Min.Y, Bounds.Max.X, Bounds.Max.Y, *Cells);
}

int32 ACrowdRenderer::RebuildBridgeCollision()
{
	UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Props/SM_Bridge.SM_Bridge"));
	const FStaticMeshRenderData* RenderData = Mesh ? Mesh->GetRenderData() : nullptr;
	if (!RenderData || RenderData->LODResources.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Bridge collision: the model has nothing to read"));
		return 0;
	}
	const FStaticMeshLODResources& Lod = RenderData->LODResources[0];
	const FPositionVertexBuffer& Vertices = Lod.VertexBuffers.PositionVertexBuffer;
	const FIndexArrayView Indices = Lod.IndexBuffer.GetArrayView();
	const FBox Bounds = Mesh->GetBoundingBox();
	if (Indices.Num() < 3 || Bounds.GetSize().X < Bounds.GetSize().Y)
	{
		// The bridge is expected to lie along its X.
		UE_LOG(LogTemp, Warning, TEXT("Bridge collision: %d indices, %d vertices, size %s"), Indices.Num(), Vertices.GetNumVertices(), *Bounds.GetSize().ToString());
		return 0;
	}
	// The top of the model over a place: the highest triangle above it. False where the model has nothing.
	const auto TopAt = [&](double X, double Y, double& OutZ)
	{
		bool bAny = false;
		for (int32 Index = 0; Index + 2 < Indices.Num(); Index += 3)
		{
			const FVector A(Vertices.VertexPosition(Indices[Index]));
			const FVector B(Vertices.VertexPosition(Indices[Index + 1]));
			const FVector C(Vertices.VertexPosition(Indices[Index + 2]));
			if (X < FMath::Min3(A.X, B.X, C.X) || X > FMath::Max3(A.X, B.X, C.X) || Y < FMath::Min3(A.Y, B.Y, C.Y) || Y > FMath::Max3(A.Y, B.Y, C.Y))
			{
				continue;
			}
			const double Det = (B.Y - C.Y) * (A.X - C.X) + (C.X - B.X) * (A.Y - C.Y);
			if (FMath::IsNearlyZero(Det))
			{
				continue;
			}
			const double U = ((B.Y - C.Y) * (X - C.X) + (C.X - B.X) * (Y - C.Y)) / Det;
			const double V = ((C.Y - A.Y) * (X - C.X) + (A.X - C.X) * (Y - C.Y)) / Det;
			if (U < 0.0 || V < 0.0 || U + V > 1.0)
			{
				continue;
			}
			const double Z = U * A.Z + V * B.Z + (1.0 - U - V) * C.Z;
			OutZ = bAny ? FMath::Max(OutZ, Z) : Z;
			bAny = true;
		}
		return bAny;
	};

	const double HalfWidth = Bounds.GetExtent().Y;
	const double MiddleY = Bounds.GetCenter().Y;
	// The height of the deck at even steps along the bridge: the middle one of a few readings across its middle.
	constexpr int32 Segments = 12;
	const auto DeckAt = [&](double X, double& OutZ)
	{
		TArray<double, TInlineAllocator<5>> Readings;
		for (int32 Across = -2; Across <= 2; ++Across)
		{
			double Z = 0.0;
			if (TopAt(X, MiddleY + Across * HalfWidth * 0.1, Z))
			{
				Readings.Add(Z);
			}
		}
		if (Readings.Num() < 3)
		{
			return false;
		}
		Readings.Sort();
		OutZ = Readings[Readings.Num() / 2];
		return true;
	};
	// The deck can stop short of the ends of the model (corner posts stand out past it): where it starts and ends.
	double DeckStart = Bounds.Min.X;
	double DeckEnd = Bounds.Max.X;
	double Unused = 0.0;
	while (DeckStart < DeckEnd && !DeckAt(DeckStart, Unused))
	{
		DeckStart += 4.0;
	}
	while (DeckEnd > DeckStart && !DeckAt(DeckEnd, Unused))
	{
		DeckEnd -= 4.0;
	}
	if (DeckEnd - DeckStart < Bounds.GetSize().X * 0.5)
	{
		UE_LOG(LogTemp, Warning, TEXT("Bridge collision: no deck found (%d indices)"), Indices.Num());
		return 0;
	}
	TArray<double> Heights;
	TArray<double> Along;
	for (int32 Node = 0; Node <= Segments; ++Node)
	{
		const double X = FMath::Lerp(DeckStart, DeckEnd, static_cast<double>(Node) / Segments);
		double Z = 0.0;
		if (!DeckAt(X, Z))
		{
			// A gap in the deck: as high as the reading before it.
			Z = Heights.Num() > 0 ? Heights.Last() : 0.0;
		}
		Along.Add(X);
		Heights.Add(Z);
	}

	// No part of the deck is steeper than an easy ramp: from the middle outward, a reading that drops faster than
	// that is lifted. (The model's ends can fall away sharply; a walker needs a slope it can always climb.)
	const double SteepestSlope = 0.5;
	for (int32 Node = Segments / 2 + 1; Node <= Segments; ++Node)
	{
		Heights[Node] = FMath::Max(Heights[Node], Heights[Node - 1] - SteepestSlope * (Along[Node] - Along[Node - 1]));
	}
	for (int32 Node = Segments / 2 - 1; Node >= 0; --Node)
	{
		Heights[Node] = FMath::Max(Heights[Node], Heights[Node + 1] - SteepestSlope * (Along[Node + 1] - Along[Node]));
	}

	// Where the rails begin: going out from the middle, the first place the model stands well above the deck, or ends.
	const double RailRise = 30.0;
	double LaneHalf = HalfWidth;
	for (int32 Node = 3; Node <= Segments - 3; ++Node)
	{
		for (const double Side : { -1.0, 1.0 })
		{
			for (double Out = 0.0; Out < HalfWidth; Out += 4.0)
			{
				double Z = 0.0;
				if (!TopAt(Along[Node], MiddleY + Side * Out, Z) || Z > Heights[Node] + RailRise)
				{
					LaneHalf = FMath::Min(LaneHalf, Out - 4.0);
					break;
				}
			}
		}
	}
	LaneHalf = FMath::Clamp(LaneHalf, HalfWidth * 0.4, HalfWidth - 10.0);

	if (!Mesh->GetBodySetup())
	{
		Mesh->CreateBodySetup();
	}
	UBodySetup* Body = Mesh->GetBodySetup();
	if (!Body)
	{
		return 0;
	}
	Mesh->Modify();
	Body->Modify();
	Body->AggGeom.EmptyElements();

	// One flat ramp between each pair of readings, its top through both. The simulation reads a box named Deck as
	// something to walk on, and any other as something in the way.
	const double DeckThickness = 60.0;
	const auto AddRamp = [&](double X0, double Z0, double X1, double Z1)
	{
		// A wedge given by its corners, not a tilted box: the bridge is squashed in the level, and only corners
		// squash truly.
		FKConvexElem Ramp;
		for (const double Side : { -1.0, 1.0 })
		{
			const double Y = MiddleY + Side * HalfWidth;
			Ramp.VertexData.Add(FVector(X0, Y, Z0));
			Ramp.VertexData.Add(FVector(X1, Y, Z1));
			Ramp.VertexData.Add(FVector(X0, Y, Z0 - DeckThickness));
			Ramp.VertexData.Add(FVector(X1, Y, Z1 - DeckThickness));
		}
		Ramp.UpdateElemBox();
		Ramp.SetName(TEXT("Deck"));
		Body->AggGeom.ConvexElems.Add(Ramp);
	};
	for (int32 Node = 0; Node < Segments; ++Node)
	{
		AddRamp(Along[Node], Heights[Node], Along[Node + 1], Heights[Node + 1]);
	}
	// Past each end the deck runs on down, at least as steeply as a gentle ramp: wherever the ground meets the
	// bridge there is a slope to walk up and no step.
	const double ApronLength = 250.0;
	const double StartSlope = FMath::Clamp((Heights[1] - Heights[0]) / (Along[1] - Along[0]), 0.25, SteepestSlope);
	const double EndSlope = FMath::Clamp((Heights[Segments - 1] - Heights[Segments]) / (Along[Segments] - Along[Segments - 1]), 0.25, SteepestSlope);
	AddRamp(Along[0] - ApronLength, Heights[0] - StartSlope * ApronLength, Along[0], Heights[0]);
	AddRamp(Along[Segments], Heights[Segments], Along[Segments] + ApronLength, Heights[Segments] - EndSlope * ApronLength);

	// The rails: one thin box down each side, from the edge of the lane to the edge of the model.
	const double RailWidth = HalfWidth - LaneHalf;
	for (const double Side : { -1.0, 1.0 })
	{
		FKBoxElem Rail(Bounds.GetSize().X, RailWidth, Bounds.GetSize().Z + 200.0);
		Rail.Center = FVector(Bounds.GetCenter().X, MiddleY + Side * (LaneHalf + RailWidth * 0.5), Bounds.GetCenter().Z + 100.0);
		Rail.SetName(TEXT("Rail"));
		Body->AggGeom.BoxElems.Add(Rail);
	}
	Body->InvalidatePhysicsData();
	Body->CreatePhysicsMeshes();
	Mesh->MarkPackageDirty();
	UE_LOG(LogTemp, Display, TEXT("Bridge collision: lane half width %.0f of %.0f, deck from %.0f to %.0f high"), LaneHalf, HalfWidth, Heights[0], Heights[Segments / 2]);
	return Body->AggGeom.BoxElems.Num() + Body->AggGeom.ConvexElems.Num();
}

bool ACrowdRenderer::MakeMeshSolid(UStaticMesh* Mesh)
{
	if (!Mesh)
	{
		return false;
	}
	if (!Mesh->GetBodySetup())
	{
		Mesh->CreateBodySetup();
	}
	UBodySetup* Body = Mesh->GetBodySetup();
	if (!Body)
	{
		return false;
	}
	if (Body->AggGeom.GetElementCount() == 0)
	{
		const FBox Bounds = Mesh->GetBoundingBox();
		FKBoxElem Box(Bounds.GetSize().X, Bounds.GetSize().Y, Bounds.GetSize().Z);
		Box.Center = Bounds.GetCenter();
		Mesh->Modify();
		Body->Modify();
		Body->AggGeom.BoxElems.Add(Box);
		Body->InvalidatePhysicsData();
		Body->CreatePhysicsMeshes();
		Mesh->MarkPackageDirty();
	}
	return true;
}

void ACrowdRenderer::RefreshPlacedActors()
{
	UWorld* World = GetWorld();
	if (!World || World->IsGameWorld())
	{
		return;
	}
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->IsA<ALevelPieceActor>() || It->IsA<AEnemyPostActor>() || It->IsA<AShrineActor>() || It->IsA<AKingdomGateActor>())
		{
			It->RerunConstructionScripts();
		}
	}
	RerunConstructionScripts();
}

TArray<FVector2D> ACrowdRenderer::GetLayoutKeyPoints()
{
	const FLevelLayout Layout = FLevelLayout::MakeOutskirts();
	TArray<FVector2D> Points = { Layout.Start, Layout.Shrine };
	if (Layout.bHasCommander)
	{
		Points.Add(Layout.Commander);
	}
	for (const FLevelPost& Post : Layout.Posts)
	{
		Points.Add(Post.Position);
	}
	for (const TArray<FVector2D>& Route : Layout.Routes)
	{
		Points.Append(Route);
	}
	return Points;
}

void ACrowdRenderer::PlacePiecesAsActors()
{
	UWorld* World = GetWorld();
	if (!World || World->IsGameWorld())
	{
		return;
	}
	for (TActorIterator<ALevelPieceActor> It(World); It; ++It)
	{
		// Already done: placing them again would double every piece.
		bUsePlacedPieces = true;
		RerunConstructionScripts();
		return;
	}
	Modify();
	const FLevelLayout Layout = FLevelLayout::MakeOutskirts();
	int32 Counts[static_cast<int32>(ELevelPiece::Count)] = {};
	const FVector Base = GetActorLocation();
	for (const FLevelPiece& Piece : Layout.Pieces)
	{
		FActorSpawnParameters Params;
		Params.OverrideLevel = GetLevel();
		ALevelPieceActor* Actor = World->SpawnActor<ALevelPieceActor>(FVector(Piece.Center.X, Piece.Center.Y, Base.Z), FRotator::ZeroRotator, Params);
		if (!Actor)
		{
			continue;
		}
		Actor->SetPiece(Piece);
		const FString KindName = FLevelLayout::GetPieceName(Piece.Kind);
		Actor->SetActorLabel(FString::Printf(TEXT("%s %d"), *KindName, ++Counts[static_cast<int32>(Piece.Kind)]));
		Actor->SetFolderPath(FName(*FString::Printf(TEXT("Layout/%s"), *KindName)));
		Actor->RerunConstructionScripts();
	}
	bUsePlacedPieces = true;
	RerunConstructionScripts();
}
#endif

void ACrowdRenderer::SetUpShapes(float GroundZ)
{
	bShapesReady = true;
	Skeletons->SetStaticMesh(WizardShapes::Load(TEXT("Cylinder")));
	Grunts->SetStaticMesh(WizardShapes::Load(TEXT("Cube")));
	Bolts->SetStaticMesh(WizardShapes::Load(TEXT("Sphere")));
	// The director's models where they are in the project; the plain tinted shapes where one is missing.
	const auto Dress = [](UInstancedStaticMeshComponent* Instances, const TCHAR* ModelPath, const TCHAR* Shape, const FLinearColor& Color)
	{
		if (UStaticMesh* Model = LoadObject<UStaticMesh>(nullptr, ModelPath))
		{
			Instances->SetStaticMesh(Model);
			return true;
		}
		Instances->SetStaticMesh(WizardShapes::Load(Shape));
		WizardShapes::Tint(Instances, Color);
		return false;
	};
	bBoneModel = Dress(Bones, TEXT("/Game/Props/SM_Bone_Pile.SM_Bone_Pile"), TEXT("Cube"), BoneColor);
	Brutes->SetStaticMesh(WizardShapes::Load(TEXT("Cube")));
	WizardShapes::Tint(Brutes, BruteColor);
	Guards->SetStaticMesh(WizardShapes::Load(TEXT("Cylinder")));
	WizardShapes::Tint(Guards, GuardColor);
	Commanders->SetStaticMesh(WizardShapes::Load(TEXT("Cube")));
	WizardShapes::Tint(Commanders, CommanderColor);
	Archers->SetStaticMesh(WizardShapes::Load(TEXT("Cone")));
	WizardShapes::Tint(Archers, ArcherColor);
	Assassins->SetStaticMesh(WizardShapes::Load(TEXT("Cone")));
	WizardShapes::Tint(Assassins, AssassinColor);
	bArrowModel = Dress(Arrows, TEXT("/Game/Weapons/SM_Archer_Arrow.SM_Archer_Arrow"), TEXT("Cylinder"), ArrowColor);
	bArmyArrowModel = Dress(ArmyArrows, TEXT("/Game/Weapons/SM_Marksman_Arrow.SM_Marksman_Arrow"), TEXT("Cylinder"), ArrowColor);
	bJavelinModel = Dress(Javelins, TEXT("/Game/Weapons/SM_Commander_Spear.SM_Commander_Spear"), TEXT("Cylinder"), ArrowColor);
	bSpearModel = Dress(Spears, TEXT("/Game/Weapons/SM_Wizard_Spear.SM_Wizard_Spear"), TEXT("Cylinder"), SpearColor);
	// Marks that lie on the ground: flat planes with a glowing rim, a soft fill and a moving pattern. The enemy's
	// are embers on blood (a warning grows dim, then burns bright for its last moment); the wizard's own is violet.
	const auto Mark = [](UInstancedStaticMeshComponent* Instances, const FLinearColor& Color, const FLinearColor& Glow, bool bRound, float Fill, float Pulse, float Spin)
	{
		Instances->SetStaticMesh(WizardShapes::Load(TEXT("Plane")));
		UMaterialInstanceDynamic* Material = WizardShapes::Effect(Instances, WizardShapes::GroundMarkPath, Glow);
		if (Material)
		{
			Material->SetVectorParameterValue(TEXT("Color"), Color);
			Material->SetVectorParameterValue(TEXT("Glow"), Glow);
			Material->SetScalarParameterValue(TEXT("Round"), bRound ? 1.0f : 0.0f);
			Material->SetScalarParameterValue(TEXT("Fill"), Fill);
			Material->SetScalarParameterValue(TEXT("Pulse"), Pulse);
			Material->SetScalarParameterValue(TEXT("Spin"), Spin);
		}
		return Material;
	};
	Mark(Blasts, SpearMarkColor, SpearMarkGlow, true, 0.55f, 5.0f, -0.35f);
	Mark(SlamRings, SlamRingColor, SlamRingGlow, true, 0.4f, 6.0f, 0.2f);
	Mark(SlamFulls, SlamFullColor, SlamFullGlow, true, 0.5f, 18.0f, 0.7f);
	Mark(ThrowLanes, SlamRingColor, SlamRingGlow, false, 0.35f, 6.0f, 1.2f);
	Mark(ThrowLocks, SlamFullColor, SlamFullGlow, false, 0.5f, 18.0f, 2.4f);
	Mark(SlamFlashes, SlamFlashColor, SlamFlashGlow, true, 0.8f, 0.0f, 0.0f);
	SweepFanMaterial = Mark(SweepFans, SlamRingColor, SlamRingGlow, true, 0.45f, 14.0f, 0.5f);
	Slashes->SetStaticMesh(WizardShapes::Load(TEXT("Cube")));
	WizardShapes::Tint(Slashes, SlashColor);
	BruteBacks->SetStaticMesh(WizardShapes::Load(TEXT("Sphere")));
	WizardShapes::Tint(BruteBacks, BruteBackColor);
	Ground->SetStaticMesh(WizardShapes::Load(TEXT("Plane")));
	Walls->SetStaticMesh(WizardShapes::Load(TEXT("Cube")));
	// Walls, houses and trees are cut away in a circle around the cursor, so what is behind them can be seen and aimed at.
	const auto SeeThrough = [this](UPrimitiveComponent* Component, const FLinearColor& Color)
	{
		UMaterialInstanceDynamic* Material = WizardShapes::TintSeeThrough(Component, Color);
		FLinearColor Value;
		if (Material && Material->GetVectorParameterValue(FName(TEXT("Cursor")), Value))
		{
			SeeThroughMaterials.Add(Material);
		}
	};
	// Walls are not cut away around the cursor (director, 2026-11-14): only houses and trees are.
	WizardShapes::Tint(Walls, WallColor);
	WizardShapes::Tint(Skeletons, SkeletonColor);
	WizardShapes::Tint(Grunts, GruntColor);
	WizardShapes::Effect(Bolts, WizardShapes::FireBoltPath, BoltColor);
	WizardShapes::Tint(Ground, GroundColor);

	for (int32 Index = 0; Index < static_cast<int32>(ELevelPiece::Count); ++Index)
	{
		const FPieceLook& Look = GetPieceLook(static_cast<ELevelPiece>(Index));
		PieceMeshes[Index]->SetStaticMesh(WizardShapes::Load(Look.Shape));
		if (Look.Height >= SeeThroughFromHeight && static_cast<ELevelPiece>(Index) != ELevelPiece::Lamp)
		{
			SeeThrough(PieceMeshes[Index], Look.Color);
		}
		else
		{
			WizardShapes::Tint(PieceMeshes[Index], Look.Color);
		}
	}
	TreeTops->SetStaticMesh(WizardShapes::Load(TEXT("Cone")));
	SeeThrough(TreeTops, TreeTopColor);
	LampGlows->SetStaticMesh(WizardShapes::Load(TEXT("Sphere")));
	WizardShapes::Tint(LampGlows, LampGlowColor);
	WaterBlockers->SetStaticMesh(WizardShapes::Load(TEXT("Cube")));

	// One wide ground just above the floor of the map, so the wizard never walks off the edge of the world.
	Ground->SetWorldLocation(FVector(0.0, 0.0, GroundZ + 2.0));
	// A level with a landscape has its own ground, painted there: this one is put away.
	bool bLandscape = false;
	if (const UWorld* World = GetWorld())
	{
		for (TActorIterator<AActor> It(World); It && !bLandscape; ++It)
		{
			bLandscape = It->GetClass()->GetName().StartsWith(TEXT("Landscape"));
		}
	}
	if (bLandscape)
	{
		// It stays under the landscape, so past the landscape's edge there is still ground to see and to stand on.
		Ground->SetWorldLocation(FVector(0.0, 0.0, GroundZ - GroundUnderLandscape));
	}

	ShrineBase->SetStaticMesh(WizardShapes::Load(TEXT("Cylinder")));
	ShrinePillar->SetStaticMesh(WizardShapes::Load(TEXT("Cylinder")));
	WizardShapes::Tint(ShrineBase, ShrineBaseColor);
	WizardShapes::Tint(ShrinePillar, ShrinePillarColor);
	bShrineChanged = true;
}

void ACrowdRenderer::RebuildPieces(float GroundZ)
{
	TArray<FTransform> Transforms[static_cast<int32>(ELevelPiece::Count)];
	TArray<FTransform> TopTransforms;
	TArray<FTransform> GlowTransforms;
	TArray<FTransform> BlockerTransforms;
	for (const FCrowdObstacle& Blocker : BuildingBlockers)
	{
		BlockerTransforms.Emplace(FQuat::Identity, FVector(Blocker.Center.X, Blocker.Center.Y, GroundZ + BuildingBlockerHeight * 0.5),
			FVector(Blocker.HalfSize.X / 50.0, Blocker.HalfSize.Y / 50.0, BuildingBlockerHeight / 100.0));
	}
	for (const FLevelPiece& Piece : Pieces)
	{
		if (Piece.Kind == ELevelPiece::House && !bShowHouseBoxes)
		{
			continue;
		}
		const FPieceLook& Look = GetPieceLook(Piece.Kind);
		const FQuat Turn = FRotator(0.0, Piece.Yaw, 0.0).Quaternion();
		TArray<FTransform>& Out = Transforms[static_cast<int32>(Piece.Kind)];
		// Basic shapes are 100 units across with the pivot in the middle.
		if (Piece.Kind == ELevelPiece::Tree)
		{
			Out.Emplace(FQuat::Identity, FVector(Piece.Center.X, Piece.Center.Y, GroundZ + Look.Height * 0.5), FVector(TreeTrunkAcross / 100.0, TreeTrunkAcross / 100.0, Look.Height / 100.0));
			TopTransforms.Emplace(FQuat::Identity, FVector(Piece.Center.X, Piece.Center.Y, GroundZ + Look.Height + TreeTopScale.Z * 50.0 - 20.0), TreeTopScale);
		}
		else if (Piece.Kind == ELevelPiece::Lamp)
		{
			Out.Emplace(FQuat::Identity, FVector(Piece.Center.X, Piece.Center.Y, GroundZ + Look.Height * 0.5), FVector(LampPostAcross / 100.0, LampPostAcross / 100.0, Look.Height / 100.0));
			GlowTransforms.Emplace(FQuat::Identity, FVector(Piece.Center.X, Piece.Center.Y, GroundZ + Look.Height + 10.0), LampGlowScale);
		}
		else if (Look.Height > 0.0)
		{
			const double Height = Look.Height * FMath::Max(Piece.HeightScale, 0.05f);
			Out.Emplace(Turn, FVector(Piece.Center.X, Piece.Center.Y, GroundZ + Height * 0.5), FVector(Piece.HalfSize.X / 50.0, Piece.HalfSize.Y / 50.0, Height / 100.0));
		}
		else
		{
			Out.Emplace(Turn, FVector(Piece.Center.X, Piece.Center.Y, GroundZ + Look.Lift), FVector(Piece.HalfSize.X / 50.0, Piece.HalfSize.Y / 50.0, PlateThickness / 100.0));
			if (FLevelLayout::IsLow(Piece.Kind))
			{
				BlockerTransforms.Emplace(FQuat::Identity, FVector(Piece.Center.X, Piece.Center.Y, GroundZ + WaterBlockerHeight * 0.5),
					FVector(Piece.HalfSize.X / 50.0, Piece.HalfSize.Y / 50.0, WaterBlockerHeight / 100.0));
			}
		}
	}
	const auto Fill = [](UInstancedStaticMeshComponent* Component, const TArray<FTransform>& Instances)
	{
		Component->ClearInstances();
		if (Instances.Num() > 0)
		{
			Component->AddInstances(Instances, false, true, false);
		}
	};
	for (int32 Index = 0; Index < static_cast<int32>(ELevelPiece::Count); ++Index)
	{
		Fill(PieceMeshes[Index], Transforms[Index]);
	}
	Fill(TreeTops, TopTransforms);
	Fill(LampGlows, GlowTransforms);
	Fill(WaterBlockers, BlockerTransforms);
}

void ACrowdRenderer::Refresh(const FCrowdSim& Sim, float GroundZ, float WizardHeight, FVector2D WizardPosition)
{
	if (!bShapesReady)
	{
		SetUpShapes(GroundZ);
	}

	if (bShrineChanged)
	{
		bShrineChanged = false;
		ShrineBase->SetWorldLocation(FVector(ShrinePosition.X, ShrinePosition.Y, GroundZ + ShrineBaseScale.Z * 50.0 + 12.0));
		ShrinePillar->SetWorldLocation(FVector(ShrinePosition.X, ShrinePosition.Y, GroundZ + ShrinePillarScale.Z * 50.0));
	}
	if (bPiecesChanged)
	{
		bPiecesChanged = false;
		RebuildPieces(GroundZ);
	}

	if (bWallsChanged)
	{
		bWallsChanged = false;
		WallTransforms.Reset();
		for (const FVector4& Box : WallBoxes)
		{
			// Basic shapes are 100 units across: X and Y of the box are its middle, Z and W its half sizes.
			WallTransforms.Emplace(FQuat::Identity, FVector(Box.X, Box.Y, GroundZ + WallHeight * 0.5), FVector(Box.Z / 50.0, Box.W / 50.0, WallHeight / 100.0));
		}
		Walls->ClearInstances();
		if (WallTransforms.Num() > 0)
		{
			Walls->AddInstances(WallTransforms, false, true, false);
		}
	}

	SkeletonTransforms.Reset();
	GruntTransforms.Reset();
	BoltTransforms.Reset();
	BoneTransforms.Reset();
	BruteTransforms.Reset();
	GuardTransforms.Reset();
	CommanderTransforms.Reset();
	SpearTransforms.Reset();
	BlastTransforms.Reset();
	SlashTransforms.Reset();
	BruteBackTransforms.Reset();
	SlamRingTransforms.Reset();
	SlamFullTransforms.Reset();
	ThrowLaneTransforms.Reset();
	SweepFanTransforms.Reset();
	ThrowLockTransforms.Reset();
	SlamFlashTransforms.Reset();
	ArcherTransforms.Reset();
	AssassinTransforms.Reset();
	ArrowTransforms.Reset();
	ArmyArrowTransforms.Reset();
	JavelinTransforms.Reset();

	// A shape Height tall and Width times that across, standing on the ground.
	const auto Stand = [GroundZ](TArray<FTransform>& Out, const FCrowdUnit& Unit, double Height, double Width)
	{
		Out.Emplace(FQuat::Identity, FVector(Unit.Position.X, Unit.Position.Y, GroundZ + Height * 0.5),
			FVector(Height * Width / 100.0, Height * Width / 100.0, Height / 100.0));
	};
	const double SkeletonHeight = Sim.GetUnitHeight(ECrowdKind::Skeleton, WizardHeight);
	const double GruntHeight = Sim.GetUnitHeight(ECrowdKind::Grunt, WizardHeight);
	const double GuardHeight = Sim.GetUnitHeight(ECrowdKind::Guard, WizardHeight);
	const double BruteHeight = Sim.GetUnitHeight(ECrowdKind::Brute, WizardHeight);
	const double CommanderHeight = Sim.GetUnitHeight(ECrowdKind::Commander, WizardHeight);
	const double ArcherHeight = Sim.GetUnitHeight(ECrowdKind::Archer, WizardHeight);
	const double AssassinHeight = Sim.GetUnitHeight(ECrowdKind::Assassin, WizardHeight);
	// Skeletons, Guards and grunts are models when theirs is there, and their simple shape when it is not.
	TBitArray<> Modeled;
	// The dead first: they take the facing of the figures that are about to be given up.
	RefreshCorpses(Sim, GroundZ, WizardHeight);
	RefreshArmyModels(Sim, GroundZ, WizardHeight, WizardPosition, Modeled);
	const TArray<FCrowdUnit>& AllUnits = Sim.GetUnits();
	for (int32 UnitIndex = 0; UnitIndex < AllUnits.Num(); ++UnitIndex)
	{
		const FCrowdUnit& Unit = AllUnits[UnitIndex];
		if (Sim.IsSweeping(Unit))
		{
			// The reach of its sweep: a slice of a circle across its front that grows out to the sweep's reach.
			const double Reach = Sim.GetSlamRadius(Unit) * Sim.GetSlamShare(Unit);
			const double Middle = FMath::Atan2(Unit.Facing.Y, Unit.Facing.X);
			SweepFanTransforms.Emplace(FRotator(0.0, FMath::RadiansToDegrees(Middle), 0.0).Quaternion(),
				FVector(Unit.Position.X, Unit.Position.Y, GroundZ + SlamRingHeight), FVector(Reach / 50.0, Reach / 50.0, 1.0));
		}
		else if (Sim.IsSlamming(Unit))
		{
			// The circle of its slam: it grows from tiny to the size of the area, then shows bright until the slam lands.
			const double Share = Sim.GetSlamShare(Unit);
			const double Across = Sim.GetSlamRadius(Unit) * Share / 50.0;
			const bool bFull = Share >= 1.0;
			(bFull ? SlamFullTransforms : SlamRingTransforms).Emplace(FQuat::Identity,
				FVector(Unit.SlamCenter.X, Unit.SlamCenter.Y, GroundZ + (bFull ? SlamFullHeight : SlamRingHeight)), FVector(Across, Across, BlastThickness));
		}
		if (Sim.IsWindingThrow(Unit) && !Unit.ThrowAim.IsNearlyZero())
		{
			// The lane of his spear: it grows from him the way the spear will go, as wide as the spear's reach to
			// either side, and shows bright for the last moment, when it no longer follows the wizard.
			const bool bLocked = Sim.IsThrowLocked(Unit);
			const double Reach = Sim.GetThrowReach(Unit) * (bLocked ? 1.0 : FMath::Lerp(ThrowLaneStartShare, 1.0, static_cast<double>(Sim.GetThrowShare(Unit))));
			const FVector2D Middle = Unit.Position + Unit.ThrowAim * (Reach * 0.5);
			const FQuat Turn = FRotator(0.0, FMath::RadiansToDegrees(FMath::Atan2(Unit.ThrowAim.Y, Unit.ThrowAim.X)), 0.0).Quaternion();
			(bLocked ? ThrowLockTransforms : ThrowLaneTransforms).Emplace(Turn,
				FVector(Middle.X, Middle.Y, GroundZ + (bLocked ? SlamFullHeight : SlamRingHeight)),
				FVector(Reach / 100.0, Sim.Tuning.CommanderSpearRadius * 2.0 / 100.0, ThrowLaneThickness));
		}
		if (Modeled[UnitIndex])
		{
			continue;
		}
		if (Unit.Kind == ECrowdKind::Commander)
		{
			Stand(CommanderTransforms, Unit, CommanderHeight, CommanderWidth);
		}
		else if (Unit.Kind == ECrowdKind::Brute)
		{
			// The box turns with the brute.
			const FVector2D Facing = Unit.Facing.IsNearlyZero() ? FVector2D(1.0, 0.0) : Unit.Facing;
			const FQuat Turn = FRotator(0.0, FMath::RadiansToDegrees(FMath::Atan2(Facing.Y, Facing.X)), 0.0).Quaternion();
			const double Across = BruteHeight * BruteWidth;
			BruteTransforms.Emplace(Turn, FVector(Unit.Position.X, Unit.Position.Y, GroundZ + BruteHeight * 0.5),
				FVector(Across / 100.0, Across / 100.0, BruteHeight / 100.0));
		}
		else if (Unit.Kind == ECrowdKind::Guard)
		{
			Stand(GuardTransforms, Unit, GuardHeight, GuardWidth);
		}
		else if (Unit.Kind == ECrowdKind::Skeleton || Unit.Kind == ECrowdKind::Marksman)
		{
			// A Marksman without its model stands as a skeleton's shape.
			Stand(SkeletonTransforms, Unit, SkeletonHeight, SkeletonWidth);
		}
		else if (Unit.Kind == ECrowdKind::Archer)
		{
			Stand(ArcherTransforms, Unit, ArcherHeight, ArcherWidth);
		}
		else if (Unit.Kind == ECrowdKind::Assassin)
		{
			// Cloaked until it is close: not drawn at all beyond its reveal range, then it grows into view.
			const double Shown = Sim.GetShown(Unit, WizardPosition);
			if (Shown > 0.0)
			{
				Stand(AssassinTransforms, Unit, AssassinHeight * FMath::Lerp(AssassinStartShare, 1.0, Shown), AssassinWidth);
			}
		}
		else
		{
			Stand(GruntTransforms, Unit, GruntHeight, GruntWidth);
		}
	}
	for (const FCrowdSlash& Slash : Sim.GetSlashes())
	{
		// A tilted bar through where the skeleton stood, longer and thinner as it fades.
		const double Alpha = FMath::Clamp(Slash.Age / FMath::Max(Sim.Tuning.SlashSeconds, 0.01f), 0.0f, 1.0f);
		SlashTransforms.Emplace(FRotator(0.0, 35.0, 50.0).Quaternion(), FVector(Slash.Position.X, Slash.Position.Y, GroundZ + SkeletonHeight * 0.55),
			FVector(0.05 * (1.0 - Alpha) + 0.01, SkeletonHeight * FMath::Lerp(0.9, 1.6, Alpha) / 100.0, 0.05 * (1.0 - Alpha) + 0.01));
	}
	for (const FCrowdBlast& Hit : Sim.GetSlamHits())
	{
		// Where a slam landed: a hot disc that spreads a little and is gone.
		const double Alpha = FMath::Clamp(Hit.Age / FMath::Max(Sim.Tuning.SlamFlashSeconds, 0.01f), 0.0f, 1.0f);
		const double Across = (Hit.Radius > 0.0f ? Hit.Radius : Sim.Tuning.BruteSlamRadius) * (1.0 + SlamFlashSpread * Alpha) / 50.0;
		SlamFlashTransforms.Emplace(FQuat::Identity, FVector(Hit.Position.X, Hit.Position.Y, GroundZ + SlamFlashHeight), FVector(Across, Across, BlastThickness));
	}
	for (const FCrowdBolt& Bolt : Sim.GetBolts())
	{
		// Its long axis (the sphere's Z, the hot end first) lies along its flight.
		const FVector2D Flight = Bolt.Velocity.GetSafeNormal();
		const FVector Along = Flight.IsNearlyZero() ? FVector::ForwardVector : FVector(Flight.X, Flight.Y, 0.0);
		BoltTransforms.Emplace(FRotationMatrix::MakeFromZ(Along).ToQuat(), FVector(Bolt.Position.X, Bolt.Position.Y, GroundZ + BoltHeight), BoltScale);
	}

	Apply(Skeletons, SkeletonTransforms);
	Apply(Grunts, GruntTransforms);
	for (const FCrowdBone& Bone : Sim.GetBones())
	{
		// The turn comes from where the pile lies, so it never changes while the pile is there.
		const double Yaw = FMath::Fmod(Bone.Position.X * 0.37 + Bone.Position.Y * 0.61, 180.0);
		const bool bBig = Bone.bGuard || Bone.Value >= 3;
		if (bBoneModel)
		{
			// The bone pile stands on the ground; a brute's or a Guard's is a bigger pile.
			BoneTransforms.Emplace(FRotator(0.0, Yaw * 2.0, 0.0).Quaternion(), FVector(Bone.Position.X, Bone.Position.Y, GroundZ), FVector(bBig ? BonePileBigScale : 1.0));
		}
		else
		{
			BoneTransforms.Emplace(FRotator(0.0, Yaw, 0.0).Quaternion(), FVector(Bone.Position.X, Bone.Position.Y, GroundZ + BoneHeight), bBig ? BruteBoneScale : BoneScale);
		}
	}
	for (const FCrowdSpear& Spear : Sim.GetSpears())
	{
		// The shaft lies along its flight: the cylinder's long axis is Z.
		const FVector2D Flight = (Spear.Target - Spear.Position).GetSafeNormal();
		const FVector Along = Flight.IsNearlyZero() ? FVector::ForwardVector : FVector(Flight.X, Flight.Y, 0.0);
		SpearTransforms.Emplace(FRotationMatrix::MakeFromZ(Along).ToQuat(), FVector(Spear.Position.X, Spear.Position.Y, GroundZ + SpearHeight), bSpearModel ? FVector::OneVector : SpearScale);
	}
	for (const FCrowdBlast& Blast : Sim.GetBlasts())
	{
		// Spreads fast, then slows: eased out over the blast's time.
		const double Alpha = FMath::Clamp(Blast.Age / FMath::Max(Sim.Tuning.BlastSeconds, 0.01f), 0.0f, 1.0f);
		const double Share = FMath::Lerp(BlastStartShare, 1.0, 1.0 - FMath::Square(1.0 - Alpha));
		const double Across = Sim.Tuning.SpearRadius * Share / 50.0;
		BlastTransforms.Emplace(FQuat::Identity, FVector(Blast.Position.X, Blast.Position.Y, GroundZ + BlastHeight), FVector(Across, Across, BlastThickness));
	}

	for (const FCrowdArrow& Arrow : Sim.GetArrows())
	{
		// The shaft lies along its flight: the cylinder's long axis is Z.
		// It flies in an arc: up from the bow, over the heads between, and down on its mark.
		const FVector2D Flight = Arrow.Velocity.GetSafeNormal();
		const FVector Along = Flight.IsNearlyZero() ? FVector::ForwardVector : FVector(Flight.X, Flight.Y, Sim.GetArrowSlope(Arrow)).GetSafeNormal();
		// A Marksman's arrow and an archer's are different models. A model is its own size; the plain shaft is scaled.
		const bool bModel = Arrow.bArmy ? bArmyArrowModel : bArrowModel;
		(Arrow.bArmy ? ArmyArrowTransforms : ArrowTransforms).Emplace(FRotationMatrix::MakeFromZ(Along).ToQuat(),
			FVector(Arrow.Position.X, Arrow.Position.Y, GroundZ + ArrowHeight + Sim.GetArrowHeight(Arrow)), bModel ? FVector::OneVector : ArrowScale);
	}
	for (const FCrowdJavelin& Javelin : Sim.GetJavelins())
	{
		// The commander's spear: a long, thick shaft, flat and straight at the wizard.
		const FVector2D Flight = Javelin.Velocity.GetSafeNormal();
		const FVector Along = Flight.IsNearlyZero() ? FVector::ForwardVector : FVector(Flight.X, Flight.Y, 0.0);
		// His own spear model, its own size; without it, the plain shaft, made long and thick.
		JavelinTransforms.Emplace(FRotationMatrix::MakeFromZ(Along).ToQuat(), FVector(Javelin.Position.X, Javelin.Position.Y, GroundZ + JavelinHeight),
			bJavelinModel ? FVector::OneVector : ArrowScale * JavelinScale);
	}

	Apply(Archers, ArcherTransforms);
	Apply(Assassins, AssassinTransforms);
	Apply(Arrows, ArrowTransforms);
	Apply(ArmyArrows, ArmyArrowTransforms);
	Apply(Javelins, JavelinTransforms);
	Apply(Bolts, BoltTransforms);
	Apply(Bones, BoneTransforms);
	Apply(Brutes, BruteTransforms);
	Apply(Guards, GuardTransforms);
	Apply(Commanders, CommanderTransforms);
	Apply(Spears, SpearTransforms);
	Apply(Blasts, BlastTransforms);
	Apply(Slashes, SlashTransforms);
	Apply(BruteBacks, BruteBackTransforms);
	Apply(SlamRings, SlamRingTransforms);
	Apply(SlamFulls, SlamFullTransforms);
	Apply(ThrowLanes, ThrowLaneTransforms);
	if (SweepFanMaterial)
	{
		SweepFanMaterial->SetScalarParameterValue(TEXT("Arc"), FMath::Clamp(Sim.Tuning.BruteSweepArc / 360.0f, 0.02f, 1.0f));
	}
	Apply(SweepFans, SweepFanTransforms);
	Apply(ThrowLocks, ThrowLockTransforms);
	Apply(SlamFlashes, SlamFlashTransforms);
}

void ACrowdRenderer::SetObstacles(const TArray<FCrowdObstacle>& Obstacles)
{
	WallBoxes.Reset();
	for (const FCrowdObstacle& Obstacle : Obstacles)
	{
		WallBoxes.Emplace(Obstacle.Center.X, Obstacle.Center.Y, Obstacle.HalfSize.X, Obstacle.HalfSize.Y);
	}
	bWallsChanged = true;
}

void ACrowdRenderer::Apply(UInstancedStaticMeshComponent* Component, const TArray<FTransform>& Transforms)
{
	if (Component->GetInstanceCount() != Transforms.Num())
	{
		Component->ClearInstances();
		if (Transforms.Num() > 0)
		{
			Component->AddInstances(Transforms, false, true, false);
		}
	}
	else if (Transforms.Num() > 0)
	{
		Component->BatchUpdateInstancesTransforms(0, Transforms, true, true, true);
	}
}
