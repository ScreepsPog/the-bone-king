#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Engine/World.h"
#include "Game/CrowdRenderer.h"
#include "Game/CrowdSubsystem.h"
#include "Game/WizardPawn.h"
#include "Sim/CrowdSim.h"
#include "Tests/WizTestFlags.h"

#include "Game/CrowdRenderer.h"
#include "Game/LevelPieceActor.h"
#include "Engine/Level.h"

#if WITH_DEV_AUTOMATION_TESTS

// Direction of 2026-11-13: the wizard is the Wizard King model on the UE5 mannequin skeleton, moved by the mannequin's
// default animation blueprint. Since he became a Character the blueprint reads his movement itself, so the test of
// the step angle this file once held went with the code it tested.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardModelScale, "WebTesting.Model.HeightSetsTheScale", WIZ_TEST_FLAGS)
bool FWizardModelScale::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("a 220 tall model shown 220 tall is not scaled"), AWizardPawn::GetModelScale(220.0f, 220.0f), 1.0f, 0.001f);
	TestEqual(TEXT("a 220 tall model shown 330 tall is 1.5 times"), AWizardPawn::GetModelScale(330.0f, 220.0f), 1.5f, 0.001f);
	TestEqual(TEXT("the height is held to the allowed range"), AWizardPawn::GetModelScale(5000.0f, 220.0f), AWizardPawn::ModelHeightMax / 220.0f, 0.001f);
	TestEqual(TEXT("a model with no height is not scaled"), AWizardPawn::GetModelScale(220.0f, 0.0f), 1.0f, 0.001f);
	return true;
}

// Direction of 2026-11-13 (15:10 PST): the army's plain skeleton is a model on the mannequin skeleton, imported at the
// size agreed for it beside the wizard. Every character model that is imported has to keep to that scale.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArmyModelSize, "WebTesting.Model.TheSkeletonModelIsTheAgreedSize", WIZ_TEST_FLAGS)
bool FArmyModelSize::RunTest(const FString& Parameters)
{
	const USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Army/SKM_Skeleton.SKM_Skeleton"));
	TestNotNull(TEXT("the skeleton model is in the project"), Mesh);
	if (!Mesh)
	{
		return false;
	}
	const FCrowdSim Sim;
	const float Wanted = Sim.GetUnitHeight(ECrowdKind::Skeleton, AWizardPawn::ModelHeightDefault);
	TestEqual(TEXT("it is as tall as a skeleton stands beside the wizard (150 of his 220)"), static_cast<float>(Mesh->GetBounds().BoxExtent.Z * 2.0), Wanted, 2.0f);
	TestNotNull(TEXT("it has a skeleton"), Mesh->GetSkeleton());
	if (Mesh->GetSkeleton())
	{
		TestEqual(TEXT("it is on the mannequin's skeleton, so the mannequin's animations play on it"), Mesh->GetSkeleton()->GetName(), FString(TEXT("SK_Mannequin")));
		TestTrue(TEXT("with the foot helper bones the animation blueprint needs"), Mesh->GetRefSkeleton().FindBoneIndex(TEXT("ik_foot_l")) != INDEX_NONE);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNewModelSizes, "WebTesting.Model.TheGuardAndGruntModelsAreTheAgreedSize", WIZ_TEST_FLAGS)
bool FNewModelSizes::RunTest(const FString& Parameters)
{
	const FCrowdSim Sim;
	const TPair<const TCHAR*, ECrowdKind> Models[] = {
		{ TEXT("/Game/Army/SKM_Guard.SKM_Guard"), ECrowdKind::Guard },
		{ TEXT("/Game/Enemy/SKM_Grunt.SKM_Grunt"), ECrowdKind::Grunt },
		{ TEXT("/Game/Enemy/SKM_Assassin.SKM_Assassin"), ECrowdKind::Assassin },
		{ TEXT("/Game/Enemy/SKM_Archer.SKM_Archer"), ECrowdKind::Archer },
		{ TEXT("/Game/Enemy/SKM_Brute.SKM_Brute"), ECrowdKind::Brute },
	};
	for (const TPair<const TCHAR*, ECrowdKind>& Model : Models)
	{
		const USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, Model.Key);
		TestNotNull(FString::Printf(TEXT("%s is in the project"), Model.Key), Mesh);
		if (!Mesh)
		{
			continue;
		}
		TestEqual(TEXT("it is as tall as its kind stands beside the wizard (Guard 175, grunt 170, assassin and archer 165, brute 240)"),
			static_cast<float>(Mesh->GetBounds().BoxExtent.Z * 2.0), Sim.GetUnitHeight(Model.Value, AWizardPawn::ModelHeightDefault), 2.0f);
		TestTrue(TEXT("it is on the mannequin's skeleton"), Mesh->GetSkeleton() && Mesh->GetSkeleton()->GetName() == TEXT("SK_Mannequin"));
		TestTrue(TEXT("with the foot helper bones"), Mesh->GetRefSkeleton().FindBoneIndex(TEXT("ik_foot_l")) != INDEX_NONE);
	}
	return true;
}

