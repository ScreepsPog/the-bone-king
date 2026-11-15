#include "Animation/AnimMontage.h"
#include "Engine/Level.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Game/CrowdRenderer.h"
#include "Game/CrowdSubsystem.h"
#include "Game/LevelPieceActor.h"
#include "Engine/Texture2D.h"
#include "Game/LevelActors.h"
#include "Game/WizardAnimInstance.h"
#include "Game/WizardHUD.h"
#include "Sim/BoltCast.h"
#include "Tests/CrowdTestHelpers.h"
#include "Tests/WizTestFlags.h"

#if WITH_DEV_AUTOMATION_TESTS

// Direction of 2026-11-14 17:20 PST:
// - "Have the boss predict sometimes with his long straight attack. This doesn't mean they do it every time."
// - "For brutes, remove their weakpoint indicator."
// - "Make the mouse a normal cursor when on any menu and back to the crosshair when in game."
// - Attack animations have a notify named "Fire" "when the attack should start. For example, if fire is called, do
//   damage with a sword or start the arrow shot."
// - "For Anim_Wizard_Bolt ... only the upper body does the animation while the default running, walking, and idle
//   still control the lower body."
// - Bows on Bow_Socket; "make the walls viewable in the editor"; the river's water is one smooth, animated mesh.

namespace AttackTest
{
	const FCrowdUnit* FirstOf(const FCrowdSim& Sim, ECrowdKind Kind)
	{
		for (const FCrowdUnit& Unit : Sim.GetUnits())
		{
			if (Unit.Kind == Kind)
			{
				return &Unit;
			}
		}
		return nullptr;
	}

