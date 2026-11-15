#include "Game/WizardHUD.h"
#include "Tests/WizTestFlags.h"

#if WITH_DEV_AUTOMATION_TESTS

// Direction of 2026-11-14 14:50 PST: "Build the final cut screen and the kingdom is open message", from the UI
// thread's hand-offs: the final cut's tally with a ledger of who was kept, cut and fell, its two buttons and its
// reveal; and the moment the commander dies (bars, banner, then where to go).

namespace FinalCutTest
{
	const float Sizes[][2] = { { 1920.0f, 1080.0f }, { 1280.0f, 720.0f }, { 640.0f, 480.0f }, { 3840.0f, 2160.0f } };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFinalCutLedger, "WebTesting.FinalCut.TheLedgerSplitsTheRaised", WIZ_TEST_FLAGS)
bool FFinalCutLedger::RunTest(const FString& Parameters)
{
	const FWizardLedger Ledger = AWizardHUD::FinalCutLedger(40, 12, 20);
	TestEqual(TEXT("kept: the army at the gate"), Ledger.Kept, 20);
	TestEqual(TEXT("cut: the sacrificed"), Ledger.Cut, 12);
	TestEqual(TEXT("fell: the rest of the raised"), Ledger.Fell, 8);
	TestEqual(TEXT("the three parts fill the bar"), Ledger.KeptShare + Ledger.CutShare + Ledger.FellShare, 1.0f, 0.001f);
	TestEqual(TEXT("each part is its share"), Ledger.KeptShare, 0.5f, 0.001f);

	// Bought units and ones a tester added stand at the gate without having been raised: nobody fell.
	const FWizardLedger More = AWizardHUD::FinalCutLedger(10, 4, 30);
	TestEqual(TEXT("fell is never below zero"), More.Fell, 0);
	TestEqual(TEXT("and the bar is still whole"), More.KeptShare + More.CutShare + More.FellShare, 1.0f, 0.001f);

	const FWizardLedger Empty = AWizardHUD::FinalCutLedger(0, 0, 0);
	TestEqual(TEXT("nothing raised: an empty bar, not a division by zero"), Empty.KeptShare + Empty.CutShare + Empty.FellShare, 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFinalCutReveal, "WebTesting.FinalCut.ThePartsComeInOneAfterAnother", WIZ_TEST_FLAGS)
bool FFinalCutReveal::RunTest(const FString& Parameters)
{
	for (int32 Part = 0; Part < AWizardHUD::FinalCutParts; ++Part)
	{
		TestEqual(TEXT("nothing shows while the screen darkens"), AWizardHUD::FinalCutPartAlpha(Part, 1.0f), 0.0f);
		TestEqual(TEXT("everything is shown by 3.6 seconds"), AWizardHUD::FinalCutPartAlpha(Part, 3.6f), 1.0f);
		if (Part > 0)
		{
			TestTrue(TEXT("a part never shows more than the one before it"),
				AWizardHUD::FinalCutPartAlpha(Part, 2.0f) <= AWizardHUD::FinalCutPartAlpha(Part - 1, 2.0f));
		}
	}
	TestTrue(TEXT("the label is in before the buttons start"), AWizardHUD::FinalCutPartAlpha(0, 2.0f) >= 1.0f && AWizardHUD::FinalCutPartAlpha(4, 2.0f) <= 0.0f);
	TestEqual(TEXT("the numbers count up from nothing"), AWizardHUD::FinalCutCount(40, 2.3f), 0);
	TestEqual(TEXT("to their value"), AWizardHUD::FinalCutCount(40, 3.4f), 40);
	const int32 Midway = AWizardHUD::FinalCutCount(40, 2.8f);
	TestTrue(TEXT("passing through the numbers between"), Midway > 0 && Midway < 40);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFinalCutButtons, "WebTesting.FinalCut.ItsButtons", WIZ_TEST_FLAGS)
bool FFinalCutButtons::RunTest(const FString& Parameters)
{
	for (const float* Size : FinalCutTest::Sizes)
	{
		const float Width = Size[0];
		const float Height = Size[1];
		const FBox2D Again = AWizardHUD::FinalCutButtonBox(0, Width, Height);
		const FBox2D Locked = AWizardHUD::FinalCutButtonBox(1, Width, Height);
		TestTrue(TEXT("March again is on the screen"), Again.Min.X >= 0.0 && Again.Min.Y >= 0.0 && Again.Max.X <= Width && Again.Max.Y <= Height);
		TestTrue(TEXT("Enter the kingdom is on the screen"), Locked.Min.X >= 0.0 && Locked.Min.Y >= 0.0 && Locked.Max.X <= Width && Locked.Max.Y <= Height);
		TestTrue(TEXT("side by side, not on one another"), Locked.Min.X >= Again.Max.X && FMath::IsNearlyEqual(Locked.Min.Y, Again.Min.Y, 1.0));
		TestTrue(TEXT("the pair is in the middle"), FMath::IsNearlyEqual((Again.Min.X + Locked.Max.X) * 0.5, Width * 0.5, 2.0));
		TestEqual(TEXT("a click on March again"), AWizardHUD::FinalCutButtonAt(Again.GetCenter(), Width, Height), 0);
		TestEqual(TEXT("a click on Enter the kingdom"), AWizardHUD::FinalCutButtonAt(Locked.GetCenter(), Width, Height), 1);
		TestEqual(TEXT("a click on the title hits nothing"), AWizardHUD::FinalCutButtonAt(FVector2D(Width * 0.5, Height * 0.2), Width, Height), static_cast<int32>(INDEX_NONE));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGateMessageTimes, "WebTesting.Gate.TheMessageThenWhereToGo", WIZ_TEST_FLAGS)
bool FGateMessageTimes::RunTest(const FString& Parameters)
{
	// Changed 2026-11-14 15:05 PST (the design's version 4, from the director through the UI thread): no bars, and
	// the banner is in at 0.3 s, fully shown from 0.8 s to 3.8 s, gone by 4.3 s; where to go follows from 4.4 s.
	TestEqual(TEXT("no banner at the kill"), AWizardHUD::GateBannerAlpha(0.2f), 0.0f);
	TestTrue(TEXT("it fades in"), AWizardHUD::GateBannerAlpha(0.55f) > 0.0f && AWizardHUD::GateBannerAlpha(0.55f) < 1.0f);
	TestEqual(TEXT("the banner is fully shown from 0.8 s"), AWizardHUD::GateBannerAlpha(0.81f), 1.0f);
	TestEqual(TEXT("for three seconds"), AWizardHUD::GateBannerAlpha(3.79f), 1.0f);
	TestTrue(TEXT("then it fades"), AWizardHUD::GateBannerAlpha(4.05f) > 0.0f && AWizardHUD::GateBannerAlpha(4.05f) < 1.0f);
	TestEqual(TEXT("and is gone"), AWizardHUD::GateBannerAlpha(4.31f), 0.0f);

	TestEqual(TEXT("where to go is not said under the banner"), AWizardHUD::GateObjectiveAlpha(4.3f), 0.0f);
	for (float Seconds = 0.0f; Seconds < 8.0f; Seconds += 0.05f)
	{
		TestFalse(TEXT("the two never show together"), AWizardHUD::GateBannerAlpha(Seconds) > 0.0f && AWizardHUD::GateObjectiveAlpha(Seconds) > 0.0f);
	}
	TestEqual(TEXT("it is said once the banner has gone"), AWizardHUD::GateObjectiveAlpha(5.1f), 1.0f);
	TestEqual(TEXT("and stays"), AWizardHUD::GateObjectiveAlpha(600.0f), 1.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGatePointer, "WebTesting.Gate.TheArrowAtTheScreenEdge", WIZ_TEST_FLAGS)
bool FGatePointer::RunTest(const FString& Parameters)
{
	const float Width = 1920.0f;
	const float Height = 1080.0f;
	FVector2D Point;
	FVector2D Direction;
	TestFalse(TEXT("a gate in view needs no arrow"), AWizardHUD::GateEdgeArrow(FVector2D(1200.0, 500.0), true, Width, Height, Point, Direction));

	TestTrue(TEXT("a gate off to the right needs one"), AWizardHUD::GateEdgeArrow(FVector2D(5000.0, 540.0), true, Width, Height, Point, Direction));
	TestEqual(TEXT("at the right, 70 in from the edge"), static_cast<float>(Point.X), Width - 70.0f, 1.0f);
	TestEqual(TEXT("level with the middle"), static_cast<float>(Point.Y), 540.0f, 1.0f);
	TestTrue(TEXT("pointing right"), Direction.X > 0.99);

	TestTrue(TEXT("a gate far above the screen"), AWizardHUD::GateEdgeArrow(FVector2D(960.0, -4000.0), true, Width, Height, Point, Direction));
	// Changed after the first screenshot: at 150 down (the design's number) the arrow sat on the line that says
	// where to go, which ends at 148. It is 200 down now.
	TestEqual(TEXT("at the top, 200 down, clear of the HUD and the line that says where to go"), static_cast<float>(Point.Y), 200.0f, 1.0f);
	TestTrue(TEXT("pointing up"), Direction.Y < -0.99);

	TestTrue(TEXT("a gate up and to the left"), AWizardHUD::GateEdgeArrow(FVector2D(-3000.0, -3000.0), true, Width, Height, Point, Direction));
	TestTrue(TEXT("the arrow stays inside its box"), Point.X >= 69.0 && Point.X <= Width - 69.0 && Point.Y >= 199.0 && Point.Y <= Height - 199.0);
	TestTrue(TEXT("pointing up and left"), Direction.X < 0.0 && Direction.Y < 0.0);

	// Behind the camera a projected point is mirrored: the arrow points the other way from it.
	TestTrue(TEXT("a gate behind the camera always needs one"), AWizardHUD::GateEdgeArrow(FVector2D(1200.0, 540.0), false, Width, Height, Point, Direction));
	TestTrue(TEXT("pointing away from the mirrored point"), Direction.X < 0.0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
