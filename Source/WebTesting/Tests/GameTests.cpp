#include "Game/CrowdSubsystem.h"
#include "Game/WizardHUD.h"
#include "Game/WizardPawn.h"
#include "Tests/WizTestFlags.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardCameraModeTargets, "WebTesting.Camera.ModeTargets", WIZ_TEST_FLAGS)
bool FWizardCameraModeTargets::RunTest(const FString& Parameters)
{
	float Pitch = 0.0f;
	float ArmLength = 0.0f;

	AWizardPawn::GetCameraTarget(EWizardCameraMode::Angled, Pitch, ArmLength);
	TestEqual(TEXT("Angled pitch"), Pitch, -55.0f);
	TestEqual(TEXT("Angled arm length"), ArmLength, 1600.0f);

	AWizardPawn::GetCameraTarget(EWizardCameraMode::TopDown, Pitch, ArmLength);
	TestEqual(TEXT("Top Down pitch"), Pitch, -89.0f);
	TestEqual(TEXT("Top Down arm length"), ArmLength, 2000.0f);

	AWizardPawn::GetCameraTarget(EWizardCameraMode::ThirdPerson, Pitch, ArmLength);
	TestEqual(TEXT("Third Person pitch"), Pitch, -15.0f);
	TestEqual(TEXT("Third Person arm length"), ArmLength, 500.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardHudEdgeFromDirection, "WebTesting.Hud.EdgeFromDirection", WIZ_TEST_FLAGS)
bool FWizardHudEdgeFromDirection::RunTest(const FString& Parameters)
{
	// Yaw 0: the camera looks along +X, so +X is the top of the screen and +Y is the right.
	FWizardEdgeWeights Weights = AWizardHUD::EdgeWeights(FVector2D(1.0, 0.0), 0.0f);
	TestTrue(TEXT("+X is the top edge"), FMath::IsNearlyEqual(Weights.Top, 1.0f, 0.001f));
	TestTrue(TEXT("+X leaves the other edges clear"), Weights.Bottom < 0.001f && Weights.Left < 0.001f && Weights.Right < 0.001f);

	Weights = AWizardHUD::EdgeWeights(FVector2D(0.0, 1.0), 0.0f);
	TestTrue(TEXT("+Y is the right edge"), FMath::IsNearlyEqual(Weights.Right, 1.0f, 0.001f));
	TestTrue(TEXT("+Y leaves the other edges clear"), Weights.Top < 0.001f && Weights.Bottom < 0.001f && Weights.Left < 0.001f);

	Weights = AWizardHUD::EdgeWeights(FVector2D(-1.0, 0.0), 0.0f);
	TestTrue(TEXT("-X is the bottom edge"), FMath::IsNearlyEqual(Weights.Bottom, 1.0f, 0.001f));

	Weights = AWizardHUD::EdgeWeights(FVector2D(0.0, 1.0), 90.0f);
	TestTrue(TEXT("with the camera turned 90 degrees, +Y is the top edge"), FMath::IsNearlyEqual(Weights.Top, 1.0f, 0.001f));

	Weights = AWizardHUD::EdgeWeights(FVector2D::ZeroVector, 0.0f);
	TestTrue(TEXT("no direction, no edge"), Weights.Top == 0.0f && Weights.Bottom == 0.0f && Weights.Left == 0.0f && Weights.Right == 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardWavesSizeGrows, "WebTesting.Waves.SizeGrows", WIZ_TEST_FLAGS)
bool FWizardWavesSizeGrows::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("first wave, lowest roll"), UCrowdSubsystem::WaveSize(0.0f, 0), 4);
	TestEqual(TEXT("first wave, highest roll"), UCrowdSubsystem::WaveSize(0.0f, 4), 8);
	TestEqual(TEXT("after 30 seconds"), UCrowdSubsystem::WaveSize(30.0f, 0), 5);
	TestEqual(TEXT("after 60 seconds"), UCrowdSubsystem::WaveSize(60.0f, 0), 6);
	TestEqual(TEXT("a roll out of range is clamped"), UCrowdSubsystem::WaveSize(0.0f, 99), 8);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardDebugParseCameraMode, "WebTesting.Debug.ParseCameraMode", WIZ_TEST_FLAGS)
