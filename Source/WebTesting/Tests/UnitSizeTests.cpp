#include "Sim/CrowdSim.h"
#include "Tests/WizTestFlags.h"

#if WITH_DEV_AUTOMATION_TESTS

// Direction of 2026-11-13: beside the wizard, low-tier units (his skeletons and enemy grunts) are small, a brute is
// larger but still smaller than him, and a commander is his size or larger.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnitSizeOrder, "WebTesting.UnitSize.OrderBesideTheWizard", WIZ_TEST_FLAGS)
bool FUnitSizeOrder::RunTest(const FString& Parameters)
{
	const FCrowdSim Sim;
	const float Wizard = 220.0f;
	const float Skeleton = Sim.GetUnitHeight(ECrowdKind::Skeleton, Wizard);
	const float Grunt = Sim.GetUnitHeight(ECrowdKind::Grunt, Wizard);
	const float Guard = Sim.GetUnitHeight(ECrowdKind::Guard, Wizard);
	const float Brute = Sim.GetUnitHeight(ECrowdKind::Brute, Wizard);
	const float Commander = Sim.GetUnitHeight(ECrowdKind::Commander, Wizard);
	// The director's table of 2026-11-13 15:55 PST replaces the first sizes (low tier under half of him, a brute under
	// him): the skeletons were too small. Heights in units beside a wizard of 220.
	TestEqual(TEXT("a plain skeleton is 150, about two thirds of him"), Skeleton, 150.0f, 1.0f);
	TestEqual(TEXT("a Guard is 175, about four fifths"), Guard, 175.0f, 1.0f);
	TestEqual(TEXT("a grunt is 170, close to a Guard"), Grunt, 170.0f, 1.0f);
	TestEqual(TEXT("a brute is 240, taller than the wizard"), Brute, 240.0f, 1.0f);
	TestTrue(TEXT("a commander is taller than a brute"), Commander > Brute);
	const float Archer = Sim.GetUnitHeight(ECrowdKind::Archer, Wizard);
	const float Assassin = Sim.GetUnitHeight(ECrowdKind::Assassin, Wizard);
	TestTrue(TEXT("an archer is about a grunt's size"), Archer >= 150.0f && Archer <= 180.0f);
	TestTrue(TEXT("an assassin is about a grunt's size"), Assassin >= 150.0f && Assassin <= 180.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnitSizeFollows, "WebTesting.UnitSize.FollowsTheWizardAndTheTuning", WIZ_TEST_FLAGS)
bool FUnitSizeFollows::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	TestEqual(TEXT("a wizard twice as tall has grunts twice as tall"),
		Sim.GetUnitHeight(ECrowdKind::Grunt, 440.0f), Sim.GetUnitHeight(ECrowdKind::Grunt, 220.0f) * 2.0f, 0.01f);
	Sim.Tuning.BruteSize = 0.75f;
	TestEqual(TEXT("the brute's share comes from the tuning"), Sim.GetUnitHeight(ECrowdKind::Brute, 200.0f), 150.0f, 0.01f);
	return true;
}

#endif