// Direction of 2026-11-13 (17:21 PST): the swing is lined up so the blow's damage lands as the swing connects.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSwingTiming, "WebTesting.Model.TheSwingConnectsWhenTheBlowLands", WIZ_TEST_FLAGS)
bool FSwingTiming::RunTest(const FString& Parameters)
{
	const float Interval = 1.0f;
	const float HitShare = 0.45f;
	const int32 Swings = 6;
	// Any unit, whenever its blow is due and whatever the clock says: at the moment of its blow, the swing it was
	// given is within a twelfth of an interval of the point where the fist connects.
	float Worst = 0.0f;
	for (const double Clock : { 0.0, 0.37, 12.81, 100.02 })
	{
		for (float TimeToBlow = 0.0f; TimeToBlow < 1.0f; TimeToBlow += 0.07f)
		{
			const int32 Swing = ACrowdRenderer::PickSwing(Clock, TimeToBlow, Interval, HitShare, Swings);
			TestTrue(TEXT("a swing that exists"), Swing >= 0 && Swing < Swings);
			const float AtBlow = ACrowdRenderer::GetSwingShare(Clock + TimeToBlow, Interval, Swing, Swings);
			float Off = FMath::Abs(AtBlow - HitShare);
			Off = FMath::Min(Off, 1.0f - Off);
			Worst = FMath::Max(Worst, Off);
		}
	}
	TestTrue(TEXT("the swing connects within a twelfth of a second of the blow"), Worst <= 1.0f / 12.0f + 0.001f);
	// The same unit keeps the same swing through its fight: later on the clock, with the blow that much nearer.
	TestEqual(TEXT("the choice does not change as time passes"), ACrowdRenderer::PickSwing(5.3, 0.6f, Interval, HitShare, Swings),
		ACrowdRenderer::PickSwing(5.5, 0.4f, Interval, HitShare, Swings));
	return true;
}

// Direction of 2026-11-13 (21:53 PST): the houses and the shrine are imported so a doorway is about 230 units tall beside
// the 220 tall wizard, with the doorstep at height 0 and the slab each stands on under the ground.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBuildingScale, "WebTesting.Model.TheBuildingsAreAtDoorScale", WIZ_TEST_FLAGS)
bool FBuildingScale::RunTest(const FString& Parameters)
{
	// Each building's height in its file's own units and its doorway's, measured on straight-on renders. The shrine is
	// a ruin with no doorway: it is sized so its window sill is at chest height, which is 0.2556 of its file for 230.
	struct FBuilding
	{
		const TCHAR* Path;
		double FileHeight;
		double FileDoor;
	};
	const FBuilding Buildings[] = {
		{ TEXT("/Game/Buildings/SM_Peasant_Cottage.SM_Peasant_Cottage"), 0.650, 0.2034 },
		{ TEXT("/Game/Buildings/SM_Burned_Cottage.SM_Burned_Cottage"), 0.651, 0.187 },
		{ TEXT("/Game/Buildings/SM_Barn.SM_Barn"), 0.464, 0.150 },
		{ TEXT("/Game/Buildings/SM_Twostory_House.SM_Twostory_House"), 0.828, 0.1753 },
		{ TEXT("/Game/Buildings/SM_Shrine.SM_Shrine"), 0.779, 0.2556 },
	};
	for (const FBuilding& Building : Buildings)
	{
		const UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, Building.Path);
		TestNotNull(FString::Printf(TEXT("%s is in the project"), Building.Path), Mesh);
		if (!Mesh)
		{
			continue;
		}
		const FBox Box = Mesh->GetBoundingBox();
		const double Door = (Box.Max.Z - Box.Min.Z) * Building.FileDoor / Building.FileHeight;
		TestTrue(FString::Printf(TEXT("%s: the doorway is about 230 tall (it is %.0f)"), Building.Path, Door), Door >= 215.0 && Door <= 245.0);
		TestTrue(FString::Printf(TEXT("%s: the doorstep is at the ground and the slab under it (bottom at %.0f)"), Building.Path, Box.Min.Z), Box.Min.Z <= -20.0 && Box.Min.Z >= -80.0);
		TestTrue(TEXT("it has its material"), Mesh->GetMaterial(0) && Mesh->GetMaterial(0)->GetName().StartsWith(TEXT("M_")));
	}
	return true;
}

