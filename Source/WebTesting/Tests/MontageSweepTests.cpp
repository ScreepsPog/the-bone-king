#include "Animation/AnimSequenceBase.h"
#include "Game/CrowdRenderer.h"
#include "Game/DeathPick.h"
#include "Game/WizardSounds.h"
#include "Sim/CrowdSim.h"
#include "Tests/CrowdTestHelpers.h"
#include "Tests/WizTestFlags.h"

#if WITH_DEV_AUTOMATION_TESTS

// Direction of 2026-11-15 00:20 PST:
// - "I created a bunch of Anim Montages for everything. They all have the same animation name, with Anim_ added to
//   the beginning."
// - Deaths: "ensure that the sound for hitting the ground lines up perfectly on when the notify begins. This doesn't
//   mean starting the sound from the start, but when the actual sound is heard."
// - "For the Brute, keep the current aoe attack with it's anim. Add a new basic attack that uses
//   Anim_A_Terra_Primary_Melee_D_slow and does a sweep attack in a shape of about 1/3 sphere".

namespace SweepTest
{
	const float StepSeconds = 0.05f;

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMontageNotifies, "WebTesting.Model.TheMontagesSayWhenThingsHappen", WIZ_TEST_FLAGS)
bool FMontageNotifies::RunTest(const FString& Parameters)
{
	// Each death is taken from its montage, and the "PlaySound" notify there is when the body meets the ground.
	const FName Thud[] = { FName(TEXT("PlaySound")) };
	const float Expected[] = { 1.0f, 0.886f, 0.597f };
	for (int32 Index = 0; Index < static_cast<int32>(EDeathFall::Count); ++Index)
	{
		const EDeathFall Fall = static_cast<EDeathFall>(Index);
		const UAnimSequenceBase* Death = ACrowdRenderer::LoadAttack(FDeathPick::AnimationPath(Fall));
		if (!TestNotNull(TEXT("the death"), Death))
		{
			continue;
		}
		TestTrue(*FString::Printf(TEXT("the death from the %s is the montage"), FDeathPick::Name(Fall)), Death->GetName().StartsWith(TEXT("Anim_A_")));
		TestEqual(*FString::Printf(TEXT("the body meets the ground where the montage says (%s)"), FDeathPick::Name(Fall)), ACrowdRenderer::GetNotifySeconds(Death, Thud), Expected[Index], 0.005f);
	}
	// The blow of an attack is at the "Attack" notify; the throw lets go at "ThrowSpear".
	const UAnimSequenceBase* Sword = ACrowdRenderer::LoadAttack(TEXT("/Game/Paragon_Picks/03_Attack_Skeleton/A_Greystone_Attack_A_Med.A_Greystone_Attack_A_Med"));
	if (TestNotNull(TEXT("the sword blow"), Sword))
	{
		TestEqual(TEXT("the sword blow lands at its Attack notify"), ACrowdRenderer::GetFireShare(Sword, -1.0f) * Sword->GetPlayLength(), 0.257f, 0.005f);
	}
	const UAnimSequenceBase* Throw = ACrowdRenderer::LoadAttack(TEXT("/Game/Paragon_Picks/06_Spear_Throw_Commander/A_Narbash_RMB_Ability_Throw.A_Narbash_RMB_Ability_Throw"));
	if (TestNotNull(TEXT("the throw"), Throw))
	{
		TestEqual(TEXT("the spear leaves at its ThrowSpear notify"), ACrowdRenderer::GetFireShare(Throw, -1.0f) * Throw->GetPlayLength(), 0.244f, 0.005f);
	}
	const FName Nothing[] = { FName(TEXT("NoSuchNotify")) };
	TestTrue(TEXT("a notify that is not there is below zero"), ACrowdRenderer::GetNotifySeconds(Sword, Nothing) < 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBodyFallLead, "WebTesting.Sound.TheThudIsHeardWhenTheBodyMeetsTheGround", WIZ_TEST_FLAGS)
bool FBodyFallLead::RunTest(const FString& Parameters)
{
	// Every body-fall sound has quiet before its thud.
	for (const EWizardCue Cue : { EWizardCue::BodyFallDirt1, EWizardCue::BodyFallDirt2, EWizardCue::BodyFallGrass1, EWizardCue::BodyFallGrass2 })
	{
		const float Lead = FWizardSoundRules::Info(Cue).HeardFrom;
		TestTrue(TEXT("its quiet lead-in is known"), Lead > 0.1f && Lead < 0.7f);
		// The body lands 0.597 s after the death (the shortest of the three): the sound starts early by its lead-in.
		float Wait = 0.0f;
		float Skip = 0.0f;
		FWizardSoundRules::GetLead(Cue, 0.597f, Wait, Skip);
		TestEqual(TEXT("it is started so that the thud falls on the moment"), Wait + (Lead - Skip), 0.597f, 0.0005f);
		TestEqual(TEXT("with none of it cut"), Skip, 0.0f);
		// A moment nearer than the lead-in: it starts at once, with the difference cut from its quiet front.
		FWizardSoundRules::GetLead(Cue, 0.1f, Wait, Skip);
		TestEqual(TEXT("too near to wait"), Wait, 0.0f);
		TestEqual(TEXT("the thud still falls on the moment"), Lead - Skip, 0.1f, 0.0005f);
	}
	TestEqual(TEXT("the first dirt thud is 0.545 s into its file"), FWizardSoundRules::Info(EWizardCue::BodyFallDirt1).HeardFrom, 0.545f);
	TestEqual(TEXT("the first grass thud is 0.225 s into its file"), FWizardSoundRules::Info(EWizardCue::BodyFallGrass1).HeardFrom, 0.225f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBruteSweep, "WebTesting.Brute.SweepsAThirdOfACircleBetweenItsSlams", WIZ_TEST_FLAGS)
bool FBruteSweep::RunTest(const FString& Parameters)
{
	// The shape: a third of a circle across its front.
	FCrowdSim Shape;
	FCrowdUnit Brute;
	Brute.Kind = ECrowdKind::Brute;
	Brute.Position = FVector2D(0.0, 0.0);
	Brute.Facing = FVector2D(1.0, 0.0);
	TestEqual(TEXT("a third of a circle"), Shape.Tuning.BruteSweepArc, 120.0f);
	TestTrue(TEXT("straight ahead is in it"), Shape.IsInSweep(Brute, FVector2D(200.0, 0.0)));
	TestTrue(TEXT("56 degrees to one side is in it"), Shape.IsInSweep(Brute, FVector2D(100.0, 150.0)));
	TestTrue(TEXT("and to the other"), Shape.IsInSweep(Brute, FVector2D(100.0, -150.0)));
	TestFalse(TEXT("61 degrees to one side is not"), Shape.IsInSweep(Brute, FVector2D(100.0, 181.0)));
	TestFalse(TEXT("straight to the side is not"), Shape.IsInSweep(Brute, FVector2D(0.0, 200.0)));
	TestFalse(TEXT("behind is not"), Shape.IsInSweep(Brute, FVector2D(-150.0, 0.0)));
	TestFalse(TEXT("past its reach is not"), Shape.IsInSweep(Brute, FVector2D(Shape.Tuning.BruteSweepRadius + 10.0, 0.0)));

	// In a fight: the slam first, then sweeps until the next slam is ready.
	FCrowdSim Sim;
	CrowdTest::MakeEnemiesPlain(Sim);
	Sim.Tuning.BruteSpeed = 0.0f;
	Sim.AddEnemy(ECrowdKind::Brute, CrowdTest::Origin);
	const FVector2D Wizard(150.0, 0.0);
	int32 Slams = 0;
	int32 Sweeps = 0;
	int32 SweepHits = 0;
	int32 SweepsBeforeSecondSlam = 0;
	bool bFirstWasSlam = false;
	bool bSawSweeping = false;
	for (float Time = 0.0f; Time < 10.0f; Time += SweepTest::StepSeconds)
	{
		const FCrowdStepResult Result = Sim.Step(SweepTest::StepSeconds, Wizard);
		if (Slams == 0 && Sweeps == 0 && Result.SlamsLanded > 0)
		{
			bFirstWasSlam = true;
		}
		Slams += Result.SlamsLanded;
		Sweeps += Result.SweepsLanded;
		SweepHits += Result.SweepsHitWizard;
		SweepsBeforeSecondSlam += Slams < 2 ? Result.SweepsLanded : 0;
		const FCrowdUnit* Unit = SweepTest::FirstOf(Sim, ECrowdKind::Brute);
		bSawSweeping |= Unit && Sim.IsSweeping(*Unit);
	}
	TestTrue(TEXT("its first attack is the slam"), bFirstWasSlam);
	TestTrue(TEXT("then it sweeps, more than once, before it slams again"), SweepsBeforeSecondSlam >= 2);
	TestTrue(TEXT("it slams again once a slam is ready"), Slams >= 2);
	TestTrue(TEXT("it is seen winding a sweep up"), bSawSweeping);
	TestEqual(TEXT("every sweep caught the wizard standing in front of it"), SweepHits, Sweeps);
	TestTrue(TEXT("a sweep does less than a slam"), Sim.Tuning.BruteSweepDamage < Sim.Tuning.BruteSlamDamage && Sim.Tuning.BruteSweepOverwhelm < Sim.Tuning.BruteSlamOverwhelm);

	// The sweep hurts the army in its arc and not behind it. (The slam is put out of the way: far from ready.)
	FCrowdSim Fight;
	CrowdTest::MakeEnemiesPlain(Fight);
	CrowdTest::FreezeArmy(Fight);
	Fight.Tuning.BruteSpeed = 0.0f;
	Fight.Tuning.SkeletonHealth = 1000.0f;
	Fight.Tuning.BruteSlamRadius = 1.0f;
	Fight.AddEnemy(ECrowdKind::Brute, CrowdTest::Origin);
	Fight.AddSkeleton(FVector2D(140.0, 30.0));
	Fight.AddSkeleton(FVector2D(-160.0, 0.0));
	int32 FightSweeps = 0;
	for (float Time = 0.0f; Time < 6.0f && FightSweeps == 0; Time += SweepTest::StepSeconds)
	{
		FightSweeps += Fight.Step(SweepTest::StepSeconds, FVector2D(3000.0, 0.0)).SweepsLanded;
	}
	TestEqual(TEXT("it swept"), FightSweeps, 1);
	float Front = 0.0f;
	float Behind = 0.0f;
	for (const FCrowdUnit& Unit : Fight.GetUnits())
	{
		if (Unit.Team == ECrowdTeam::Army)
		{
			(Unit.Position.X > 0.0 ? Front : Behind) = Unit.MaxHealth - Unit.Health;
		}
	}
	// (The wizard is far off, so the army is hurt more: the sweep's damage times what being away costs.)
	TestEqual(TEXT("the skeleton in front took the sweep"), Front, Fight.Tuning.BruteSweepDamage * Fight.Tuning.AwayDamageTaken, 0.01f);
	TestEqual(TEXT("the one behind took nothing"), Behind, 0.0f);

	// With no time between slams it only ever slams, as it did.
	FCrowdSim Old;
	CrowdTest::MakeEnemiesPlain(Old);
	Old.Tuning.BruteSpeed = 0.0f;
	Old.Tuning.BruteSlamEvery = 0.0f;
	Old.AddEnemy(ECrowdKind::Brute, CrowdTest::Origin);
	int32 OldSweeps = 0;
	int32 OldSlams = 0;
	for (float Time = 0.0f; Time < 8.0f; Time += SweepTest::StepSeconds)
	{
		const FCrowdStepResult Result = Old.Step(SweepTest::StepSeconds, Wizard);
		OldSweeps += Result.SweepsLanded;
		OldSlams += Result.SlamsLanded;
	}
	TestEqual(TEXT("no sweeps with BruteSlamEvery 0"), OldSweeps, 0);
	TestTrue(TEXT("only slams"), OldSlams >= 2);

	// The picture: its sweep is wound up over the wind-up of the sweep.
	TestEqual(TEXT("the sweep animation winds up as the sweep does"), ACrowdRenderer::MakeSwingTiming(Sim.Tuning, ECrowdKind::Brute, 1, 0.28f, 1.333f).BlowAt, Sim.Tuning.BruteSweepWindup, 0.001f);
	return true;
}

// Direction of 2026-11-15 01:00 PST: death montages "end up going into an A pose, then slowly sink"; sounds "that come
// from outside the player" are to "play at location so there is 3d to the audio".
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCorpseHolds, "WebTesting.Death.TheBodyHoldsItsLastFrame", WIZ_TEST_FLAGS)
bool FCorpseHolds::RunTest(const FString& Parameters)
{
	const float Length = 2.233f;
	TestEqual(TEXT("it starts at the start"), ACrowdRenderer::GetCorpseSeconds(0.0f, Length), 0.0f);
	TestEqual(TEXT("it plays at its own speed"), ACrowdRenderer::GetCorpseSeconds(1.0f, Length), 1.0f);
	const float Held = ACrowdRenderer::GetCorpseSeconds(Length, Length);
	TestTrue(TEXT("at its end it is just short of the end, never on or past it"), Held < Length && Held > Length - 0.1f);
	TestEqual(TEXT("and it stays there while it lies"), ACrowdRenderer::GetCorpseSeconds(Length + 2.0f, Length), Held);
	TestEqual(TEXT("and while it sinks"), ACrowdRenderer::GetCorpseSeconds(Length + ACrowdRenderer::CorpseRestSeconds + ACrowdRenderer::CorpseSinkSeconds, Length), Held);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlacedSounds, "WebTesting.Sound.SoundsFromOutsideThePlayerHaveAPlace", WIZ_TEST_FLAGS)
bool FPlacedSounds::RunTest(const FString& Parameters)
{
	for (const EWizardCue Cue : { EWizardCue::SwordHitsEnemy, EWizardCue::SwordHitsArmor, EWizardCue::SkeletonDies, EWizardCue::HammerHitsGround, EWizardCue::FireImpact,
		EWizardCue::DaggerMiss, EWizardCue::BowShoot, EWizardCue::BodyFallDirt1, EWizardCue::BodyFallDirt2, EWizardCue::BodyFallGrass1, EWizardCue::BodyFallGrass2 })
	{
		TestFalse(*FString::Printf(TEXT("%s is placed in the world"), FWizardSoundRules::Info(Cue).Name), FWizardSoundRules::IsPlayersOwn(Cue));
	}
	for (const EWizardCue Cue : { EWizardCue::Denied, EWizardCue::Blink, EWizardCue::Summoned, EWizardCue::FireThrow })
	{
		TestTrue(*FString::Printf(TEXT("%s is the player's own"), FWizardSoundRules::Info(Cue).Name), FWizardSoundRules::IsPlayersOwn(Cue));
	}
	TestEqual(TEXT("the refused click is on the interface channel"), FWizardSoundRules::ChannelOf(EWizardCue::Denied), 3);
	TestEqual(TEXT("a blow is on the effects channel"), FWizardSoundRules::ChannelOf(EWizardCue::SwordHitsEnemy), 2);
	TestTrue(TEXT("close to him a sound comes to the middle"), FWizardSoundRules::MiddleAt < FWizardSoundRules::MiddleFrom && FWizardSoundRules::MiddleFrom < FWizardSoundRules::NearDistance);
	return true;
}

#endif
