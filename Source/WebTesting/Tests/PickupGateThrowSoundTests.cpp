#include "Game/WizardSounds.h"
#include "Sim/CrowdSim.h"
#include "Sound/SoundBase.h"
#include "Tests/CrowdTestHelpers.h"
#include "Tests/WizTestFlags.h"

#if WITH_DEV_AUTOMATION_TESTS

// Direction of 2026-11-15 02:28 PST: "Bone-Pickup.wav for everytime a bone pile is picked up. Gate-Open.wav for when
// the gate opens. Commander-Spear-Throw.wav for when the commander throws their spear."

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoundBonePicked, "WebTesting.Sound.EveryBonePilePickedUpIsReported", WIZ_TEST_FLAGS)
bool FSoundBonePicked::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	// Two piles under the wizard, one far from him.
	TestTrue(TEXT("a pile"), Sim.DropBone(FVector2D(10.0, 0.0), 3));
	TestTrue(TEXT("another"), Sim.DropBone(FVector2D(-10.0, 5.0), 2));
	TestTrue(TEXT("one far away"), Sim.DropBone(FVector2D(4000.0, 0.0), 1));
	int32 Reported = 0;
	int32 Collected = 0;
	for (int32 Step = 0; Step < 10; ++Step)
	{
		Collected += Sim.Step(0.05f, CrowdTest::Origin).BonesCollected;
		for (const FCrowdEvent& Event : Sim.GetEvents())
		{
			if (Event.Type == ECrowdEvent::BonePicked)
			{
				++Reported;
				TestTrue(TEXT("it is reported where the pile lay"), Event.Position.Size() < 50.0);
			}
		}
	}
	TestEqual(TEXT("the bones of the two piles are his"), Collected, 5);
	TestEqual(TEXT("each pile picked up is reported, once"), Reported, 2);
	TestEqual(TEXT("the far pile still lies"), Sim.GetBones().Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoundSpearThrown, "WebTesting.Sound.TheCommandersThrowIsReported", WIZ_TEST_FLAGS)
bool FSoundSpearThrown::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	CrowdTest::MakeEnemiesPlain(Sim);
	Sim.Tuning.CommanderSpeed = 0.0f;
	Sim.Tuning.CommanderSpearRange = 5000.0f;
	const FVector2D Post(800.0, 0.0);
	Sim.AddEnemy(ECrowdKind::Commander, Post);
	int32 Thrown = 0;
	int32 Reported = 0;
	for (float Time = 0.0f; Time < 12.0f; Time += 0.05f)
	{
		const int32 Now = Sim.Step(0.05f, CrowdTest::Origin).JavelinsThrown;
		int32 Events = 0;
		for (const FCrowdEvent& Event : Sim.GetEvents())
		{
			if (Event.Type == ECrowdEvent::SpearThrown)
			{
				++Events;
				TestEqual(TEXT("it is the commander's"), static_cast<int32>(Event.Kind), static_cast<int32>(ECrowdKind::Commander));
				TestTrue(TEXT("from where he stands"), FVector2D::Distance(Event.Position, Post) < 5.0);
			}
		}
		TestEqual(TEXT("reported in the step the spear leaves his hand"), Events, Now);
		Thrown += Now;
		Reported += Events;
	}
	TestTrue(TEXT("he threw"), Thrown >= 1);
	TestEqual(TEXT("every throw is reported"), Reported, Thrown);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoundNewThree, "WebTesting.Sound.ThePickupTheGateAndTheThrowHaveTheirSounds", WIZ_TEST_FLAGS)
bool FSoundNewThree::RunTest(const FString& Parameters)
{
	const struct { EWizardCue Cue; const TCHAR* Name; const TCHAR* Asset; } Rows[] = {
		{ EWizardCue::BonePickup, TEXT("bonepickup"), TEXT("SFX_Bone_Pickup") },
		{ EWizardCue::GateOpen, TEXT("gateopen"), TEXT("SFX_Gate_Open") },
		{ EWizardCue::CommanderSpearThrow, TEXT("commanderspearthrow"), TEXT("SFX_Commander_Spear_Throw") },
	};
	for (const auto& Row : Rows)
	{
		const FWizardCueInfo& Info = FWizardSoundRules::Info(Row.Cue);
		TestEqual(Row.Name, FString(Info.Name), FString(Row.Name));
		TestTrue(Row.Asset, FString(Info.Path).Contains(Row.Asset));
		TestNotNull(Row.Asset, LoadObject<USoundBase>(nullptr, Info.Path));
		TestEqual(TEXT("its name finds it"), static_cast<int32>(FWizardSoundRules::CueByName(Row.Name)), static_cast<int32>(Row.Cue));
		TestEqual(TEXT("on the effects channel"), FWizardSoundRules::ChannelOf(Row.Cue), 2);
	}
	// A pile is picked up under the wizard's feet: his own. The gate is heard wherever he stands (it tells him the
	// way is open). The throw comes from where the commander is.
	TestTrue(TEXT("the pickup is the player's own"), FWizardSoundRules::IsPlayersOwn(EWizardCue::BonePickup));
	TestTrue(TEXT("the gate is heard wherever he stands"), FWizardSoundRules::IsPlayersOwn(EWizardCue::GateOpen));
	TestFalse(TEXT("the throw has a place"), FWizardSoundRules::IsPlayersOwn(EWizardCue::CommanderSpearThrow));
	// Many piles walked over at once are not one sound each on top of the other, and not one sound for all either.
	const float Gap = FWizardSoundRules::Info(EWizardCue::BonePickup).MinGap;
	TestTrue(TEXT("piles close together are each heard"), Gap > 0.0f && Gap <= 0.1f);
	return true;
}

#endif
