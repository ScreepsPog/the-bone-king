#include "Sim/CrowdSim.h"
#include "Tests/CrowdTestHelpers.h"
#include "Tests/WizTestFlags.h"

#if WITH_DEV_AUTOMATION_TESTS

// Direction of 2026-11-14 (00:45 PST): when the enemy loses sight of the wizard, by distance or around a corner, it
// goes to the spot he was last seen at, looks about for a few seconds, then runs back to where it was when it started.

namespace SightTest
{
	const float StepSeconds = 0.05f;
	const FVector2D Post(600.0, 0.0);

	void Run(FCrowdSim& Sim, float Seconds, FVector2D Wizard)
	{
		for (float Time = 0.0f; Time < Seconds - 0.001f; Time += StepSeconds)
		{
			Sim.Step(StepSeconds, Wizard);
		}
	}

	/** One grunt standing at its post, in group 1. */
	void PostGrunt(FCrowdSim& Sim, FVector2D Where = Post)
	{
		CrowdTest::MakeEnemiesPlain(Sim);
		Sim.Tuning.PostWanderRadius = 0.0f;
		Sim.AddGarrisonEnemy(ECrowdKind::Grunt, Where, 1);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLostByDistance, "WebTesting.Sight.LostByDistanceItSearchesThenGoesBack", WIZ_TEST_FLAGS)
bool FLostByDistance::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	SightTest::PostGrunt(Sim);
	TestEqual(TEXT("it looks about for a few seconds"), Sim.Tuning.SearchSeconds, 3.0f);

	// The wizard walks up: the grunt notices him and comes.
	SightTest::Run(Sim, 0.5f, CrowdTest::Origin);
	const FCrowdUnit* Grunt = CrowdTest::First(Sim, ECrowdTeam::Enemy);
	if (!TestNotNull(TEXT("the grunt"), Grunt))
	{
		return false;
	}
	TestFalse(TEXT("it has left its post"), Grunt->bAsleep);
	TestTrue(TEXT("and comes at the wizard"), Grunt->Position.X < SightTest::Post.X - 50.0);
	TestTrue(TEXT("it remembers where it stood"), Grunt->ReturnPoint.Equals(SightTest::Post, 1.0));

	// He is gone, far out of sight.
	const FVector2D Far(6000.0, 0.0);
	Sim.Step(SightTest::StepSeconds, Far);
	Grunt = CrowdTest::First(Sim, ECrowdTeam::Enemy);
	TestTrue(TEXT("with the wizard out of sight it goes to where he was last seen"), Grunt->Alert == ECrowdAlert::Seek);

	float Waited = 0.0f;
	for (; Waited < 10.0f && Grunt->Alert == ECrowdAlert::Seek; Waited += SightTest::StepSeconds)
	{
		Sim.Step(SightTest::StepSeconds, Far);
		Grunt = CrowdTest::First(Sim, ECrowdTeam::Enemy);
		TestTrue(TEXT("it does not follow him"), Grunt->Position.X < SightTest::Post.X + 20.0);
	}
	TestTrue(TEXT("there, it looks about"), Grunt->Alert == ECrowdAlert::Look);
	TestTrue(TEXT("at the spot he was last seen"), FVector2D::Distance(Grunt->Position, CrowdTest::Origin) <= Sim.Tuning.SearchArriveRadius + 1.0);

	const FVector2D Stood = Grunt->Position;
	SightTest::Run(Sim, Sim.Tuning.SearchSeconds - 0.5f, Far);
	Grunt = CrowdTest::First(Sim, ECrowdTeam::Enemy);
	TestTrue(TEXT("it is still looking"), Grunt->Alert == ECrowdAlert::Look);
	TestTrue(TEXT("and has not walked"), Grunt->Position.Equals(Stood, 1.0));
	TestEqual(TEXT("it counts as looking for him"), Sim.CountAlert(ECrowdAlert::Look), 1);

	SightTest::Run(Sim, 1.0f, Far);
	Grunt = CrowdTest::First(Sim, ECrowdTeam::Enemy);
	TestTrue(TEXT("after a few seconds it turns back"), Grunt->Alert == ECrowdAlert::Return);

	SightTest::Run(Sim, 6.0f, Far);
	Grunt = CrowdTest::First(Sim, ECrowdTeam::Enemy);
	TestTrue(TEXT("it is back where it stood when it started"), FVector2D::Distance(Grunt->Position, SightTest::Post) <= 80.0);
	TestEqual(TEXT("and at its post again"), Sim.CountAsleep(), 1);

