#include "Game/WizardPawn.h"
#include "Tests/WizTestFlags.h"

#if WITH_DEV_AUTOMATION_TESTS

// Direction of 2026-11-13: every control has exactly one key. Space is Blink, F commands the army and C recalls it,
// so the shrine moved to E, the spear lost E, and Summon lost Space. (The check that F was kept free went when
// command mode was built.)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKeyBindingOneEach, "WebTesting.Keys.OneKeyForEachControl", WIZ_TEST_FLAGS)
bool FKeyBindingOneEach::RunTest(const FString& Parameters)
{
	TSet<FName> Actions;
	TSet<FKey> Keys;
	for (const FWizardKeyBinding& Binding : AWizardPawn::GetKeyBindings())
	{
		TestTrue(*FString::Printf(TEXT("%s has a real key"), *Binding.Action.ToString()), Binding.Key.IsValid());
		TestFalse(*FString::Printf(TEXT("%s has only one key"), *Binding.Action.ToString()), Actions.Contains(Binding.Action));
		TestFalse(*FString::Printf(TEXT("%s works only one control"), *Binding.Key.ToString()), Keys.Contains(Binding.Key));
		Actions.Add(Binding.Action);
		Keys.Add(Binding.Key);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKeyBindingChosen, "WebTesting.Keys.TheChosenKeys", WIZ_TEST_FLAGS)
bool FKeyBindingChosen::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Space blinks"), AWizardPawn::GetKey(TEXT("Blink")) == EKeys::SpaceBar);
	TestTrue(TEXT("Q summons"), AWizardPawn::GetKey(TEXT("Summon")) == EKeys::Q);
	TestTrue(TEXT("E uses the shrine"), AWizardPawn::GetKey(TEXT("Shrine")) == EKeys::E);
	TestTrue(TEXT("the right mouse button throws the spear"), AWizardPawn::GetKey(TEXT("Spear")) == EKeys::RightMouseButton);
	TestTrue(TEXT("the left mouse button fires Bolt"), AWizardPawn::GetKey(TEXT("Bolt")) == EKeys::LeftMouseButton);
	TestTrue(TEXT("P pauses"), AWizardPawn::GetKey(TEXT("Pause")) == EKeys::P);
	TestTrue(TEXT("F commands the army"), AWizardPawn::GetKey(TEXT("Command")) == EKeys::F);
	TestTrue(TEXT("C recalls it"), AWizardPawn::GetKey(TEXT("Recall")) == EKeys::C);
	TestTrue(TEXT("the mouse wheel sets the number"), AWizardPawn::GetKey(TEXT("Command Count")) == EKeys::MouseWheelAxis);
	TestTrue(TEXT("the debug overlay has a key in every build"), AWizardPawn::GetKey(TEXT("Debug Overlay")) == EKeys::H);
	TestTrue(TEXT("the cameras have keys in every build"), AWizardPawn::GetKey(TEXT("Camera Top Down")) == EKeys::Two);
	return true;
}

#endif
