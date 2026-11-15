#include "Game/CrowdSubsystem.h"
#include "Sim/CrowdSim.h"
#include "Sim/LevelLayout.h"
#include "Tests/CrowdTestHelpers.h"
#include "Tests/WizTestFlags.h"

#if WITH_DEV_AUTOMATION_TESTS

// Direction of 2026-11-15 02:45 PST:
// - "Have the spear sound follow the actual spear rather than staying in one location."
// - "For the wizard's bolt, do not allow houses or lamp posts to destroy the bolt. Allow it to pass through."
// - "allow the commanders spear to hit all targets in it's path as it goes."

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBoltPassesHouses, "WebTesting.Bolt.PassesThroughHousesAndLampPosts", WIZ_TEST_FLAGS)
bool FBoltPassesHouses::RunTest(const FString& Parameters)
{
	// A house between the wizard and a soldier: the Bolt goes through it and meets the soldier.
	FCrowdSim Sim;
	CrowdTest::MakeEnemiesPlain(Sim);
	Sim.Tuning.GruntSpeed = 0.0f;
	FCrowdObstacle House = CrowdTest::Wall(300.0, 0.0, 60.0, 200.0);
	House.bBoltPasses = true;
	Sim.SetObstacles({ House });
	Sim.AddEnemy(ECrowdKind::Grunt, FVector2D(600.0, 0.0));
	Sim.FireBolt(CrowdTest::Origin, FVector2D(1.0, 0.0));
	int32 Hits = 0;
	int32 Walls = 0;
	for (int32 Step = 0; Step < 30; ++Step)
	{
		Sim.Step(0.05f, CrowdTest::Origin);
		for (const FCrowdEvent& Event : Sim.GetEvents())
		{
			Hits += Event.Type == ECrowdEvent::BoltHit ? 1 : 0;
			Walls += Event.Type == ECrowdEvent::BoltWall ? 1 : 0;
		}
	}
	TestEqual(TEXT("the house does not stop it"), Walls, 0);
	TestEqual(TEXT("it meets the soldier behind the house"), Hits, 1);

	// A wall that is not a house still stops it.
	FCrowdSim Walled;
	CrowdTest::MakeEnemiesPlain(Walled);
	Walled.Tuning.GruntSpeed = 0.0f;
	Walled.SetObstacles({ CrowdTest::Wall(300.0, 0.0, 60.0, 200.0) });
	Walled.AddEnemy(ECrowdKind::Grunt, FVector2D(600.0, 0.0));
	Walled.FireBolt(CrowdTest::Origin, FVector2D(1.0, 0.0));
	Hits = 0;
	Walls = 0;
	for (int32 Step = 0; Step < 30; ++Step)
	{
		Walled.Step(0.05f, CrowdTest::Origin);
		for (const FCrowdEvent& Event : Walled.GetEvents())
		{
			Hits += Event.Type == ECrowdEvent::BoltHit ? 1 : 0;
			Walls += Event.Type == ECrowdEvent::BoltWall ? 1 : 0;
		}
	}
	TestEqual(TEXT("a wall stops it"), Walls, 1);
	TestEqual(TEXT("and nothing behind the wall is met"), Hits, 0);

	// Which obstacles of a level let it through: houses, not trees.
	FLevelLayout Layout;
	FLevelPiece Piece;
	Piece.Kind = ELevelPiece::House;
	Piece.Center = FVector2D(100.0, 0.0);
	Layout.Pieces.Add(Piece);
	Piece.Kind = ELevelPiece::Tree;
	Piece.Center = FVector2D(900.0, 0.0);
	Layout.Pieces.Add(Piece);
	const TArray<FCrowdObstacle> Made = Layout.MakeObstacles();
	if (TestEqual(TEXT("both block walking"), Made.Num(), 2))
	{
		TestTrue(TEXT("a Bolt passes a house"), Made[0].bBoltPasses);
		TestFalse(TEXT("a tree stops it"), Made[1].bBoltPasses);
	}

	// The meshes placed in the level.
	TestTrue(TEXT("a house's mesh"), UCrowdSubsystem::BoltPassesMesh(TEXT("/Game/Buildings/SM_Barn.SM_Barn")));
	TestTrue(TEXT("a lamp post's mesh"), UCrowdSubsystem::BoltPassesMesh(TEXT("/Game/Props/SM_Lamp_Post.SM_Lamp_Post")));
	TestFalse(TEXT("a castle wall's mesh"), UCrowdSubsystem::BoltPassesMesh(TEXT("/Game/Kingdom/SM_Castle_Wall.SM_Castle_Wall")));
	TestFalse(TEXT("a bridge's mesh"), UCrowdSubsystem::BoltPassesMesh(TEXT("/Game/Props/SM_Bridge.SM_Bridge")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpearPierces, "WebTesting.Commander.HisSpearHitsEveryoneInItsPath", WIZ_TEST_FLAGS)
bool FSpearPierces::RunTest(const FString& Parameters)
{
	FCrowdSim Sim;
	CrowdTest::MakeEnemiesPlain(Sim);
	CrowdTest::FreezeArmy(Sim);
	Sim.Tuning.CommanderSpeed = 0.0f;
	Sim.Tuning.CommanderSpearRange = 5000.0f;
	Sim.Tuning.CommanderSpearLeadChance = 0.0f;
	Sim.Tuning.CommanderSpearInterval = 1000.0f;
	Sim.Tuning.SkeletonHealth = 100.0f;
	Sim.Tuning.CommanderSpearDamage = 15.0f;
	Sim.AddEnemy(ECrowdKind::Commander, FVector2D(900.0, 0.0));
	// Three of the army in the spear's way, one well off to the side.
	Sim.AddSkeleton(FVector2D(300.0, 0.0));
	Sim.AddSkeleton(FVector2D(450.0, 10.0));
	Sim.AddSkeleton(FVector2D(600.0, -10.0));
	Sim.AddSkeleton(FVector2D(450.0, 400.0));
	int32 Thrown = 0;
	int32 HitArmy = 0;
	int32 HitWizard = 0;
	int32 ThrownId = 0;
	int32 FlyingId = 0;
	for (float Time = 0.0f; Time < 6.0f; Time += 0.05f)
	{
		const FCrowdStepResult Result = Sim.Step(0.05f, CrowdTest::Origin);
		Thrown += Result.JavelinsThrown;
		HitArmy += Result.JavelinsHitArmy;
		HitWizard += Result.JavelinsHitWizard;
		for (const FCrowdEvent& Event : Sim.GetEvents())
		{
			if (Event.Type == ECrowdEvent::SpearThrown)
			{
				ThrownId = Event.Id;
			}
		}
		if (Sim.GetJavelins().Num() > 0)
		{
			FlyingId = Sim.GetJavelins()[0].Id;
		}
	}
	TestEqual(TEXT("he threw once"), Thrown, 1);
	TestEqual(TEXT("each of the three in its way is hit, once"), HitArmy, 3);
	TestEqual(TEXT("and it still reaches the wizard"), HitWizard, 1);
	TestTrue(TEXT("the throw is reported with the spear's own number, so its sound can follow it"), ThrownId != 0 && ThrownId == FlyingId);
	int32 Hurt = 0;
	int32 Whole = 0;
	for (const FCrowdUnit& Unit : Sim.GetUnits())
	{
		if (Unit.Team == ECrowdTeam::Army)
		{
			Hurt += FMath::IsNearlyEqual(Unit.Health, 85.0f, 0.01f) ? 1 : 0;
			Whole += FMath::IsNearlyEqual(Unit.Health, 100.0f, 0.01f) ? 1 : 0;
		}
	}
	TestEqual(TEXT("each took the spear's damage once"), Hurt, 3);
	TestEqual(TEXT("the one off to the side is untouched"), Whole, 1);
	return true;
}

#endif