	// And it can notice him again.
	SightTest::Run(Sim, 0.5f, SightTest::Post + FVector2D(0.0, 500.0));
	TestEqual(TEXT("he comes near again and it leaves its post again"), Sim.CountAsleep(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLostAroundCorner, "WebTesting.Sight.LostAroundACornerItDoesNotFollow", WIZ_TEST_FLAGS)
bool FLostAroundCorner::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	// A long wall west of the post.
	Sim.SetObstacles({ CrowdTest::Wall(300.0, 0.0, 20.0, 400.0) });
	SightTest::PostGrunt(Sim);

	// The wizard shows himself on the grunt's side of the wall.
	const FVector2D Seen(700.0, 300.0);
	SightTest::Run(Sim, 0.3f, Seen);
	TestEqual(TEXT("the grunt notices him"), Sim.CountAsleep(), 0);

	// He steps behind the wall: near enough to see, but the wall is between them.
	const FVector2D Hidden(-200.0, 0.0);
	const FCrowdUnit* Grunt = CrowdTest::First(Sim, ECrowdTeam::Enemy);
	TestFalse(TEXT("the wall hides him"), Sim.CanSee(*Grunt, Hidden));
	TestTrue(TEXT("though he is within sight's reach"), FVector2D::Distance(Grunt->Position, Hidden) < Sim.Tuning.SightRadius);

	double Nearest = TNumericLimits<double>::Max();
	bool bLooked = false;
	for (float Time = 0.0f; Time < 14.0f; Time += SightTest::StepSeconds)
	{
		Sim.Step(SightTest::StepSeconds, Hidden);
		Grunt = CrowdTest::First(Sim, ECrowdTeam::Enemy);
		Nearest = FMath::Min(Nearest, FVector2D::Distance(Grunt->Position, Hidden));
		if (Grunt->Alert == ECrowdAlert::Look && !Grunt->bAsleep)
		{
			bLooked = true;
			TestTrue(TEXT("it looks where he was last seen"), FVector2D::Distance(Grunt->Position, Seen) <= Sim.Tuning.SearchArriveRadius + 1.0);
		}
	}
	TestTrue(TEXT("it looked about"), bLooked);
	TestTrue(TEXT("it never came around the wall after him"), Nearest > 400.0);
	TestTrue(TEXT("it is back where it stood"), FVector2D::Distance(Grunt->Position, SightTest::Post) <= 80.0);
	TestEqual(TEXT("at its post, and the wall keeps it from noticing him again"), Sim.CountAsleep(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeenAgain, "WebTesting.Sight.SeenAgainItChasesAgain", WIZ_TEST_FLAGS)
bool FSeenAgain::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	SightTest::PostGrunt(Sim);
	SightTest::Run(Sim, 0.5f, CrowdTest::Origin);
	SightTest::Run(Sim, 0.3f, FVector2D(6000.0, 0.0));
	TestEqual(TEXT("it is going to where he was last seen"), Sim.CountAlert(ECrowdAlert::Seek), 1);

	const FVector2D Back(0.0, 300.0);
	Sim.Step(SightTest::StepSeconds, Back);
	const FCrowdUnit* Grunt = CrowdTest::First(Sim, ECrowdTeam::Enemy);
	TestTrue(TEXT("he shows himself and it chases again"), Grunt && Grunt->Alert == ECrowdAlert::Chase);
	SightTest::Run(Sim, 3.0f, Back);
	TestEqual(TEXT("and it reaches him"), CrowdTest::CountTouching(Sim, Back), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGroupSharesSight, "WebTesting.Sight.AGroupSeesHimWhenOneOfItDoes", WIZ_TEST_FLAGS)
bool FGroupSharesSight::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	Sim.SetObstacles({ CrowdTest::Wall(300.0, 0.0, 20.0, 200.0) });
	CrowdTest::MakeEnemiesPlain(Sim);
	Sim.Tuning.PostWanderRadius = 0.0f;
	Sim.Tuning.GruntSpeed = 0.0f;
	// One behind the wall from the wizard, one with a clear line to him.
	Sim.AddGarrisonEnemy(ECrowdKind::Grunt, FVector2D(600.0, 0.0), 1);
	Sim.AddGarrisonEnemy(ECrowdKind::Grunt, FVector2D(600.0, 600.0), 1);
	SightTest::Run(Sim, 0.2f, CrowdTest::Origin);

	TestFalse(TEXT("the wall hides him from the first"), Sim.CanSee(Sim.GetUnits()[0], CrowdTest::Origin));
	TestTrue(TEXT("the second sees him"), Sim.CanSee(Sim.GetUnits()[1], CrowdTest::Origin));
	TestEqual(TEXT("the whole group has left its post"), Sim.CountAsleep(), 0);
	TestEqual(TEXT("and both know where he is"), Sim.CountAlert(ECrowdAlert::Chase), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSentNeverLose, "WebTesting.Sight.ThoseSentAfterHimNeverLoseHim", WIZ_TEST_FLAGS)
bool FSentNeverLose::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	CrowdTest::MakeEnemiesPlain(Sim);
	// A wave's grunt, far past sight's reach.
	Sim.AddEnemy(ECrowdKind::Grunt, FVector2D(3000.0, 0.0));
	SightTest::Run(Sim, 2.0f, CrowdTest::Origin);
	const FCrowdUnit* Grunt = CrowdTest::First(Sim, ECrowdTeam::Enemy);
	TestTrue(TEXT("it comes straight at him all the same"), Grunt && Grunt->Position.X < 2600.0);
	TestEqual(TEXT("nobody is looking for him"), Sim.CountAlert(ECrowdAlert::Seek) + Sim.CountAlert(ECrowdAlert::Look), 0);
	return true;
}

#endif