	/** The wizard walks straight along +Y from the origin; the commander stands at 800 along X. Counts spears that hit him. */
	int32 WalkPast(FCrowdSim& Sim, float Seconds, bool& bOutAimedAhead, bool& bOutAimedAtHim)
	{
		CrowdTest::MakeEnemiesPlain(Sim);
		Sim.Tuning.CommanderSpeed = 0.0f;
		Sim.Tuning.CommanderSpearRange = 5000.0f;
		Sim.AddEnemy(ECrowdKind::Commander, FVector2D(800.0, 0.0));
		FVector2D Wizard = CrowdTest::Origin;
		int32 Hits = 0;
		bOutAimedAhead = false;
		bOutAimedAtHim = false;
		for (float Time = 0.0f; Time < Seconds; Time += 0.05f)
		{
			Wizard.Y += 300.0 * 0.05;
			Hits += Sim.Step(0.05f, Wizard).JavelinsHitWizard;
			const FCrowdUnit* Commander = FirstOf(Sim, ECrowdKind::Commander);
			if (Commander && Sim.IsWindingThrow(*Commander) && !Sim.IsThrowLocked(*Commander))
			{
				const FVector2D ToWizard = (Wizard - Commander->Position).GetSafeNormal();
				// Ahead of him is further along +Y, the way he walks.
				bOutAimedAhead = bOutAimedAhead || Commander->ThrowAim.Y > ToWizard.Y + 0.05;
				bOutAimedAtHim = bOutAimedAtHim || FMath::IsNearlyEqual(Commander->ThrowAim.Y, ToWizard.Y, 0.01);
			}
		}
		return Hits;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommanderLeads, "WebTesting.Commander.SometimesThrowsWhereTheWizardWillBe", WIZ_TEST_FLAGS)
bool FCommanderLeads::RunTest(const FString& Parameters)
{
	const FCrowdTuning Shipped;
	TestTrue(TEXT("he predicts sometimes, not every time"), Shipped.CommanderSpearLeadChance > 0.0f && Shipped.CommanderSpearLeadChance < 1.0f);

	bool bAhead = false;
	bool bAtHim = false;
	FCrowdSim Always;
	Always.Tuning.CommanderSpearLeadChance = 1.0f;
	const int32 LedHits = AttackTest::WalkPast(Always, 2.5f, bAhead, bAtHim);
	TestTrue(TEXT("a throw that predicts is aimed ahead of the wizard, the way he walks"), bAhead);
	TestTrue(TEXT("and catches him if he keeps walking straight"), LedHits >= 1);

	FCrowdSim Never;
	Never.Tuning.CommanderSpearLeadChance = 0.0f;
	const int32 PlainHits = AttackTest::WalkPast(Never, 2.5f, bAhead, bAtHim);
	TestTrue(TEXT("a throw that does not predict is aimed at where he is"), bAtHim && !bAhead);
	TestEqual(TEXT("and walking on gets him out of its way"), PlainHits, 0);

	// With the chance as shipped, some throws predict and some do not.
	FCrowdSim Mixed;
	CrowdTest::MakeEnemiesPlain(Mixed);
	Mixed.Tuning.CommanderSpeed = 0.0f;
	Mixed.Tuning.CommanderSpearInterval = 0.2f;
	Mixed.AddEnemy(ECrowdKind::Commander, FVector2D(800.0, 0.0));
	int32 Led = 0;
	int32 Plain = 0;
	bool bWasWinding = false;
	for (int32 Step = 0; Step < 600; ++Step)
	{
		Mixed.Step(0.05f, CrowdTest::Origin);
		const FCrowdUnit* Commander = AttackTest::FirstOf(Mixed, ECrowdKind::Commander);
		const bool bWinding = Commander && Mixed.IsWindingThrow(*Commander);
		if (bWinding && !bWasWinding)
		{
			++(Commander->bThrowLeads ? Led : Plain);
		}
		bWasWinding = bWinding;
	}
	TestTrue(TEXT("over many throws some predict"), Led >= 2);
	TestTrue(TEXT("and some do not"), Plain >= 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrikeTimes, "WebTesting.Attack.TheSimulationSaysWhenABlowLands", WIZ_TEST_FLAGS)
bool FStrikeTimes::RunTest(const FString& Parameters)
{
	// A brute's slam: the blow is ahead of it while the circle grows, and behind it while it rests.
	FCrowdSim Sim;
	CrowdTest::MakeEnemiesPlain(Sim);
	Sim.Tuning.BruteSpeed = 0.0f;
	Sim.AddEnemy(ECrowdKind::Brute, FVector2D(150.0, 0.0));
	bool bBefore = false;
	bool bAfter = false;
	float Last = 1000.0f;
	bool bFalls = true;
	for (int32 Step = 0; Step < 120 && !bAfter; ++Step)
	{
		const int32 Landed = Sim.Step(0.05f, CrowdTest::Origin).SlamsLanded;
		const FCrowdUnit* Brute = AttackTest::FirstOf(Sim, ECrowdKind::Brute);
		float TimeToBlow = 0.0f;
		if (Brute && Sim.GetStrike(*Brute, TimeToBlow))
		{
			if (TimeToBlow > 0.0f)
			{
				bBefore = true;
				bFalls = bFalls && TimeToBlow < Last;
				Last = TimeToBlow;
				TestTrue(TEXT("never further off than the whole wind-up"), TimeToBlow <= Sim.Tuning.BruteSlamGrow + Sim.Tuning.BruteSlamHold + 0.001f);
			}
			else if (Landed > 0 || TimeToBlow < 0.0f)
			{
				bAfter = true;
			}
		}
	}
	TestTrue(TEXT("the slam's blow is counted down to"), bBefore && bFalls);
	TestTrue(TEXT("and after it lands the brute follows through"), bAfter);

	// An archer with someone to shoot at: its next arrow is its blow.
	FCrowdSim Range;
	CrowdTest::MakeEnemiesPlain(Range);
	Range.Tuning.ArcherSpeed = 0.0f;
	Range.AddEnemy(ECrowdKind::Archer, FVector2D(500.0, 0.0));
	int32 Arrows = 0;
	bool bDrawing = false;
	for (int32 Step = 0; Step < 80; ++Step)
	{
		Arrows += Range.Step(0.05f, CrowdTest::Origin).ArrowsFired;
		const FCrowdUnit* Archer = AttackTest::FirstOf(Range, ECrowdKind::Archer);
		float TimeToBlow = 0.0f;
		if (Archer && Arrows > 0 && Range.GetStrike(*Archer, TimeToBlow))
		{
			bDrawing = true;
			TestEqual(TEXT("the time to its next arrow"), TimeToBlow, Archer->AttackTimer);
		}
	}
	TestTrue(TEXT("the archer shot"), Arrows >= 1);
	TestTrue(TEXT("and is drawing for the next arrow"), bDrawing);

	// A Marksman with nothing to shoot at stands.
	FCrowdSim Idle;
	Idle.AddSkeleton(FVector2D(100.0, 0.0), ECrowdKind::Marksman);
	for (int32 Step = 0; Step < 80; ++Step)
	{
		Idle.Step(0.05f, CrowdTest::Origin);
	}
	const FCrowdUnit* Marksman = AttackTest::FirstOf(Idle, ECrowdKind::Marksman);
	float Unused = 0.0f;
	TestTrue(TEXT("a Marksman with no enemy in range is not shooting"), Marksman && !Idle.GetStrike(*Marksman, Unused));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFireNotifies, "WebTesting.Attack.TheAnimationsFireWhenTheBlowLands", WIZ_TEST_FLAGS)
bool FFireNotifies::RunTest(const FString& Parameters)
{
	const TCHAR* Paths[] = {
		TEXT("/Game/Animations/Archer/Anim_Archer_Shoot.Anim_Archer_Shoot"),
		TEXT("/Game/Animations/Brute/Anim_Brute_Hammer_Slam.Anim_Brute_Hammer_Slam"),
		TEXT("/Game/Animations/Wizard/Anim_Wizard_Bolt.Anim_Wizard_Bolt"),
	};
	for (const TCHAR* Path : Paths)
	{
		const UAnimMontage* Montage = LoadObject<UAnimMontage>(nullptr, Path);
		if (!TestNotNull(Path, Montage))
		{
			continue;
		}
		const float Share = ACrowdRenderer::GetFireShare(Montage, -1.0f);
		AddInfo(FString::Printf(TEXT("%s: Fire at %.2f of %.2f s"), Path, Share * Montage->GetPlayLength(), Montage->GetPlayLength()));
		TestTrue(TEXT("it has a Fire notify, inside the animation"), Share > 0.0f && Share < 1.0f);
	}
	TestEqual(TEXT("an animation with no Fire notify gives the fallback"), ACrowdRenderer::GetFireShare(nullptr, 0.45f), 0.45f);

	// One turn of the animation is stretched so its Fire moment meets the blow.
	const FCrowdTuning Tuning;
	TestEqual(TEXT("an archer's is the time between its arrows"), ACrowdRenderer::GetStrikeInterval(Tuning, ECrowdKind::Archer, 0.6f), Tuning.ArcherInterval);
	TestEqual(TEXT("a Marksman's too"), ACrowdRenderer::GetStrikeInterval(Tuning, ECrowdKind::Marksman, 0.6f), Tuning.MarksmanInterval);
	const float Slam = ACrowdRenderer::GetStrikeInterval(Tuning, ECrowdKind::Brute, 0.5f);
	TestEqual(TEXT("a slam's starts with the circle: its Fire share of the turn is the wind-up"), Slam * 0.5f, Tuning.BruteSlamGrow + Tuning.BruteSlamHold, 0.001f);
	// The figure picked for a brute at the start of its slam is at the start of the animation.
	const float WindUp = Tuning.BruteSlamGrow + Tuning.BruteSlamHold;
	const int32 Picked = ACrowdRenderer::PickSwing(12.3, WindUp, Slam, 0.5f, 6);
	const float Share = ACrowdRenderer::GetSwingShare(12.3, Slam, Picked, 6);
	TestTrue(TEXT("within a twelfth of a turn of the start"), Share < 1.0f / 12.0f + 0.001f || Share > 1.0f - 1.0f / 12.0f - 0.001f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBoltCastTimes, "WebTesting.Bolt.LeavesAtTheAnimationsFireMoment", WIZ_TEST_FLAGS)
bool FBoltCastTimes::RunTest(const FString& Parameters)
{
	const float Interval = 0.45f;
	const float FireTime = 0.8f;
	const float Length = 2.0f;

	// No animation: at once, and every Interval while held, as before.
	FBoltCast Plain;
	TestTrue(TEXT("with no animation the Bolt leaves at once"), Plain.Step(0.01f, true, true, Interval, 0.2f, 0.0f, 0.0f));
	TestTrue(TEXT("and nothing plays"), Plain.AnimTime < 0.0f);
	int32 Bolts = 0;
	for (int32 Step = 0; Step < 100; ++Step)
	{
		Bolts += Plain.Step(0.01f, true, true, Interval, 0.2f, 0.0f, 0.0f) ? 1 : 0;
	}
	TestEqual(TEXT("then one every 0.45 seconds: two more in the next second"), Bolts, 2);

	// With the animation: the Bolt leaves 0.2 s after the cast starts, with the animation at its Fire moment.
	FBoltCast Cast;
	TestFalse(TEXT("the cast starts and no Bolt leaves yet"), Cast.Step(0.01f, true, true, Interval, 0.2f, FireTime, Length));
	TestEqual(TEXT("the animation starts"), Cast.AnimTime, 0.0f);
	float Waited = 0.0f;
	bool bLeft = false;
	while (!bLeft && Waited < 1.0f)
	{
		bLeft = Cast.Step(0.01f, false, true, Interval, 0.2f, FireTime, Length);
		Waited += 0.01f;
	}
	TestTrue(TEXT("the Bolt leaves though the button was let go"), bLeft);
	TestEqual(TEXT("0.2 seconds after the cast began"), Waited, 0.2f, 0.011f);
	TestEqual(TEXT("with the animation at its Fire moment"), Cast.AnimTime, FireTime);
	TestEqual(TEXT("fully shown then"), Cast.GetWeight(FireTime, Length), 1.0f);

	// Let go: the rest of the animation plays out, then stops.
	for (int32 Step = 0; Step < 130; ++Step)
	{
		TestFalse(TEXT("no Bolt with the button up"), Cast.Step(0.01f, false, true, Interval, 0.2f, FireTime, Length));
	}
	TestTrue(TEXT("the animation has played out"), Cast.AnimTime < 0.0f);
	TestEqual(TEXT("and shows nothing"), Cast.GetWeight(FireTime, Length), 0.0f);

	// Held: still one Bolt every Interval.
	FBoltCast Held;
	int32 Count = 0;
	for (int32 Step = 0; Step < 300; ++Step)
	{
		Count += Held.Step(0.01f, true, true, Interval, 0.2f, FireTime, Length) ? 1 : 0;
	}
	TestTrue(TEXT("held for three seconds: six or seven Bolts, as without the animation"), Count >= 6 && Count <= 7);

	// A menu comes up while a cast is in hand: no Bolt.
	FBoltCast Stopped;
	Stopped.Step(0.01f, true, true, Interval, 0.2f, FireTime, Length);
	bool bAny = false;
	for (int32 Step = 0; Step < 40; ++Step)
	{
		bAny = bAny || Stopped.Step(0.01f, true, false, Interval, 0.2f, FireTime, Length);
	}
	TestFalse(TEXT("a cast that ends while he may not cast lets no Bolt go"), bAny);

	const UClass* AnimClass = LoadClass<UAnimInstance>(nullptr, TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C"));
	TestTrue(TEXT("the mannequin's animation blueprint can lay a cast over the upper body"), AnimClass && AnimClass->IsChildOf(UWizardAnimInstance::StaticClass()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMenuPointer, "WebTesting.Hud.ThePointerOnMenusTheCrosshairInTheGame", WIZ_TEST_FLAGS)
bool FMenuPointer::RunTest(const FString& Parameters)
{
	TestFalse(TEXT("in the game: the crosshair"), AWizardHUD::WantsMenuPointer(false, false, false, false));
	TestTrue(TEXT("the start menu"), AWizardHUD::WantsMenuPointer(true, true, false, false));
	TestTrue(TEXT("the pause menu"), AWizardHUD::WantsMenuPointer(false, true, false, false));
	TestTrue(TEXT("the shrine's shop"), AWizardHUD::WantsMenuPointer(false, false, true, false));
	TestTrue(TEXT("the final cut screen"), AWizardHUD::WantsMenuPointer(false, false, false, true));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlyingModels, "WebTesting.Art.ArrowsTheSpearAndTheBonePileAreModels", WIZ_TEST_FLAGS)
bool FFlyingModels::RunTest(const FString& Parameters)
{
	// Direction of 2026-11-14 18:10 PST: the archer's arrow, the Marksman's arrow, the wizard's thrown spear, and
	// the bone pile in place of the dropped bones.
	const TCHAR* Flying[] = {
		TEXT("/Game/Weapons/SM_Archer_Arrow.SM_Archer_Arrow"),
		TEXT("/Game/Weapons/SM_Marksman_Arrow.SM_Marksman_Arrow"),
		TEXT("/Game/Weapons/SM_Wizard_Spear.SM_Wizard_Spear"),
	};
	for (const TCHAR* Path : Flying)
	{
		const UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, Path);
		if (TestNotNull(Path, Mesh))
		{
			const FBox Box = Mesh->GetBoundingBox();
			// The game lays a flying thing along its flight by its Z, around its middle.
			TestTrue(TEXT("it is longest along Z"), Box.GetSize().Z > Box.GetSize().X * 2.0 && Box.GetSize().Z > Box.GetSize().Y * 2.0);
			TestTrue(TEXT("and held by its middle"), FMath::Abs(Box.GetCenter().Z) < Box.GetSize().Z * 0.05);
		}
	}
	const UStaticMesh* Pile = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Props/SM_Bone_Pile.SM_Bone_Pile"));
	if (TestNotNull(TEXT("the bone pile"), Pile))
	{
		TestTrue(TEXT("it stands on the ground"), FMath::Abs(Pile->GetBoundingBox().Min.Z) < 2.0);
		TestTrue(TEXT("and is small enough to lie among the fighting"), Pile->GetBoundingBox().GetSize().GetMax() < 80.0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTurnedHouses, "WebTesting.Level.ATurnedHouseBlocksOnlyItsWalls", WIZ_TEST_FLAGS)
bool FTurnedHouses::RunTest(const FString& Parameters)
{
	// Direction of 2026-11-14 18:45 PST: "for all the houses, tighten the collision box on them if possible."
	const FBox House(FVector(-500.0, -300.0, 0.0), FVector(500.0, 300.0, 600.0));
	const auto Blocks = [](const TArray<FCrowdObstacle>& Boxes, FVector2D Point)
	{
		return Boxes.ContainsByPredicate([Point](const FCrowdObstacle& Box) { return Box.Contains(Point, 0.0); });
	};

	TArray<FCrowdObstacle> Square;
	UCrowdSubsystem::MakeBuildingObstacles(House, FTransform(FRotator::ZeroRotator, FVector(2000.0, 1000.0, 0.0)), Square);
	TestEqual(TEXT("a house square to the level is one box"), Square.Num(), 1);
	TestTrue(TEXT("its walls without its eaves"), Square[0].HalfSize.Equals(FVector2D(500.0, 300.0) * UCrowdSubsystem::BuildingBodyShare, 0.01));

	TArray<FCrowdObstacle> Quarter;
	UCrowdSubsystem::MakeBuildingObstacles(House, FTransform(FRotator(0.0, 90.0, 0.0), FVector::ZeroVector), Quarter);
	TestEqual(TEXT("a quarter turn is still one box"), Quarter.Num(), 1);
	TestTrue(TEXT("the other way round"), Quarter[0].HalfSize.Equals(FVector2D(300.0, 500.0) * UCrowdSubsystem::BuildingBodyShare, 0.5));

	TArray<FCrowdObstacle> Turned;
	const FTransform Slant(FRotator(0.0, 45.0, 0.0), FVector::ZeroVector);
	UCrowdSubsystem::MakeBuildingObstacles(House, Slant, Turned);
	TestTrue(TEXT("a turned house is several small boxes"), Turned.Num() >= 6);
	TestTrue(TEXT("its middle is blocked"), Blocks(Turned, FVector2D(0.0, 0.0)));
	const FVector AlongWall = Slant.TransformPosition(FVector(380.0, 0.0, 0.0));
	TestTrue(TEXT("and so is the inside near an end wall"), Blocks(Turned, FVector2D(AlongWall.X, AlongWall.Y)));
	// One box around the whole turned house would reach about 480 along X; its corner there is open ground.
	const FCrowdObstacle Whole = UCrowdSubsystem::MakeBuildingObstacle(House.TransformBy(Slant));
	const FVector2D OpenCorner(Whole.HalfSize.X - 20.0, -(Whole.HalfSize.Y - 20.0));
	TestTrue(TEXT("the one big box blocked open ground at its corner"), Whole.Contains(OpenCorner, 0.0));
	TestFalse(TEXT("the small boxes do not"), Blocks(Turned, OpenCorner));
	const FVector Outside = Slant.TransformPosition(FVector(0.0, 300.0 * UCrowdSubsystem::BuildingBodyShare + 120.0, 0.0));
	TestFalse(TEXT("a step outside a long wall is open"), Blocks(Turned, FVector2D(Outside.X, Outside.Y)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUpperBodyTurn, "WebTesting.Bolt.TheCastPointsAheadWhateverTheHipsDo", WIZ_TEST_FLAGS)
bool FUpperBodyTurn::RunTest(const FString& Parameters)
{
	// Direction of 2026-11-14 18:45 PST: "find a way so the throwing arm (right), throws correctly forward."
	const FQuat Animation = FRotator(5.0, 10.0, 0.0).Quaternion();
	TestTrue(TEXT("on still hips the upper body takes the animation's turn"),
		UWizardAnimInstance::MakeUpperRootRotation(FQuat::Identity, Animation).Equals(Animation, 0.0001));
	const FQuat Hips = FRotator(0.0, 40.0, 8.0).Quaternion();
	const FQuat Local = UWizardAnimInstance::MakeUpperRootRotation(Hips, Animation);
	TestTrue(TEXT("on hips that are turned and leaning, it still points the animation's way in the figure's space"), (Hips * Local).Equals(Animation, 0.0001));
	TestFalse(TEXT("which is not the animation's turn laid straight on the hips"), Local.Equals(Animation, 0.01));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRoadsAndBridges, "WebTesting.Level.RoadsArePaintedAndBridgesAreModels", WIZ_TEST_FLAGS)
bool FRoadsAndBridges::RunTest(const FString& Parameters)
{
	// Direction of 2026-11-14 18:45 PST: "remove all the road/path blocks and use the landscape"; the bridge model
	// "to replace bridge blocks"; "ensure the water doesn't go over the bridge or clip".
	const UWorld* Map = LoadObject<UWorld>(nullptr, TEXT("/Game/Maps/Outskirts.Outskirts"));
	if (!TestNotNull(TEXT("the Outskirts level"), Map) || !TestNotNull(TEXT("its level"), Map->PersistentLevel.Get()))
	{
		return false;
	}
	int32 Paths = 0;
	int32 BridgePieces = 0;
	int32 BridgeModels = 0;
	double WaterZ = 1.e9;
	double LowestDeck = 1.e9;
	for (const AActor* Actor : Map->PersistentLevel->Actors)
	{
		if (const ALevelPieceActor* Piece = Cast<ALevelPieceActor>(Actor))
		{
			Paths += Piece->Kind == ELevelPiece::Path ? 1 : 0;
			BridgePieces += Piece->Kind == ELevelPiece::Bridge ? 1 : 0;
		}
		const AStaticMeshActor* Mesh = Cast<AStaticMeshActor>(Actor);
		const UStaticMesh* Asset = Mesh && Mesh->GetStaticMeshComponent() ? Mesh->GetStaticMeshComponent()->GetStaticMesh() : nullptr;
		if (Asset && Asset->GetPathName() == TEXT("/Game/Props/SM_Bridge.SM_Bridge"))
		{
			++BridgeModels;
			// The top of the model, which is at least as high as its deck.
			const FTransform Placed = Mesh->GetStaticMeshComponent()->GetRelativeTransform();
			LowestDeck = FMath::Min(LowestDeck, Placed.GetLocation().Z + Asset->GetBoundingBox().Max.Z * Placed.GetScale3D().Z);
		}
		if (Asset && Asset->GetPathName() == TEXT("/Game/Level/SM_River_Water.SM_River_Water"))
		{
			WaterZ = Mesh->GetStaticMeshComponent()->GetRelativeTransform().GetLocation().Z;
		}
	}
	TestEqual(TEXT("no road blocks are left: the roads are painted on the ground"), Paths, 0);
	TestEqual(TEXT("each bridge piece has its model"), BridgeModels, BridgePieces);
	TestTrue(TEXT("there are bridges"), BridgePieces >= 1);
	TestTrue(TEXT("the water lies low, just over the ground"), WaterZ < 6.0);
	TestTrue(TEXT("and under the bridges"), LowestDeck > WaterZ + 10.0);

	const UStaticMesh* Spear = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Weapons/SM_Commander_Spear.SM_Commander_Spear"));
	if (TestNotNull(TEXT("the commander's spear"), Spear))
	{
		TestTrue(TEXT("it is longest along Z, to lie along its flight"), Spear->GetBoundingBox().GetSize().Z > Spear->GetBoundingBox().GetSize().X * 2.0);
	}
	TestNotNull(TEXT("the Marksman's picture for the shrine"), LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/T_Icon_Marksman.T_Icon_Marksman")));
	TestNotNull(TEXT("the steel hammer"), LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Weapons/SM_Steel_Hammer.SM_Steel_Hammer")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWallBoxes, "WebTesting.Level.WallsAreActorsInTheLevel", WIZ_TEST_FLAGS)
bool FWallBoxes::RunTest(const FString& Parameters)
{
	const FVector2D Straight = AWallActor::MakeHalfSize(FVector2D(400.0, 60.0), 0.0f, FVector2D(1.0, 1.0));
	TestTrue(TEXT("a wall's box is half its size each way"), Straight.Equals(FVector2D(200.0, 30.0), 0.01));
	const FVector2D Turned = AWallActor::MakeHalfSize(FVector2D(400.0, 60.0), 90.0f, FVector2D(1.0, 1.0));
	TestTrue(TEXT("a quarter turn swaps its length and thickness"), Turned.Equals(FVector2D(30.0, 200.0), 0.01));
	const FVector2D Scaled = AWallActor::MakeHalfSize(FVector2D(400.0, 60.0), 0.0f, FVector2D(2.0, -1.0));
	TestTrue(TEXT("the actor's scale counts, either way round"), Scaled.Equals(FVector2D(400.0, 30.0), 0.01));
	const FVector2D Slanted = AWallActor::MakeHalfSize(FVector2D(400.0, 60.0), 45.0f, FVector2D(1.0, 1.0));
	TestTrue(TEXT("a slanted wall takes up the box around it"), Slanted.X > 150.0 && FMath::IsNearlyEqual(Slanted.X, Slanted.Y, 0.01));

	const UWorld* Map = LoadObject<UWorld>(nullptr, TEXT("/Game/Maps/Outskirts.Outskirts"));
	if (!TestNotNull(TEXT("the Outskirts level"), Map) || !TestNotNull(TEXT("its level"), Map->PersistentLevel.Get()))
	{
		return false;
	}
	int32 Walls = 0;
	bool bRiver = false;
	for (const AActor* Actor : Map->PersistentLevel->Actors)
	{
		Walls += Cast<AWallActor>(Actor) ? 1 : 0;
		const AStaticMeshActor* Mesh = Cast<AStaticMeshActor>(Actor);
		const UStaticMesh* Asset = Mesh && Mesh->GetStaticMeshComponent() ? Mesh->GetStaticMeshComponent()->GetStaticMesh() : nullptr;
		bRiver = bRiver || (Asset && Asset->GetPathName() == TEXT("/Game/Level/SM_River_Water.SM_River_Water"));
	}
	// Changed 2026-11-14 19:40 PST: the director had the walls replaced by a battlefield with a soft edge
	// (WebTesting.Level.ABattlefieldNotWalls), so none are left. A Wall actor still works where one is placed.
	TestEqual(TEXT("the walls around the level were replaced by the battlefield's edge"), Walls, 0);
	TestTrue(TEXT("the river's water is one smooth mesh in the level"), bRiver);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