// Direction of 2026-11-13 (22:50 PST): the Outskirts is a level the director edits, and a building placed in it blocks.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlacedBuildingBlocks, "WebTesting.Level.APlacedBuildingBlocksItsFootprint", WIZ_TEST_FLAGS)
bool FPlacedBuildingBlocks::RunTest(const FString& Parameters)
{
	const FCrowdObstacle Obstacle = UCrowdSubsystem::MakeBuildingObstacle(FBox(FVector(1000.0, -400.0, -50.0), FVector(2000.0, 400.0, 700.0)));
	TestEqual(TEXT("the box is centered on the building"), Obstacle.Center, FVector2D(1500.0, 0.0));
	TestEqual(TEXT("it is the footprint drawn in, so eaves and the slab's edge do not block"), Obstacle.HalfSize, FVector2D(500.0, 400.0) * UCrowdSubsystem::BuildingBodyShare);
	TestFalse(TEXT("a building is a wall, not water: Bolts stop at it"), Obstacle.bLow);
	TestTrue(TEXT("a unit in its middle is inside"), Obstacle.Contains(FVector2D(1500.0, 0.0), 0.0));
	TestFalse(TEXT("a unit under the eaves is not"), Obstacle.Contains(FVector2D(1980.0, 0.0), 0.0));

	const UWorld* Map = LoadObject<UWorld>(nullptr, TEXT("/Game/Maps/Outskirts.Outskirts"));
	TestNotNull(TEXT("the Outskirts is a level in the project"), Map);

	// Direction of 2026-11-14 (01:05 PST): the layout's pieces are actors in the level; the ground is a landscape.
	int32 PieceActors = 0;
	int32 BlockingPieces = 0;
	int32 Landscapes = 0;
	bool bUsesPlaced = false;
	if (Map && Map->PersistentLevel)
	{
		for (const AActor* Actor : Map->PersistentLevel->Actors)
		{
			if (const ALevelPieceActor* Piece = Cast<ALevelPieceActor>(Actor))
			{
				++PieceActors;
				BlockingPieces += FLevelLayout::Blocks(Piece->Kind) ? 1 : 0;
				const FLevelPiece Made = Piece->MakePiece();
				TestTrue(TEXT("a piece is read where its actor stands"), Made.Center.Equals(FVector2D(Piece->GetActorLocation().X, Piece->GetActorLocation().Y), 0.01));
			}
			else if (const ACrowdRenderer* Layout = Cast<ACrowdRenderer>(Actor))
			{
				bUsesPlaced = Layout->UsesPlacedPieces();
			}
			else if (Actor && Actor->GetClass()->GetName() == TEXT("Landscape"))
			{
				++Landscapes;
			}
		}
	}
	// Changed 2026-11-14 17:40 PST: the director replaced the tree pieces with trees from a Fab pack, so the level
	// holds 79 pieces (it held over 120) and 43 that block (it held over 100): the river and three trees.
	// Changed again 2026-11-14 18:45 PST: the director had the 33 road blocks removed (the roads are painted on the
	// ground), which leaves 46 pieces.
	TestTrue(TEXT("the level holds the layout's pieces as actors"), PieceActors >= 40);
	TestTrue(TEXT("the river among them"), BlockingPieces >= 40);
	TestTrue(TEXT("the layout actor uses the placed pieces"), bUsesPlaced);
	TestEqual(TEXT("the ground is one landscape"), Landscapes, 1);
	return true;
}

// Found 2026-11-13 (18:40 PST): with the engine's skin cache on, every model on screen made a new buffer and bind group
// each frame in the browser, and the browser freeing them in bulk dropped frames about every second. It stays off.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSkinCacheOff, "WebTesting.Model.TheSkinCacheIsOff", WIZ_TEST_FLAGS)
bool FSkinCacheOff::RunTest(const FString& Parameters)
{
	const IConsoleVariable* Compile = IConsoleManager::Get().FindConsoleVariable(TEXT("r.SkinCache.CompileShaders"));
	TestNotNull(TEXT("the engine has the skin cache switch"), Compile);
	TestEqual(TEXT("skin cache shaders are not compiled"), Compile ? Compile->GetInt() : -1, 0);
	const IConsoleVariable* Mode = IConsoleManager::Get().FindConsoleVariable(TEXT("r.SkinCache.Mode"));
	TestEqual(TEXT("the skin cache is not used"), Mode ? Mode->GetInt() : 0, 0);
	return true;
}

#endif
