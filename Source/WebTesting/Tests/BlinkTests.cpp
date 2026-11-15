#include "Sim/BlinkState.h"
#include "Sim/OverwhelmMeter.h"
#include "Tests/CrowdTestHelpers.h"
#include "Tests/WizTestFlags.h"

#if WITH_DEV_AUTOMATION_TESTS

// Direction of 2026-11-13 (changed 12:00 PST): Blink goes the way the wizard is moving, not toward the mouse, and a
// direction held during it bends it to that side. It takes half a second, nothing can hurt him during it, and its
// cooldown starts when it ends. The earlier tests of "ends on the aim point" went with that rule.

namespace BlinkTest
{
	const TArray<FCrowdObstacle> NoWalls;

	/** Runs a whole blink in 50 steps, holding Steer throughout. */
	void Run(FBlinkState& Blink, FVector2D Steer = FVector2D::ZeroVector, TConstArrayView<FCrowdObstacle> Walls = NoWalls)
	{
		for (int32 Index = 0; Index < 50 && Blink.IsActive(); ++Index)
		{
			Blink.Step(0.01f, 0.5f, 3.0f, Steer, 0.35f, Walls, 38.0f);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBlinkHeldDirection, "WebTesting.Blink.GoesTheFullRangeTheWayHeIsMoving", WIZ_TEST_FLAGS)
bool FBlinkHeldDirection::RunTest(const FString& Parameters)
{
	FBlinkState Blink;
	TestFalse(TEXT("it does not start with no direction"), Blink.Start(FVector2D(100.0, 100.0), FVector2D::ZeroVector, 700.0f));
	TestTrue(TEXT("it starts to the left"), Blink.Start(FVector2D(100.0, 100.0), FVector2D(0.0, -3.0), 700.0f));
	BlinkTest::Run(Blink);
	TestFalse(TEXT("it has ended"), Blink.IsActive());
	TestTrue(TEXT("it went the full range to the left"), Blink.GetPosition().Equals(FVector2D(100.0, -600.0), 1.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBlinkSteer, "WebTesting.Blink.AHeldDirectionBendsIt", WIZ_TEST_FLAGS)
bool FBlinkSteer::RunTest(const FString& Parameters)
{
	FBlinkState Blink;
	Blink.Start(FVector2D::ZeroVector, FVector2D(0.0, -1.0), 700.0f);
	BlinkTest::Run(Blink, FVector2D(1.0, 0.0));
	TestTrue(TEXT("holding forward moves it forward"), Blink.GetPosition().X > 150.0);
	TestTrue(TEXT("but less than it moves along the blink"), Blink.GetPosition().X < 350.0);
	TestTrue(TEXT("it still goes the full range to the left"), FMath::IsNearlyEqual(Blink.GetPosition().Y, -700.0, 1.0));

	FBlinkState Back;
	Back.Start(FVector2D::ZeroVector, FVector2D(0.0, -1.0), 700.0f);
	BlinkTest::Run(Back, FVector2D(0.0, 1.0));
	TestTrue(TEXT("holding the opposite way does not shorten it"), Back.GetPosition().Equals(FVector2D(0.0, -700.0), 1.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBlinkEndAtWall, "WebTesting.Blink.StopsAtAWallAndWater", WIZ_TEST_FLAGS)
bool FBlinkEndAtWall::RunTest(const FString& Parameters)
{
	TArray<FCrowdObstacle> Walls = { CrowdTest::Wall(350.0, 0.0, 50.0, 1000.0) };
	FBlinkState Blink;
	Blink.Start(FVector2D::ZeroVector, FVector2D(1.0, 0.0), 700.0f);
	BlinkTest::Run(Blink, FVector2D::ZeroVector, Walls);
	TestTrue(TEXT("it moves toward the wall"), Blink.GetPosition().X > 150.0);
	TestTrue(TEXT("it stops on the near side, clear of the wall"), Blink.GetPosition().X <= 262.0);

	Walls[0].bLow = true;
	FBlinkState OverWater;
	OverWater.Start(FVector2D::ZeroVector, FVector2D(1.0, 0.0), 700.0f);
	BlinkTest::Run(OverWater, FVector2D::ZeroVector, Walls);
	TestTrue(TEXT("water stops it too"), OverWater.GetPosition().X > 150.0 && OverWater.GetPosition().X <= 262.0);

	// Against a wall at an angle he slides along it.
	Walls[0].bLow = false;
	FBlinkState Slide;
	Slide.Start(FVector2D::ZeroVector, FVector2D(1.0, 1.0), 700.0f);
	BlinkTest::Run(Slide, FVector2D::ZeroVector, Walls);
	TestTrue(TEXT("he does not pass the wall"), Slide.GetPosition().X <= 262.0);
	TestTrue(TEXT("he slides along it"), Slide.GetPosition().Y > 300.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBlinkTiming, "WebTesting.Blink.HalfASecondThenTheCooldown", WIZ_TEST_FLAGS)
bool FBlinkTiming::RunTest(const FString& Parameters)
{
	const FVector2D None = FVector2D::ZeroVector;
	FBlinkState Blink;
	TestTrue(TEXT("a fresh blink can start"), Blink.CanStart());
	TestTrue(TEXT("it starts"), Blink.Start(None, FVector2D(1.0, 0.0), 700.0f));
	TestTrue(TEXT("it is under way"), Blink.IsActive());
	TestFalse(TEXT("a second one cannot start during it"), Blink.Start(None, FVector2D(0.0, 1.0), 700.0f));
	TestFalse(TEXT("0.4 seconds in, it has not ended"), Blink.Step(0.4f, 0.5f, 3.0f, None, 0.35f, BlinkTest::NoWalls, 38.0f));
	TestTrue(TEXT("still under way"), Blink.IsActive());
	TestTrue(TEXT("it ends once 0.5 seconds have passed"), Blink.Step(0.1f, 0.5f, 3.0f, None, 0.35f, BlinkTest::NoWalls, 38.0f));
	TestFalse(TEXT("it is over"), Blink.IsActive());
	TestEqual(TEXT("the cooldown starts when it ends"), Blink.GetCooldown(), 3.0f, 0.001f);
	TestFalse(TEXT("it cannot start during the cooldown"), Blink.Start(None, FVector2D(1.0, 0.0), 700.0f));
	Blink.Step(2.9f, 0.5f, 3.0f, None, 0.35f, BlinkTest::NoWalls, 38.0f);
	TestFalse(TEXT("not yet ready after 2.9 seconds"), Blink.CanStart());
	Blink.Step(0.2f, 0.5f, 3.0f, None, 0.35f, BlinkTest::NoWalls, 38.0f);
	TestTrue(TEXT("ready after the cooldown"), Blink.CanStart());
	TestTrue(TEXT("it starts again"), Blink.Start(None, FVector2D(1.0, 0.0), 700.0f));
	return true;
}

// Direction of 2026-11-13 (14:30 PST): the feel of the Dash in the director's Nexus project, a burst that starts at
// full speed. The blink starts fast and slows to a stop, takes a little longer, and can be steered more, and as
// much at its end as at its start. The test of "eased in and out, half the time is half the way" went with the old rule.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBlinkGlide, "WebTesting.Blink.StartsFastAndSlowsToAStop", WIZ_TEST_FLAGS)
bool FBlinkGlide::RunTest(const FString& Parameters)
{
	const FVector2D None = FVector2D::ZeroVector;
	FBlinkState Blink;
	Blink.Start(FVector2D(100.0, 0.0), FVector2D(1.0, 0.0), 600.0f);
	TestTrue(TEXT("it begins where he stood"), Blink.GetPosition().Equals(FVector2D(100.0, 0.0), 0.01));
	Blink.Step(0.125f, 0.5f, 3.0f, None, 0.35f, BlinkTest::NoWalls, 38.0f);
	TestTrue(TEXT("a quarter of the time covers over two fifths of the way (a burst)"), Blink.GetPosition().X > 100.0 + 0.4 * 600.0);
	Blink.Step(0.125f, 0.5f, 3.0f, None, 0.35f, BlinkTest::NoWalls, 38.0f);
	TestTrue(TEXT("half the time is well over half the way"), Blink.GetPosition().X > 100.0 + 0.7 * 600.0);
	Blink.Step(0.25f, 0.5f, 3.0f, None, 0.35f, BlinkTest::NoWalls, 38.0f);
	TestTrue(TEXT("it ends at the full range"), Blink.GetPosition().Equals(FVector2D(700.0, 0.0), 0.5));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBlinkLateSteer, "WebTesting.Blink.CanBeSteeredAsMuchAtTheEndAsAtTheStart", WIZ_TEST_FLAGS)
bool FBlinkLateSteer::RunTest(const FString& Parameters)
{
	const FVector2D None = FVector2D::ZeroVector;
	const FVector2D Up(0.0, 1.0);
	FBlinkState Blink;
	Blink.Start(FVector2D::ZeroVector, FVector2D(1.0, 0.0), 700.0f);
	Blink.Step(0.4f, 0.5f, 3.0f, None, 0.5f, BlinkTest::NoWalls, 38.0f);
	TestEqual(TEXT("nothing held, no sideways move"), Blink.GetPosition().Y, 0.0, 0.01);
	// Held for the last fifth of the time: a fifth of the whole sideways reach (0.5 of 700).
	Blink.Step(0.1f, 0.5f, 3.0f, Up, 0.5f, BlinkTest::NoWalls, 38.0f);
	TestEqual(TEXT("the last fifth of the time gives a fifth of the sideways reach"), Blink.GetPosition().Y, 70.0, 2.0);
	TestEqual(TEXT("and the blink still went its full length"), Blink.GetPosition().X, 700.0, 0.5);

	const FCrowdTuning Tuning;
	TestTrue(TEXT("the blink takes a little longer than half a second"), Tuning.BlinkSeconds > 0.55f && Tuning.BlinkSeconds <= 0.8f);
	TestTrue(TEXT("and steers more than before (0.35)"), Tuning.BlinkSteer >= 0.5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBlinkInvulnerable, "WebTesting.Blink.NothingFillsTheMeterDuringIt", WIZ_TEST_FLAGS)
bool FBlinkInvulnerable::RunTest(const FString& Parameters)
{
	TArray<FVector2D> Five;
	Five.Init(FVector2D(50.0, 0.0), 5);
	FOverwhelmMeter Meter;
	Meter.Step(1.0f, Five);
	TestEqual(TEXT("five enemies for a second fill 0.4"), Meter.GetValue(), 0.4f, 0.001f);
	Meter.bInvulnerable = true;
	TestFalse(TEXT("he does not fall while invulnerable"), Meter.Step(10.0f, Five));
	TestFalse(TEXT("an arrow adds nothing while invulnerable"), Meter.Add(0.5f));
	TestEqual(TEXT("the meter holds during the blink"), Meter.GetValue(), 0.4f, 0.001f);
	Meter.bInvulnerable = false;
	Meter.Step(1.0f, Five);
	TestEqual(TEXT("it fills again afterwards"), Meter.GetValue(), 0.8f, 0.001f);
	return true;
}

#endif
