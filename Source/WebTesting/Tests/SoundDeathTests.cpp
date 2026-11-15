#include "Animation/AnimSequenceBase.h"
#include "Engine/StaticMesh.h"
#include "Game/CrowdRenderer.h"
#include "Game/DeathPick.h"
#include "Game/WizardPawn.h"
#include "Game/WizardSounds.h"
#include "Misc/ConfigCacheIni.h"
#include "PhysicsEngine/BodySetup.h"
#include "Sim/CrowdSim.h"
#include "Sound/SoundBase.h"
#include "Tests/CrowdTestHelpers.h"
#include "Tests/WizTestFlags.h"

#if WITH_DEV_AUTOMATION_TESTS

// Direction of 2026-11-14 22:30 PST:
// - Deaths: "use A_Grux_Death_B, A_Kallari_Death_B, and A_Narbash_Death_Fwd. Have them be used based on where the
//   final blow came from."
// - "For Overwhelm, use A_Khaimera_Stunned_Start. Try to smooth it as much as possible from idle to this."
// - "For Skeleton Attacks, use A_Greystone_Attack_A_Med and A_Minions_Attack_B_SetB. Have the AI use one or the other
//   for each attack for variability." Brute, assassin, the commander's throw and attack each have one of their own.
// - Music: the menu's on the menu, the game's in the game, the end's at the gate, "a soft transition in and out".
// - Sound effects for the blink, the summon, a skeleton's blow (on a soldier or archer; on a brute or the commander),
//   a skeleton's death, a brute's slam that "hits nobody", the Bolt thrown and its impact ("not if it runs out of
//   range"), an assassin's miss, an archer's and a Marksman's shot, and a body falling on grass or dirt.
// - "Adjust the collision box that the player can go over. Sometimes, the player cannot go over the bridge."

namespace SoundTest
{
	const float StepSeconds = 0.05f;

	/** Steps the crowd and gathers every event of a type. */
	TArray<FCrowdEvent> Run(FCrowdSim& Sim, float Seconds, FVector2D Wizard, ECrowdEvent Type)
	{
		TArray<FCrowdEvent> Found;
		for (float Time = 0.0f; Time < Seconds - 0.001f; Time += StepSeconds)
		{
			Sim.Step(StepSeconds, Wizard);
			for (const FCrowdEvent& Event : Sim.GetEvents())
			{
				if (Event.Type == Type)
				{
					Found.Add(Event);
				}
			}
		}
		return Found;
	}

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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoundArmyBlow, "WebTesting.Sound.ASkeletonsBlowIsReported", WIZ_TEST_FLAGS)
bool FSoundArmyBlow::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	CrowdTest::MakeEnemiesPlain(Sim);
	Sim.Tuning.GruntSpeed = 0.0f;
	Sim.Tuning.GruntHealth = 100000.0f;
	Sim.Tuning.GruntDamagePerSecond = 0.0f;
	Sim.AddSkeleton(FVector2D(0.0, 0.0));
	Sim.AddEnemy(ECrowdKind::Grunt, FVector2D(60.0, 0.0));
	const TArray<FCrowdEvent> Blows = SoundTest::Run(Sim, 3.0f, FVector2D(0.0, -100.0), ECrowdEvent::ArmyBlow);
	TestTrue(TEXT("the skeleton's blows on the soldier are reported, one for each blow"), Blows.Num() >= 2 && Blows.Num() <= 4);
	if (Blows.Num() > 0)
	{
		TestEqual(TEXT("with the kind that was struck"), static_cast<int32>(Blows[0].Kind), static_cast<int32>(ECrowdKind::Grunt));
		TestTrue(TEXT("where it stands"), Blows[0].Position.Equals(FVector2D(60.0, 0.0), 30.0));
	}
	// The soldier strikes back all the while: its blows on the skeleton are not among these.
	for (const FCrowdEvent& Blow : Blows)
	{
		TestEqual(TEXT("every blow reported is one on an enemy"), static_cast<int32>(Blow.Team), static_cast<int32>(ECrowdTeam::Enemy));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoundBolt, "WebTesting.Sound.ABoltReportsWhatItMeetsAndNothingWhenItRunsOut", WIZ_TEST_FLAGS)