bool FWizardDebugParseCameraMode::RunTest(const FString& Parameters)
{
	EWizardCameraMode Mode = EWizardCameraMode::Angled;

	TestTrue(TEXT("TopDown is accepted"), AWizardPawn::ParseCameraMode(TEXT("TopDown"), Mode));
	TestTrue(TEXT("TopDown parses to Top Down"), Mode == EWizardCameraMode::TopDown);

	TestTrue(TEXT("Angled is accepted"), AWizardPawn::ParseCameraMode(TEXT("Angled"), Mode));
	TestTrue(TEXT("Angled parses to Angled"), Mode == EWizardCameraMode::Angled);

	TestTrue(TEXT("lower case is accepted"), AWizardPawn::ParseCameraMode(TEXT("topdown"), Mode));
	TestTrue(TEXT("topdown parses to Top Down"), Mode == EWizardCameraMode::TopDown);

	TestTrue(TEXT("ThirdPerson is accepted"), AWizardPawn::ParseCameraMode(TEXT("ThirdPerson"), Mode));
	TestTrue(TEXT("ThirdPerson parses to Third Person"), Mode == EWizardCameraMode::ThirdPerson);

	TestFalse(TEXT("Sideways is rejected"), AWizardPawn::ParseCameraMode(TEXT("Sideways"), Mode));
	TestTrue(TEXT("a rejected name leaves the mode alone"), Mode == EWizardCameraMode::ThirdPerson);
	TestFalse(TEXT("empty text is rejected"), AWizardPawn::ParseCameraMode(TEXT(""), Mode));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardHudLayoutScale, "WebTesting.Hud.LayoutScale", WIZ_TEST_FLAGS)
bool FWizardHudLayoutScale::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("1920x1080 is the base size"), FMath::IsNearlyEqual(AWizardHUD::LayoutScale(1920.0f, 1080.0f), 1.0f, 0.001f));
	TestTrue(TEXT("4K is twice the base size"), FMath::IsNearlyEqual(AWizardHUD::LayoutScale(3840.0f, 2160.0f), 2.0f, 0.001f));
	TestTrue(TEXT("1280x720 is two thirds"), FMath::IsNearlyEqual(AWizardHUD::LayoutScale(1280.0f, 720.0f), 2.0f / 3.0f, 0.001f));
	TestTrue(TEXT("640x480 stops at the floor so text stays readable"), FMath::IsNearlyEqual(AWizardHUD::LayoutScale(640.0f, 480.0f), 0.5f, 0.001f));
	TestTrue(TEXT("a narrow window scales by its width"), FMath::IsNearlyEqual(AWizardHUD::LayoutScale(1440.0f, 1080.0f), 0.75f, 0.001f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardDebugTuningByName, "WebTesting.Debug.TuningByName", WIZ_TEST_FLAGS)
bool FWizardDebugTuningByName::RunTest(const FString& Parameters)
{
	FCrowdTuning Tuning;
	float* Leash = FindCrowdTuningValue(Tuning, TEXT("leashradius"));
	if (TestNotNull(TEXT("LeashRadius is found in any letter case"), Leash))
	{
		*Leash = 640.0f;
		TestEqual(TEXT("the found value is the one in the tuning"), Tuning.LeashRadius, 640.0f);
	}
	TestNull(TEXT("an unknown name finds nothing"), FindCrowdTuningValue(Tuning, TEXT("NoSuchValue")));
	TestTrue(TEXT("the list of names is not empty"), GetCrowdTuningEntries().Num() >= 20);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
