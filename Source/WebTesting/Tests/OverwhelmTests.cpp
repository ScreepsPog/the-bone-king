#include "Sim/OverwhelmMeter.h"
#include "Tests/WizTestFlags.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace OverwhelmTest
{
	TArray<FVector2D> Offsets(int32 Count, FVector2D Offset = FVector2D(50.0, 0.0))
	{
		TArray<FVector2D> Result;
		Result.Init(Offset, Count);
		return Result;
	}

	const TArray<FVector2D> None;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOverwhelmFillsPerEnemy, "WebTesting.Overwhelm.FillsPerEnemy", WIZ_TEST_FLAGS)
bool FOverwhelmFillsPerEnemy::RunTest(const FString& Parameters)
{
	FOverwhelmMeter Meter;
	Meter.Step(1.0f, OverwhelmTest::Offsets(5));
	TestTrue(TEXT("five enemies for one second fill 0.4"), FMath::IsNearlyEqual(Meter.GetValue(), 0.4f, 0.001f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOverwhelmDrainsWhenClear, "WebTesting.Overwhelm.DrainsWhenClear", WIZ_TEST_FLAGS)
bool FOverwhelmDrainsWhenClear::RunTest(const FString& Parameters)
{
	FOverwhelmMeter Meter;
	// Changed 2026-11-14 (director: it does not go down until 2 seconds without build-up, and lasts a little longer):
	// this asked for 0.25 gone after one clear second.
	Meter.Step(1.0f, OverwhelmTest::Offsets(5));
	Meter.Step(Meter.Tuning.DrainDelay, OverwhelmTest::None);
	Meter.Step(1.0f, OverwhelmTest::None);
	TestTrue(TEXT("one clear second past the wait drains 0.2"), FMath::IsNearlyEqual(Meter.GetValue(), 0.2f, 0.001f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOverwhelmNeverBelowZero, "WebTesting.Overwhelm.NeverBelowZero", WIZ_TEST_FLAGS)
bool FOverwhelmNeverBelowZero::RunTest(const FString& Parameters)
{
	FOverwhelmMeter Meter;
	Meter.Step(10.0f, OverwhelmTest::None);
	TestEqual(TEXT("value"), Meter.GetValue(), 0.0f);
	TestFalse(TEXT("not falling"), Meter.IsFalling());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOverwhelmDirection, "WebTesting.Overwhelm.Direction", WIZ_TEST_FLAGS)
bool FOverwhelmDirection::RunTest(const FString& Parameters)
{
	FOverwhelmMeter Meter;
	Meter.Step(0.1f, { FVector2D(100.0, 0.0), FVector2D(100.0, 0.0) });
	TestTrue(TEXT("pressure from +X"), Meter.GetPressureDirection().Equals(FVector2D(1.0, 0.0), 0.001));

	Meter.Step(0.1f, { FVector2D(100.0, 0.0), FVector2D(0.0, 100.0) });
	TestTrue(TEXT("pressure from the +X +Y diagonal"), Meter.GetPressureDirection().Equals(FVector2D(1.0, 1.0).GetSafeNormal(), 0.001));

	Meter.Step(0.1f, OverwhelmTest::None);
	TestTrue(TEXT("no pressure when clear"), Meter.GetPressureDirection().IsNearlyZero());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOverwhelmZeroDirection, "WebTesting.Overwhelm.ZeroDirection", WIZ_TEST_FLAGS)
bool FOverwhelmZeroDirection::RunTest(const FString& Parameters)
{
	FOverwhelmMeter Meter;
	Meter.Step(0.1f, { FVector2D(0.0, 0.0) });
	const FVector2D Direction = Meter.GetPressureDirection();
	TestFalse(TEXT("no NaN"), Direction.ContainsNaN());
	TestTrue(TEXT("zero direction"), Direction.IsNearlyZero());
	TestTrue(TEXT("the meter still fills"), Meter.GetValue() > 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOverwhelmFallsOnce, "WebTesting.Overwhelm.FallsOnce", WIZ_TEST_FLAGS)
bool FOverwhelmFallsOnce::RunTest(const FString& Parameters)
{
	FOverwhelmMeter Meter;
	int32 Falls = 0;
	for (int32 Index = 0; Index < 6; ++Index)
	{
		Falls += Meter.Step(0.5f, OverwhelmTest::Offsets(10)) ? 1 : 0;
	}
	TestEqual(TEXT("the fall is reported exactly once"), Falls, 1);
	TestTrue(TEXT("falling"), Meter.IsFalling());
	TestEqual(TEXT("value stays full"), Meter.GetValue(), 1.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOverwhelmFallTimeline, "WebTesting.Overwhelm.FallTimeline", WIZ_TEST_FLAGS)
bool FOverwhelmFallTimeline::RunTest(const FString& Parameters)
{
	FOverwhelmMeter Meter;
	TestTrue(TEXT("the fall starts"), Meter.Step(2.0f, OverwhelmTest::Offsets(10)));
	TestEqual(TEXT("fall alpha at the start"), Meter.GetFallAlpha(), 0.0f);
	TestFalse(TEXT("not finished at the start"), Meter.IsFallFinished());

	Meter.Step(1.0f, OverwhelmTest::None);
	TestTrue(TEXT("half way after one second"), FMath::IsNearlyEqual(Meter.GetFallAlpha(), 0.5f, 0.001f));
	TestFalse(TEXT("not finished half way"), Meter.IsFallFinished());

	Meter.Step(1.0f, OverwhelmTest::None);
	TestTrue(TEXT("finished after two seconds"), Meter.IsFallFinished());

	Meter.Reset();
	TestFalse(TEXT("reset clears the fall"), Meter.IsFalling());
	TestEqual(TEXT("reset clears the value"), Meter.GetValue(), 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOverwhelmGodMode, "WebTesting.Overwhelm.GodMode", WIZ_TEST_FLAGS)
bool FOverwhelmGodMode::RunTest(const FString& Parameters)
{
	FOverwhelmMeter Meter;
	Meter.bGodMode = true;
	bool bFell = false;
	for (int32 Index = 0; Index < 10; ++Index)
	{
		bFell |= Meter.Step(1.0f, OverwhelmTest::Offsets(10));
	}
	TestFalse(TEXT("never falls"), bFell);
	TestEqual(TEXT("value"), Meter.GetValue(), 0.0f);
	TestFalse(TEXT("not falling"), Meter.IsFalling());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