bool FSoundBolt::RunTest(const FString& Parameters)
{
	{
		FCrowdSim Sim;
		CrowdTest::MakeEnemiesPlain(Sim);
		Sim.Tuning.GruntSpeed = 0.0f;
		Sim.AddEnemy(ECrowdKind::Grunt, FVector2D(400.0, 0.0));
		Sim.FireBolt(CrowdTest::Origin, FVector2D(1.0, 0.0));
		const TArray<FCrowdEvent> Hits = SoundTest::Run(Sim, 1.5f, CrowdTest::Origin, ECrowdEvent::BoltHit);
		TestEqual(TEXT("a Bolt that meets an enemy reports it once"), Hits.Num(), 1);
	}
	{
		FCrowdSim Sim;
		Sim.SetObstacles({ CrowdTest::Wall(400.0, 0.0, 30.0, 200.0) });
		Sim.FireBolt(CrowdTest::Origin, FVector2D(1.0, 0.0));
		const TArray<FCrowdEvent> Walls = SoundTest::Run(Sim, 1.5f, CrowdTest::Origin, ECrowdEvent::BoltWall);
		TestEqual(TEXT("a Bolt that meets a wall reports it once"), Walls.Num(), 1);
		if (Walls.Num() > 0)
		{
			TestTrue(TEXT("at the wall"), Walls[0].Position.X > 300.0 && Walls[0].Position.X < 420.0);
		}
	}
	{
		FCrowdSim Sim;
		Sim.FireBolt(CrowdTest::Origin, FVector2D(1.0, 0.0));
		int32 Any = 0;
		for (float Time = 0.0f; Time < 3.0f; Time += SoundTest::StepSeconds)
		{
			Sim.Step(SoundTest::StepSeconds, CrowdTest::Origin);
			Any += Sim.GetEvents().Num();
		}
		TestEqual(TEXT("a Bolt that runs out of range reports nothing"), Any, 0);
		TestEqual(TEXT("and is gone"), Sim.GetBolts().Num(), 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDeathDirection, "WebTesting.Death.IsShownByWhereTheLastBlowCameFrom", WIZ_TEST_FLAGS)
bool FDeathDirection::RunTest(const FString& Parameters)
{
	const FVector2D Facing(1.0, 0.0);
	TestEqual(TEXT("a blow from straight ahead"), static_cast<int32>(FDeathPick::Pick(Facing, FVector2D::ZeroVector, FVector2D(300.0, 0.0))), static_cast<int32>(EDeathFall::FromFront));
	TestEqual(TEXT("a blow from a little to one side of ahead is still the front"), static_cast<int32>(FDeathPick::Pick(Facing, FVector2D::ZeroVector, FVector2D(300.0, 120.0))), static_cast<int32>(EDeathFall::FromFront));
	TestEqual(TEXT("a blow from behind"), static_cast<int32>(FDeathPick::Pick(Facing, FVector2D::ZeroVector, FVector2D(-300.0, 50.0))), static_cast<int32>(EDeathFall::FromBehind));
	TestEqual(TEXT("a blow from the left"), static_cast<int32>(FDeathPick::Pick(Facing, FVector2D::ZeroVector, FVector2D(0.0, 300.0))), static_cast<int32>(EDeathFall::FromSide));
	TestEqual(TEXT("a blow from the right"), static_cast<int32>(FDeathPick::Pick(Facing, FVector2D::ZeroVector, FVector2D(40.0, -300.0))), static_cast<int32>(EDeathFall::FromSide));
	TestEqual(TEXT("with no facing it is the front"), static_cast<int32>(FDeathPick::Pick(FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D(0.0, 300.0))), static_cast<int32>(EDeathFall::FromFront));

	// The three are the director's three, each its own.
	TestTrue(TEXT("from behind is Narbash falling forward"), FString(FDeathPick::AnimationPath(EDeathFall::FromBehind)).Contains(TEXT("A_Narbash_Death_Fwd")));
	TestTrue(TEXT("from the front is Kallari"), FString(FDeathPick::AnimationPath(EDeathFall::FromFront)).Contains(TEXT("A_Kallari_Death_B")));
	TestTrue(TEXT("from the side is Grux"), FString(FDeathPick::AnimationPath(EDeathFall::FromSide)).Contains(TEXT("A_Grux_Death_B")));
	for (int32 Index = 0; Index < static_cast<int32>(EDeathFall::Count); ++Index)
	{
		const EDeathFall Fall = static_cast<EDeathFall>(Index);
		const UAnimSequenceBase* Animation = ACrowdRenderer::LoadAttack(FDeathPick::AnimationPath(Fall));
		if (TestNotNull(*FString::Printf(TEXT("the death from the %s is in the project"), FDeathPick::Name(Fall)), Animation))
		{
			TestTrue(TEXT("the body meets the ground inside the animation"), FDeathPick::FallSeconds(Fall) > 0.2f && FDeathPick::FallSeconds(Fall) < Animation->GetPlayLength());
		}
	}

	// The simulation says where the blow that killed came from: here a Bolt from the west, on a soldier that looks east.
	FCrowdSim Sim;
	CrowdTest::MakeEnemiesPlain(Sim);
	Sim.Tuning.GruntHealth = 1.0f;
	Sim.AddEnemy(ECrowdKind::Grunt, FVector2D(400.0, 0.0));
	// The wizard stands far to the east, so the soldier walks (and faces) east; the Bolt comes from behind it.
	const FVector2D Wizard(3000.0, 0.0);
	for (int32 Step = 0; Step < 6; ++Step)
	{
		Sim.Step(SoundTest::StepSeconds, Wizard);
	}
	Sim.FireBolt(FVector2D(0.0, 0.0), FVector2D(1.0, 0.0));
	const TArray<FCrowdEvent> Deaths = SoundTest::Run(Sim, 2.0f, Wizard, ECrowdEvent::Death);
	if (TestEqual(TEXT("its death is reported once"), Deaths.Num(), 1))
	{
		TestEqual(TEXT("as an enemy's"), static_cast<int32>(Deaths[0].Team), static_cast<int32>(ECrowdTeam::Enemy));
		TestTrue(TEXT("with where the Bolt came at it from: the west"), Deaths[0].From.X < Deaths[0].Position.X && FMath::Abs(Deaths[0].From.Y - Deaths[0].Position.Y) < 60.0);
		TestEqual(TEXT("which is behind it"), static_cast<int32>(FDeathPick::Pick(Deaths[0].Facing, Deaths[0].Position, Deaths[0].From)), static_cast<int32>(EDeathFall::FromBehind));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoundSlamMiss, "WebTesting.Sound.ASlamOnNobodyIsReportedAsAMiss", WIZ_TEST_FLAGS)
bool FSoundSlamMiss::RunTest(const FString& Parameters)
{
	// The wizard stands in front of the brute until its circle starts, then steps well away: the slam lands on nobody.
	FCrowdSim Sim;
	CrowdTest::MakeEnemiesPlain(Sim);
	Sim.Tuning.BruteSpeed = 0.0f;
	Sim.AddEnemy(ECrowdKind::Brute, CrowdTest::Origin);
	FVector2D Wizard(200.0, 0.0);
	int32 Misses = 0;
	int32 Landed = 0;
	for (float Time = 0.0f; Time < 4.0f && Landed == 0; Time += SoundTest::StepSeconds)
	{
		const FCrowdUnit* Brute = SoundTest::FirstOf(Sim, ECrowdKind::Brute);
		if (Brute && Sim.IsSlamming(*Brute))
		{
			Wizard = FVector2D(2000.0, 0.0);
		}
		Landed += Sim.Step(SoundTest::StepSeconds, Wizard).SlamsLanded;
		for (const FCrowdEvent& Event : Sim.GetEvents())
		{
			Misses += Event.Type == ECrowdEvent::SlamMiss ? 1 : 0;
		}
	}
	TestEqual(TEXT("the slam landed"), Landed, 1);
	TestEqual(TEXT("on nobody: a miss"), Misses, 1);

	// One that catches the wizard is not a miss.
	FCrowdSim Hit;
	CrowdTest::MakeEnemiesPlain(Hit);
	Hit.Tuning.BruteSpeed = 0.0f;
	Hit.AddEnemy(ECrowdKind::Brute, CrowdTest::Origin);
	int32 HitLanded = 0;
	int32 HitMisses = 0;
	for (float Time = 0.0f; Time < 4.0f && HitLanded == 0; Time += SoundTest::StepSeconds)
	{
		HitLanded += Hit.Step(SoundTest::StepSeconds, FVector2D(200.0, 0.0)).SlamsLanded;
		for (const FCrowdEvent& Event : Hit.GetEvents())
		{
			HitMisses += Event.Type == ECrowdEvent::SlamMiss ? 1 : 0;
		}
	}
	TestEqual(TEXT("a slam on the wizard landed"), HitLanded, 1);
	TestEqual(TEXT("and is no miss"), HitMisses, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoundArrow, "WebTesting.Sound.AnArrowLoosedIsReported", WIZ_TEST_FLAGS)
bool FSoundArrow::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	CrowdTest::MakeEnemiesPlain(Sim);
	Sim.AddEnemy(ECrowdKind::Archer, FVector2D(600.0, 0.0));
	int32 Fired = 0;
	int32 Reported = 0;
	for (float Time = 0.0f; Time < 4.0f; Time += SoundTest::StepSeconds)
	{
		Fired += Sim.Step(SoundTest::StepSeconds, CrowdTest::Origin).ArrowsFired;
		for (const FCrowdEvent& Event : Sim.GetEvents())
		{
			Reported += Event.Type == ECrowdEvent::ArrowLoosed && Event.Kind == ECrowdKind::Archer ? 1 : 0;
		}
	}
	TestTrue(TEXT("the archer shot"), Fired >= 2);
	TestEqual(TEXT("every arrow is reported"), Reported, Fired);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAssassinSwing, "WebTesting.Assassin.SwingsAtTheWizardAndMissesWhenHeIsGone", WIZ_TEST_FLAGS)
bool FAssassinSwing::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	CrowdTest::MakeEnemiesPlain(Sim);
	Sim.Tuning.AssassinSpeed = 0.0f;
	Sim.AddEnemy(ECrowdKind::Assassin, FVector2D(60.0, 0.0));
	const int32 TouchingBefore = CrowdTest::CountTouching(Sim, CrowdTest::Origin);

	// On the wizard it swings, again and again, and its swings reach him.
	TestTrue(TEXT("its swings at the wizard land"), SoundTest::Run(Sim, 3.0f, CrowdTest::Origin, ECrowdEvent::AssassinBlow).Num() >= 2);
	const FCrowdUnit* Assassin = SoundTest::FirstOf(Sim, ECrowdKind::Assassin);
	float TimeToBlow = 0.0f;
	TestTrue(TEXT("it is shown striking"), Assassin && Sim.GetStrike(*Assassin, TimeToBlow));
	TestEqual(TEXT("its swing changes nothing of what it counts for on the meter"), CrowdTest::CountTouching(Sim, CrowdTest::Origin), TouchingBefore);

	// The wizard blinks: the swing that ends meanwhile misses.
	Sim.bWizardInvulnerable = true;
	TestTrue(TEXT("a swing while he blinks misses"), SoundTest::Run(Sim, 1.5f, CrowdTest::Origin, ECrowdEvent::AssassinMiss).Num() >= 1);
	Sim.bWizardInvulnerable = false;

	// The wizard is gone: the swing it was in misses, once, and it stops swinging.
	const FVector2D Far(3000.0, 0.0);
	const TArray<FCrowdEvent> Misses = SoundTest::Run(Sim, 3.0f, Far, ECrowdEvent::AssassinMiss);
	TestEqual(TEXT("the swing it was in misses, once"), Misses.Num(), 1);
	Assassin = SoundTest::FirstOf(Sim, ECrowdKind::Assassin);
	TestFalse(TEXT("then it is not striking"), Assassin && Sim.GetStrike(*Assassin, TimeToBlow));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommanderThrowShown, "WebTesting.Commander.TheThrowIsShownBeforeAndAfterTheSpearLeaves", WIZ_TEST_FLAGS)
bool FCommanderThrowShown::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	CrowdTest::MakeEnemiesPlain(Sim);
	Sim.Tuning.CommanderSpeed = 0.0f;
	Sim.Tuning.CommanderSpearRange = 5000.0f;
	Sim.AddEnemy(ECrowdKind::Commander, FVector2D(800.0, 0.0));
	bool bSawBefore = false;
	bool bSawAfter = false;
	bool bEnded = false;
	float LastBefore = 1000.0f;
	int32 Thrown = 0;
	for (float Time = 0.0f; Time < 6.0f && !bEnded; Time += SoundTest::StepSeconds)
	{
		Thrown += Sim.Step(SoundTest::StepSeconds, CrowdTest::Origin).JavelinsThrown;
		const FCrowdUnit* Commander = SoundTest::FirstOf(Sim, ECrowdKind::Commander);
		float TimeToThrow = 0.0f;
		const bool bShown = Commander && Sim.GetThrow(*Commander, TimeToThrow);
		if (bShown && Thrown == 0)
		{
			bSawBefore = true;
			TestTrue(TEXT("drawing back, the time to the throw counts down from the wind-up"), TimeToThrow >= -0.001f && TimeToThrow <= Sim.Tuning.CommanderSpearWindup + 0.001f && TimeToThrow < LastBefore);
			LastBefore = TimeToThrow;
		}
		else if (bShown)
		{
			bSawAfter = true;
			TestTrue(TEXT("after it, the time is how long ago the spear left"), TimeToThrow <= 0.0f);
		}
		else if (Thrown > 0)
		{
			bEnded = true;
		}
	}
	TestTrue(TEXT("the throw is shown while he draws back"), bSawBefore);
	TestTrue(TEXT("and as he follows it through"), bSawAfter);
	TestTrue(TEXT("and then no more, until the next"), bEnded);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoundRules, "WebTesting.Sound.TheRulesPickTheRightSound", WIZ_TEST_FLAGS)
bool FSoundRules::RunTest(const FString& Parameters)
{
	// A skeleton's blow.
	TestEqual(TEXT("on a soldier"), static_cast<int32>(FWizardSoundRules::ForArmyBlow(ECrowdKind::Grunt)), static_cast<int32>(EWizardCue::SwordHitsEnemy));
	TestEqual(TEXT("on an archer"), static_cast<int32>(FWizardSoundRules::ForArmyBlow(ECrowdKind::Archer)), static_cast<int32>(EWizardCue::SwordHitsEnemy));
	TestEqual(TEXT("on a brute"), static_cast<int32>(FWizardSoundRules::ForArmyBlow(ECrowdKind::Brute)), static_cast<int32>(EWizardCue::SwordHitsArmor));
	TestEqual(TEXT("on the commander"), static_cast<int32>(FWizardSoundRules::ForArmyBlow(ECrowdKind::Commander)), static_cast<int32>(EWizardCue::SwordHitsArmor));

	// A body on the ground: two sounds for each kind of ground.
	TSet<int32> Dirt;
	TSet<int32> Grass;
	for (int32 Variant = 0; Variant < 6; ++Variant)
	{
		Dirt.Add(static_cast<int32>(FWizardSoundRules::ForBodyFall(true, Variant)));
		Grass.Add(static_cast<int32>(FWizardSoundRules::ForBodyFall(false, Variant)));
	}
	TestTrue(TEXT("dirt has its two"), Dirt.Num() == 2 && Dirt.Contains(static_cast<int32>(EWizardCue::BodyFallDirt1)) && Dirt.Contains(static_cast<int32>(EWizardCue::BodyFallDirt2)));
	TestTrue(TEXT("grass has its two"), Grass.Num() == 2 && Grass.Contains(static_cast<int32>(EWizardCue::BodyFallGrass1)) && Grass.Contains(static_cast<int32>(EWizardCue::BodyFallGrass2)));

	TestTrue(TEXT("a skeleton dies as a skeleton"), FWizardSoundRules::DiesAsSkeleton(ECrowdTeam::Army, ECrowdKind::Skeleton));
	TestFalse(TEXT("a soldier does not"), FWizardSoundRules::DiesAsSkeleton(ECrowdTeam::Enemy, ECrowdKind::Grunt));

	// The director's trims.
	TestEqual(TEXT("the skeleton's death starts at its second rattle"), FWizardSoundRules::Info(EWizardCue::SkeletonDies).StartSeconds, 0.3f);
	TestEqual(TEXT("the dagger's miss is its first half second"), FWizardSoundRules::Info(EWizardCue::DaggerMiss).PlaySeconds, 0.5f);

	// Farther is quieter.
	TestEqual(TEXT("beside the wizard it is at full loudness"), FWizardSoundRules::DistanceVolume(100.0), 1.0f);
	TestTrue(TEXT("farther it is quieter"), FWizardSoundRules::DistanceVolume(2000.0) < 1.0f && FWizardSoundRules::DistanceVolume(2000.0) > FWizardSoundRules::DistanceVolume(2800.0));
	TestEqual(TEXT("far away it is silent"), FWizardSoundRules::DistanceVolume(5000.0), 0.0f);

	// Music.
	TestEqual(TEXT("the start menu has the menu's music"), static_cast<int32>(FWizardSoundRules::MusicFor(true, false)), static_cast<int32>(EWizardMusic::Menu));
	TestEqual(TEXT("the game has the game's"), static_cast<int32>(FWizardSoundRules::MusicFor(false, false)), static_cast<int32>(EWizardMusic::Game));
	TestEqual(TEXT("the gate has the end's"), static_cast<int32>(FWizardSoundRules::MusicFor(false, true)), static_cast<int32>(EWizardMusic::End));
	TestTrue(TEXT("the menu's is Darkness Rises"), FString(FWizardSoundRules::MusicPath(EWizardMusic::Menu)).Contains(TEXT("MUS_01_Menu_Darkness_Rises")));
	TestTrue(TEXT("the game's is Forgotten Pleasure"), FString(FWizardSoundRules::MusicPath(EWizardMusic::Game)).Contains(TEXT("MUS_03_Forgotten_Pleasure")));
	TestTrue(TEXT("the end's is Beyond The Horizon"), FString(FWizardSoundRules::MusicPath(EWizardMusic::End)).Contains(TEXT("MUS_02_Start_Beyond_The_Horizon")));
	TestTrue(TEXT("music fades in and out, softly"), FWizardSoundRules::MusicFadeSeconds >= 1.5f);

	// Every sound is in the project.
	for (int32 Index = 0; Index < static_cast<int32>(EWizardCue::Count); ++Index)
	{
		const FWizardCueInfo& Info = FWizardSoundRules::Info(static_cast<EWizardCue>(Index));
		TestNotNull(*FString::Printf(TEXT("the sound for %s"), Info.Name), LoadObject<USoundBase>(nullptr, Info.Path));
		TestEqual(TEXT("its name finds it"), static_cast<int32>(FWizardSoundRules::CueByName(Info.Name)), Index);
	}
	for (const EWizardMusic Music : { EWizardMusic::Menu, EWizardMusic::Game, EWizardMusic::End })
	{
		TestNotNull(*FString::Printf(TEXT("the %s music"), FWizardSoundRules::MusicName(Music)), LoadObject<USoundBase>(nullptr, FWizardSoundRules::MusicPath(Music)));
	}

	// Thirty blows in one frame are one sound.
	FWizardCueThrottle Throttle;
	TestTrue(TEXT("the first starts"), Throttle.Allow(EWizardCue::SwordHitsEnemy, 10.0));
	TestFalse(TEXT("the same again at once does not"), Throttle.Allow(EWizardCue::SwordHitsEnemy, 10.01));
	TestTrue(TEXT("another sound does"), Throttle.Allow(EWizardCue::BowShoot, 10.01));
	TestTrue(TEXT("and the first again a moment later"), Throttle.Allow(EWizardCue::SwordHitsEnemy, 10.2));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGroundPaintGrid, "WebTesting.Sound.TheGroundIsDirtOrGrassByItsPaint", WIZ_TEST_FLAGS)
bool FGroundPaintGrid::RunTest(const FString& Parameters)
{
	FGroundPaint Paint;
	TestFalse(TEXT("with no grid, everywhere is grass"), Paint.IsDirt(FVector2D(0.0, 0.0)));
	Paint.Width = 2;
	Paint.Height = 2;
	Paint.Min = FVector2D(0.0, 0.0);
	Paint.Max = FVector2D(200.0, 200.0);
	// Row 0 (low Y): grass, dirt. Row 1: dirt, grass.
	Paint.Cells = TEXT("0110");
	TestTrue(TEXT("the grid is whole"), Paint.IsValid());
	TestFalse(TEXT("low X, low Y is grass"), Paint.IsDirt(FVector2D(50.0, 50.0)));
	TestTrue(TEXT("high X, low Y is dirt"), Paint.IsDirt(FVector2D(150.0, 50.0)));
	TestTrue(TEXT("low X, high Y is dirt"), Paint.IsDirt(FVector2D(50.0, 150.0)));
	TestFalse(TEXT("high X, high Y is grass"), Paint.IsDirt(FVector2D(150.0, 150.0)));
	TestFalse(TEXT("off the grid is grass"), Paint.IsDirt(FVector2D(-10.0, 50.0)));

	// The level's own grid is in the game's settings, baked from the landscape's paint.
	int32 Width = 0;
	FString Cells;
	TestTrue(TEXT("the level's grid is in the settings"), GConfig->GetInt(TEXT("WizardGroundPaint"), TEXT("Width"), Width, GGameIni) && GConfig->GetString(TEXT("WizardGroundPaint"), TEXT("Cells"), Cells, GGameIni));
	TestTrue(TEXT("and is square"), Width > 0 && Cells.Len() == Width * Width);
	TestTrue(TEXT("with both dirt and grass in it"), Cells.Contains(TEXT("1")) && Cells.Contains(TEXT("0")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSwingTimingShape, "WebTesting.Model.AWoundUpAttackStretchesItsWindUpAndFollowsThroughAtItsOwnSpeed", WIZ_TEST_FLAGS)
bool FSwingTimingShape::RunTest(const FString& Parameters)
{
	const FCrowdTuning Tuning;
	// A brute's slam: an animation 1.5 s long whose blow is 0.3 through it (0.45 s in).
	const ACrowdRenderer::FSwingTiming Slam = ACrowdRenderer::MakeSwingTiming(Tuning, ECrowdKind::Brute, 0, 0.3f, 1.5f);
	const float WindUp = Tuning.BruteSlamGrow + Tuning.BruteSlamHold;
	TestTrue(TEXT("it is wound up"), Slam.bOneShot);
	TestEqual(TEXT("its blow comes at the end of the simulation's wind-up"), Slam.BlowAt, WindUp, 0.001f);
	TestEqual(TEXT("it starts at the animation's start"), Slam.PositionAt(0.0f), 0.0f, 0.001f);
	TestEqual(TEXT("half way through the wind-up it is half way to the blow"), Slam.PositionAt(WindUp * 0.5f), 0.225f, 0.001f);
	TestEqual(TEXT("at the blow it is at the animation's blow"), Slam.PositionAt(WindUp), 0.45f, 0.001f);
	TestEqual(TEXT("after it, it plays at its own speed"), Slam.PositionAt(WindUp + 0.4f), 0.85f, 0.001f);
	TestEqual(TEXT("and stops at its end"), Slam.PositionAt(WindUp + 5.0f), 1.5f, 0.001f);
	TestEqual(TEXT("one turn is the wind-up and the rest of the animation"), Slam.Cycle, WindUp + 1.05f, 0.001f);

	// The commander's two: his slash and his throw have their own wind-ups.
	TestEqual(TEXT("the slash winds up as the slash does"), ACrowdRenderer::MakeSwingTiming(Tuning, ECrowdKind::Commander, 0, 0.2f, 1.6f).BlowAt, Tuning.CommanderSlashGrow + Tuning.CommanderSlashHold, 0.001f);
	TestEqual(TEXT("the throw winds up as the throw does"), ACrowdRenderer::MakeSwingTiming(Tuning, ECrowdKind::Commander, 1, 0.2f, 1.5f).BlowAt, Tuning.CommanderSpearWindup, 0.001f);

	// A skeleton's blow comes again and again: the whole animation turns once for each.
	const ACrowdRenderer::FSwingTiming Sword = ACrowdRenderer::MakeSwingTiming(Tuning, ECrowdKind::Skeleton, 0, 0.33f, 0.8f);
	TestFalse(TEXT("a sword blow is not wound up"), Sword.bOneShot);
	TestEqual(TEXT("one turn is the time between blows"), Sword.Cycle, Tuning.MeleeInterval, 0.001f);
	TestEqual(TEXT("at the blow it is at the animation's blow"), Sword.PositionAt(Sword.BlowAt), 0.33f * 0.8f, 0.001f);

	// The director's picks are in the project, and a missing one is nothing, not a crash.
	const TCHAR* Picks[] = {
		TEXT("/Game/Paragon_Picks/03_Attack_Skeleton/A_Greystone_Attack_A_Med.A_Greystone_Attack_A_Med"),
		TEXT("/Game/Paragon_Picks/03_Attack_Skeleton/A_Minions_Attack_B_SetB.A_Minions_Attack_B_SetB"),
		TEXT("/Game/Paragon_Picks/04_Attack_Brute/A_Terra_Primary_Melee_D_slow.A_Terra_Primary_Melee_D_slow"),
		TEXT("/Game/Paragon_Picks/05_Attack_Assassin/A_Kallari_Attack_GuardBreak_A.A_Kallari_Attack_GuardBreak_A"),
		TEXT("/Game/Paragon_Picks/06_Spear_Throw_Commander/A_Narbash_RMB_Ability_Throw.A_Narbash_RMB_Ability_Throw"),
		TEXT("/Game/Paragon_Picks/07_Attack_Commander/A_Greystone_Attack_PrimaryA.A_Greystone_Attack_PrimaryA"),
		TEXT("/Game/Paragon_Picks/02_Player_Fall_Overwhelmed/A_Khaimera_Stunned_Start.A_Khaimera_Stunned_Start"),
		TEXT("/Game/Paragon_Picks/09_Optional_Run_Walk_Idle/A_Greystone_Jog_Fwd.A_Greystone_Jog_Fwd"),
		TEXT("/Game/Paragon_Picks/10_Extras_Noted/A_Gideon_LevelStart.A_Gideon_LevelStart"),
	};
	for (const TCHAR* Pick : Picks)
	{
		TestNotNull(Pick, ACrowdRenderer::LoadAttack(Pick));
	}
	TestNull(TEXT("no path gives nothing"), ACrowdRenderer::LoadAttack(nullptr));
	TestNull(TEXT("an animation that is not there gives nothing"), ACrowdRenderer::LoadAttack(TEXT("/Game/Paragon_Picks/03_Attack_Skeleton/A_Not_There.A_Not_There")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardFallEase, "WebTesting.Wizard.OverwhelmedHeEasesToHisKneesAndThenGoesUnder", WIZ_TEST_FLAGS)
bool FWizardFallEase::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("at the start none of the animation shows"), AWizardPawn::GetFallBlend(0.0f), 0.0f);
	TestEqual(TEXT("after the blend all of it does"), AWizardPawn::GetFallBlend(AWizardPawn::FallBlendSeconds), 1.0f);
	float Last = 0.0f;
	float BiggestStep = 0.0f;
	for (int32 Step = 1; Step <= 20; ++Step)
	{
		const float Now = AWizardPawn::GetFallBlend(AWizardPawn::FallBlendSeconds * Step / 20.0f);
		TestTrue(TEXT("it only ever grows"), Now >= Last);
		BiggestStep = FMath::Max(BiggestStep, Now - Last);
		Last = Now;
	}
	TestTrue(TEXT("with no jump in it"), BiggestStep < 0.1f);
	TestTrue(TEXT("it starts gently"), AWizardPawn::GetFallBlend(AWizardPawn::FallBlendSeconds * 0.1f) < 0.05f);
	TestTrue(TEXT("and ends gently"), AWizardPawn::GetFallBlend(AWizardPawn::FallBlendSeconds * 0.9f) > 0.95f);
	TestTrue(TEXT("the blend takes a moment, not an instant"), AWizardPawn::FallBlendSeconds >= 0.3f);

	// With the animation he kneels first and only then sinks; without it he sinks from the start, as before.
	TestEqual(TEXT("half way through the fall he has not sunk"), AWizardPawn::GetFallSink(0.5f, true), 0.0f);
	TestTrue(TEXT("late in it he is going under"), AWizardPawn::GetFallSink(0.9f, true) > 0.3f);
	TestEqual(TEXT("at the end he is under"), AWizardPawn::GetFallSink(1.0f, true), 1.0f);
	TestEqual(TEXT("without the animation it is as it was"), AWizardPawn::GetFallSink(0.5f, false), 0.25f, 0.001f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBridgeForWalking, "WebTesting.Level.TheBridgeIsMadeForWalking", WIZ_TEST_FLAGS)
bool FBridgeForWalking::RunTest(const FString& Parameters)
{
	const UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Props/SM_Bridge.SM_Bridge"));
	const UBodySetup* Body = Mesh ? Mesh->GetBodySetup() : nullptr;
	if (!TestNotNull(TEXT("the bridge model has collision"), Body))
	{
		return false;
	}
	int32 Ramps = 0;
	double Steepest = 0.0;
	for (const FKConvexElem& Convex : Body->AggGeom.ConvexElems)
	{
		if (Convex.GetName() != FName(TEXT("Deck")))
		{
			AddError(TEXT("a shape on the bridge that is neither its deck nor a rail: it can catch a walker"));
			continue;
		}
		++Ramps;
		// Its top runs from its first corner to its second: how steep that is.
		if (Convex.VertexData.Num() >= 2)
		{
			const FVector Along = Convex.VertexData[1] - Convex.VertexData[0];
			Steepest = FMath::Max(Steepest, FMath::Abs(Along.Z) / FMath::Max(FMath::Abs(Along.X), 1.0));
		}
	}
	TestTrue(TEXT("the deck is a run of ramps, with one more past each end"), Ramps >= 10);
	TestTrue(TEXT("none steeper than a walker climbs with ease (1 in 2)"), Steepest <= 0.51);
	int32 Rails = 0;
	for (const FKBoxElem& Box : Body->AggGeom.BoxElems)
	{
		Rails += Box.GetName() == FName(TEXT("Rail")) ? 1 : 0;
	}
	TestEqual(TEXT("one rail down each side"), Rails, 2);
	TestEqual(TEXT("and nothing else"), Body->AggGeom.BoxElems.Num() + Body->AggGeom.SphereElems.Num() + Body->AggGeom.SphylElems.Num(), 2);
	return true;
}

#endif
