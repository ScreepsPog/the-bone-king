#include "Sim/CrowdSim.h"

namespace
{
	// Spreads units that sit on exactly the same spot, so the push direction is never undefined.
	const double GoldenAngle = 2.39996322972865332;

	// Melee still lands when separation nudges two fighters slightly past the range.
	const float MeleeSlack = 5.0f;

	// A bolt is thinner than a unit: it stops this far from the face of a wall.
	const double BoltWallMargin = 10.0;

	// A roam spot counts as reached this close to it.
	const double RoamArriveDistance = 5.0;
	// An enemy running back counts as back within this of where it stood: others of its group may stand in the way.
	const double ReturnArriveDistance = 60.0;
	// Looking about: how far to each side of its own bearing it turns (radians), and how fast it sweeps.
	const double LookSweep = 1.4;
	const double LookRate = 2.2;

	const int32 RoamPickTries = 6;

	// Where the first group stands, as an angle around the wizard. Off the axes, so no group starts straight ahead.
	const double GroupFirstAngle = 0.6;

	// Added to a Guard's distance when the spear chooses whose bones to take, so plain skeletons always go first.
	const double GuardSpareScore = 1.0e12;

	// A skeleton assembled from bones stands up this far from the wizard.
	const double AssembleDistance = 90.0;

	// A patrol's point counts as reached this close to it, and each of the patrol keeps its own place this far from the line.
	const double PatrolArriveDistance = 40.0;
	const double PatrolSpread = 70.0;

	// An archer that has started to run keeps running until the nearest threat is this many times its flee range away.
	const double ArcherSafeShare = 1.5;
	// With nothing to shoot, it walks toward the wizard until it is this share of its range from him.
	const double ArcherApproachShare = 0.8;
	// An arrow has this long over its flight before it is given up as lost.
	const float ArrowSpareSeconds = 0.2f;
	// How hard an assassin swerves around a skeleton in its way.
	// How long the commander is shown following a throw through after the spear has left his hand.
	const float ThrowFollowSeconds = 1.3f;
	const double AssassinSwerve = 1.6;
	// The wizard has to go this far past the rejoin distance before sent units will rejoin on his return.
	const double CommandArmSlack = 60.0;
	// A unit on its way steers around one of its own side that stands within this many separation radii ahead of it...
	const double AvoidRangeShare = 1.6;
	// ...this hard.
	const double AvoidSwerve = 1.2;
	// A unit this many separation radii from its spot gives the spot up when one of its own side stands on it.
	const double TakenCheckShare = 2.5;
	// A spot is taken when one of its own side stands within this share of the separation radius of it.
	const double TakenRadiusShare = 0.5;
	// ...or within this share of the room the two bodies need.
	const double TakenBodyShare = 1.0;
	// How often one of the army may give up a spot it cannot reach or that is taken, in seconds.
	const float RepickSeconds = 0.4f;
	const int32 OpenPickTries = 10;
	// The wizard's speed, as an archer reckons it, is never taken for more than this (a Blink is not a walk).
	const double LeadSpeedMost = 900.0;
	// An enemy turns on a skeleton in its way when it is this much farther off than their bodies touching.
	const double BlockGap = 20.0;
	// A unit going slower than this keeps facing the way it faced.
	const double FacingSpeed = 5.0;
	// A unit that moved less than this in a step counts as standing.
	const double StandingMove = 0.5;
	// How far inside its reach a unit closes on an enemy that runs from it.
	const double RunnerCloseShare = 0.7;
	// A sent group's circle never reaches the edge of the rejoin circle.
	const double CommandRadiusShare = 0.85;
	// A unit that ends up in a wall or over water is moved out this many times before it is put back where it last stood clear.
	const int32 PushOutTries = 6;
	const double PushOutGap = 0.01;

	// A javelin flies this share of the commander's range before it drops.
	const double JavelinReachShare = 1.2;

	// The circle of a brute's slam, or of the commander's slash: the same thing with other numbers.
	struct FSlamShape
	{
		float Radius;
		float Distance;
		float Grow;
		float Hold;
		float Rest;
	};

	FSlamShape MakeSlamShape(const FCrowdTuning& Tuning, ECrowdKind Kind)
	{
		if (Kind == ECrowdKind::Commander)
		{
			return { Tuning.CommanderSlashRadius, Tuning.CommanderSlashDistance, Tuning.CommanderSlashGrow, Tuning.CommanderSlashHold, Tuning.CommanderSlashRest };
		}
		return { Tuning.BruteSlamRadius, Tuning.BruteSlamDistance, Tuning.BruteSlamGrow, Tuning.BruteSlamHold, Tuning.BruteSlamRest };
	}

	/** The shape of the attack a unit is in: a brute in a sweep has the sweep's reach and times. */
	FSlamShape MakeSlamShape(const FCrowdTuning& Tuning, const FCrowdUnit& Unit)
	{
		if (Unit.Kind == ECrowdKind::Brute && Unit.bSweep)
		{
			return { Tuning.BruteSweepRadius, 0.0f, Tuning.BruteSweepWindup, 0.0f, Tuning.BruteSweepRest };
		}
		return MakeSlamShape(Tuning, Unit.Kind);
	}

	bool IsUnitItem(ECrowdShopItem Item)
	{
		return Item == ECrowdShopItem::Guard || Item == ECrowdShopItem::Marksman;
	}

	bool MatchesCommandKind(const FCrowdUnit& Unit, ECrowdCommandKind Kind)
	{
		switch (Kind)
		{
		case ECrowdCommandKind::Skeleton:
			return Unit.Kind == ECrowdKind::Skeleton;
		case ECrowdCommandKind::Guard:
			return Unit.Kind == ECrowdKind::Guard;
		case ECrowdCommandKind::Any:
		default:
			return true;
		}
	}
}

bool FCrowdSim::AddEnemy(ECrowdKind Kind, FVector2D Position)
{
	if (CountTeam(ECrowdTeam::Enemy) >= Tuning.MaxEnemies)
	{
		return false;
	}
	FCrowdUnit Unit;
	Unit.Position = Position;
	Unit.Health = GetStats(Kind).Health;
	Unit.MaxHealth = Unit.Health;
	Unit.Team = ECrowdTeam::Enemy;
	Unit.Kind = Kind;
	Unit.Flank = Random.FRandRange(-1.0f, 1.0f);
	Unit.SpeedScale = 1.0f + Random.FRandRange(-Tuning.EnemySpeedVariation, Tuning.EnemySpeedVariation);
	Unit.Id = NextId++;
	// Its first blow is wound up like any other. (An archer's timer is for its arrows.)
	Unit.AttackTimer = Kind == ECrowdKind::Archer ? 0.0f : FMath::Max(Tuning.MeleeInterval, 0.0f) * Tuning.MeleeFirstShare;
	PushOutOfObstacles(Unit.Position);
	Units.Add(Unit);
	return true;
}

int32 FCrowdSim::GetBoneValue(ECrowdKind Kind) const
{
	switch (Kind)
	{
	case ECrowdKind::Grunt:
		return FMath::RoundToInt32(Tuning.BonesPerGrunt);
	case ECrowdKind::Brute:
		return FMath::RoundToInt32(Tuning.BonesPerBrute);
	case ECrowdKind::Commander:
		return FMath::RoundToInt32(Tuning.BonesPerCommander);
	case ECrowdKind::Archer:
		return FMath::RoundToInt32(Tuning.BonesPerArcher);
	case ECrowdKind::Assassin:
		return FMath::RoundToInt32(Tuning.BonesPerAssassin);
	default:
		return 0;
	}
}

bool FCrowdSim::AddGarrisonEnemy(ECrowdKind Kind, FVector2D Position, int32 Squad, float PostRadius, FVector2D Post)
{
	if (!AddEnemy(Kind, Position))
	{
		return false;
	}
	FCrowdUnit& Unit = Units.Last();
	Unit.bAsleep = true;
	Unit.bPosted = true;
	Unit.WakeHealth = Unit.Health;
	Unit.Squad = Squad;
	// Most of a group walks about near where it was posted; some stand. The commander stands.
	Unit.Home = Unit.Position;
	Unit.bWanders = Kind != ECrowdKind::Commander && Random.FRand() >= Tuning.PostIdleShare;
	Unit.RoamOffset = FVector2D::ZeroVector;
	if (PostRadius > 0.0f)
	{
		// A post with ground of its own: it walks about the whole of it, from the place it was given in it.
		Unit.PostRadius = PostRadius;
		Unit.Home = Post;
		Unit.RoamOffset = Unit.Position - Post;
	}
	Unit.RoamTimer = Random.FRandRange(0.0f, Tuning.RoamPauseMax);
	return true;
}

int32 FCrowdSim::AddRoute(const TArray<FVector2D>& Points)
{
	return Routes.Add(Points);
}

bool FCrowdSim::AddPatrolEnemy(ECrowdKind Kind, FVector2D Position, int32 Squad, int32 Route, int32 RoutePoint, int32 RouteStep)
{
	if (!Routes.IsValidIndex(Route) || Routes[Route].Num() < 2 || !AddGarrisonEnemy(Kind, Position, Squad))
	{
		return false;
	}
	FCrowdUnit& Unit = Units.Last();
	Unit.Route = Route;
	Unit.RoutePoint = FMath::Clamp(RoutePoint, 0, Routes[Route].Num() - 1);
	Unit.RouteStep = RouteStep >= 0 ? 1 : -1;
	// Its own place in the group, so a patrol walks as a loose band and not in single file.
	const double Angle = Random.FRandRange(0.0f, UE_TWO_PI);
	Unit.RoamOffset = FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * (PatrolSpread * FMath::Sqrt(static_cast<double>(Random.FRand())));
	return true;
}

int32 FCrowdSim::CountSquad(int32 Squad) const
{
	if (Squad == INDEX_NONE)
	{
		return 0;
	}
	int32 Count = 0;
	for (const FCrowdUnit& Unit : Units)
	{
		Count += Unit.Team == ECrowdTeam::Enemy && Unit.Squad == Squad ? 1 : 0;
	}
	return Count;
}

void FCrowdSim::MoveAtPost(FCrowdUnit& Unit, float DeltaSeconds)
{
	// An enemy that has not noticed the wizard: on patrol it walks its road, at a post it walks about a little or stands.
	FVector2D Goal = Unit.Position;
	double Speed = 0.0;
	double Arrive = RoamArriveDistance;
	const bool bOnPatrol = Routes.IsValidIndex(Unit.Route) && Routes[Unit.Route].Num() > 1;
	if (bOnPatrol)
	{
		const TArray<FVector2D>& Route = Routes[Unit.Route];
		Unit.RoutePoint = FMath::Clamp(Unit.RoutePoint, 0, Route.Num() - 1);
		Goal = Route[Unit.RoutePoint] + Unit.RoamOffset;
		Speed = Tuning.PatrolWalkSpeed;
		Arrive = PatrolArriveDistance;
	}
	else if (Unit.bWanders && (Unit.PostRadius > 0.0f ? Unit.PostRadius : Tuning.PostWanderRadius) > 0.0f)
	{
		Goal = Unit.Home + Unit.RoamOffset;
		Speed = Tuning.PostWanderSpeed;
	}
	else
	{
		return;
	}

	const FVector2D ToGoal = Goal - Unit.Position;
	const double Distance = ToGoal.Size();
	if (Distance > Arrive)
	{
		if (!bOnPatrol && IsSpotTaken(Unit, Goal))
		{
			// One of its own stands on the spot it was strolling to: it stops where it is and picks another later.
			Unit.RoamOffset = Unit.Position - Unit.Home;
			return;
		}
		if (Speed > 0.0)
		{
			Unit.Position += ToGoal / Distance * FMath::Min(Speed * DeltaSeconds, Distance);
		}
		return;
	}

	if (bOnPatrol)
	{
		// On to the next point; at an end of the road it turns back.
		const int32 Last = Routes[Unit.Route].Num() - 1;
		if (Unit.RoutePoint + Unit.RouteStep < 0 || Unit.RoutePoint + Unit.RouteStep > Last)
		{
			Unit.RouteStep = -Unit.RouteStep;
		}
		Unit.RoutePoint += Unit.RouteStep;
		return;
	}

	// It stands a while, then picks another spot near its post. A spot behind a wall is not taken.
	Unit.RoamTimer -= DeltaSeconds;
	if (Unit.RoamTimer <= 0.0f)
	{
		Unit.RoamTimer = Random.FRandRange(Tuning.RoamPauseMin, Tuning.RoamPauseMax);
		const double Angle = Random.FRandRange(0.0f, UE_TWO_PI);
		const FVector2D Offset = FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * ((Unit.PostRadius > 0.0f ? Unit.PostRadius : Tuning.PostWanderRadius) * FMath::Sqrt(static_cast<double>(Random.FRand())));
		if (Obstacles.Num() == 0 || HasClearPath(Unit.Position, Unit.Home + Offset))
		{
			Unit.RoamOffset = Offset;
		}
	}
}

int32 FCrowdSim::CountAsleep() const
{
	int32 Count = 0;
	for (const FCrowdUnit& Unit : Units)
	{
		Count += Unit.Team == ECrowdTeam::Enemy && Unit.bAsleep ? 1 : 0;
	}
	return Count;
}

void FCrowdSim::WakeAll()
{
	// Sent after him: from here on they know where he is, as a wave does.
	for (FCrowdUnit& Unit : Units)
	{
		Unit.bAsleep = false;
		Unit.bPosted = false;
		Unit.Alert = ECrowdAlert::Chase;
	}
	Sights.Reset();
}

int32 FCrowdSim::CountAlert(ECrowdAlert Alert) const
{
	int32 Count = 0;
	for (const FCrowdUnit& Unit : Units)
	{
		Count += Unit.Team == ECrowdTeam::Enemy && !Unit.bAsleep && Unit.Alert == Alert ? 1 : 0;
	}
	return Count;
}

bool FCrowdSim::CanSee(const FCrowdUnit& Unit, FVector2D Place) const
{
	if (Tuning.SightRadius <= 0.0f)
	{
		return true;
	}
	return FVector2D::DistSquared(Unit.Position, Place) <= FMath::Square(static_cast<double>(Tuning.SightRadius))
		&& HasClearShot(Unit.Position, Place);
}

int32 FCrowdSim::FindSight(int32 Key) const
{
	for (int32 Index = 0; Index < Sights.Num(); ++Index)
	{
		if (Sights[Index].Key == Key)
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

int32 FCrowdSim::FindOrAddSight(int32 Key, FVector2D Place)
{
	int32 Index = FindSight(Key);
	if (Index == INDEX_NONE)
	{
		Index = Sights.AddDefaulted();
		Sights[Index].Key = Key;
		Sights[Index].LastSeen = Place;
		if (Obstacles.Num() > 0)
		{
			Sights[Index].SeekField.Build(Place, Obstacles, Tuning.UnitRadius);
		}
	}
	return Index;
}

void FCrowdSim::Rouse(FCrowdUnit& Unit, FVector2D Place)
{
	// It leaves its post for the place something was noticed. If the wizard is in sight from there, it chases him.
	Unit.bAsleep = false;
	Unit.ReturnPoint = Unit.Position;
	Unit.Alert = ECrowdAlert::Seek;
	Unit.AlertTimer = Tuning.SeekSeconds;
	FCrowdSight& Sight = Sights[FindOrAddSight(GetSightKey(Unit), Place)];
	if (!Sight.LastSeen.Equals(Place, 1.0))
	{
		Sight.LastSeen = Place;
		if (Obstacles.Num() > 0)
		{
			Sight.SeekField.Build(Place, Obstacles, Tuning.UnitRadius);
		}
	}
	Sight.bReturnBuilt = false;
}

void FCrowdSim::UpdateSight(float DeltaSeconds, FVector2D WizardPosition)
{
	for (FCrowdSight& Sight : Sights)
	{
		Sight.bSaw = Sight.bSees;
		Sight.bSees = false;
		Sight.bAlive = false;
	}
	// A group sees him when any one of it does.
	for (const FCrowdUnit& Unit : Units)
	{
		if (Unit.Team != ECrowdTeam::Enemy || Unit.bAsleep || !Unit.bPosted)
		{
			continue;
		}
		FCrowdSight& Sight = Sights[FindOrAddSight(GetSightKey(Unit), Unit.Position)];
		Sight.bAlive = true;
		if (!Sight.bSees && CanSee(Unit, WizardPosition))
		{
			Sight.bSees = true;
		}
	}
	for (int32 Index = Sights.Num() - 1; Index >= 0; --Index)
	{
		FCrowdSight& Sight = Sights[Index];
		if (!Sight.bAlive)
		{
			// The group is dead, or all of it is back at its post.
			Sights.RemoveAtSwap(Index);
		}
		else if (Sight.bSees)
		{
			Sight.LastSeen = WizardPosition;
			Sight.bReturnBuilt = false;
		}
		else if (Sight.bSaw && Obstacles.Num() > 0)
		{
			// Just lost: the way to where he was last seen is worked out once.
			Sight.SeekField.Build(Sight.LastSeen, Obstacles, Tuning.UnitRadius);
		}
	}

	const double ArriveSquared = FMath::Square(static_cast<double>(Tuning.SearchArriveRadius));
	for (FCrowdUnit& Unit : Units)
	{
		if (Unit.Team != ECrowdTeam::Enemy || Unit.bAsleep || !Unit.bPosted)
		{
			continue;
		}
		const int32 SightIndex = FindSight(GetSightKey(Unit));
		if (!Sights.IsValidIndex(SightIndex))
		{
			continue;
		}
		FCrowdSight& Sight = Sights[SightIndex];
		if (Sight.bSees)
		{
			Unit.Alert = ECrowdAlert::Chase;
			continue;
		}
		Unit.AlertTimer -= DeltaSeconds;
		switch (Unit.Alert)
		{
		case ECrowdAlert::Chase:
			Unit.Alert = ECrowdAlert::Seek;
			Unit.AlertTimer = Tuning.SeekSeconds;
			break;
		case ECrowdAlert::Seek:
			if (Unit.AlertTimer <= 0.0f || FVector2D::DistSquared(Unit.Position, Sight.LastSeen) <= ArriveSquared)
			{
				Unit.Alert = ECrowdAlert::Look;
				Unit.AlertTimer = Tuning.SearchSeconds;
			}
			break;
		case ECrowdAlert::Look:
			if (Unit.AlertTimer <= 0.0f)
			{
				Unit.Alert = ECrowdAlert::Return;
				Unit.AlertTimer = Tuning.ReturnSeconds;
				if (!Sight.bReturnBuilt && Obstacles.Num() > 0)
				{
					Sight.ReturnField.Build(Unit.ReturnPoint, Obstacles, Tuning.UnitRadius);
					Sight.bReturnBuilt = true;
				}
			}
			break;
		case ECrowdAlert::Return:
		default:
			if (Unit.AlertTimer <= 0.0f || FVector2D::DistSquared(Unit.Position, Unit.ReturnPoint) <= FMath::Square(ReturnArriveDistance))
			{
				// Back where it stood: it takes up its post or its patrol again, and can be roused again.
				Unit.bAsleep = true;
				Unit.Alert = ECrowdAlert::Chase;
				Unit.WakeHealth = Unit.Health;
				Unit.TargetIndex = INDEX_NONE;
				if (!Routes.IsValidIndex(Unit.Route))
				{
					Unit.RoamOffset = Unit.Position - Unit.Home;
					Unit.RoamTimer = Random.FRandRange(Tuning.RoamPauseMin, Tuning.RoamPauseMax);
				}
			}
			break;
		}
	}
}

bool FCrowdSim::GetNearestPost(FVector2D From, FVector2D& OutPost) const
{
	double BestSquared = TNumericLimits<double>::Max();
	for (const FCrowdUnit& Unit : Units)
	{
		if (Unit.Team != ECrowdTeam::Enemy || !Unit.bAsleep)
		{
			continue;
		}
		const double DistanceSquared = FVector2D::DistSquared(Unit.Position, From);
		if (DistanceSquared < BestSquared)
		{
			BestSquared = DistanceSquared;
			OutPost = Unit.Position;
		}
	}
	return BestSquared < TNumericLimits<double>::Max();
}

bool FCrowdSim::IsResearched(ECrowdShopItem Item) const
{
	switch (Item)
	{
	case ECrowdShopItem::Spear:
		return bSpearUnlocked;
	case ECrowdShopItem::Armor:
		return bArmor;
	case ECrowdShopItem::Blades:
		return bBlades;
	case ECrowdShopItem::Focus:
		return bFocus;
	default:
		return false;
	}
}

int32 FCrowdSim::WakeGarrisons(FVector2D WizardPosition)
{
	// A group leaves its post when the wizard or one of his army comes close to any of them, or when one of them is hit.
	const double WakeSquared = FMath::Square(static_cast<double>(Tuning.WakeRadius));
	// With sight on (Tuning.SightRadius above 0) a wall between hides them: a group is not roused through a house.
	// What it noticed, and where, is where the group goes first.
	const bool bWallsHide = Tuning.SightRadius > 0.0f;
	TArray<int32, TInlineAllocator<8>> Woken;
	TArray<FVector2D, TInlineAllocator<8>> WokenPlaces;
	int32 Loners = 0;
	for (FCrowdUnit& Unit : Units)
	{
		if (Unit.Team != ECrowdTeam::Enemy || !Unit.bAsleep)
		{
			continue;
		}
		FVector2D Place = WizardPosition;
		bool bWake = Unit.Health < Unit.WakeHealth || (FVector2D::DistSquared(Unit.Position, WizardPosition) <= WakeSquared
			&& (!bWallsHide || HasClearShot(Unit.Position, WizardPosition)));
		if (!bWake)
		{
			for (const FCrowdUnit& Other : Units)
			{
				if (Other.Team == ECrowdTeam::Army && FVector2D::DistSquared(Unit.Position, Other.Position) <= WakeSquared
					&& (!bWallsHide || HasClearShot(Unit.Position, Other.Position)))
				{
					bWake = true;
					Place = Other.Position;
					break;
				}
			}
		}
		if (!bWake)
		{
			continue;
		}
		if (Unit.Squad == INDEX_NONE)
		{
			Rouse(Unit, Place);
			++Loners;
		}
		else if (!Woken.Contains(Unit.Squad))
		{
			Woken.Add(Unit.Squad);
			WokenPlaces.Add(Place);
		}
	}
	if (Woken.Num() > 0)
	{
		for (FCrowdUnit& Unit : Units)
		{
			const int32 Which = Unit.Team == ECrowdTeam::Enemy ? Woken.IndexOfByKey(Unit.Squad) : INDEX_NONE;
			if (Which == INDEX_NONE)
			{
				continue;
			}
			if (Unit.bAsleep)
			{
				Rouse(Unit, WokenPlaces[Which]);
			}
			else if (Unit.bPosted && Unit.Alert != ECrowdAlert::Chase)
			{
				// One of the group that was looking or going back: it goes to the new place with the rest.
				Unit.Alert = ECrowdAlert::Seek;
				Unit.AlertTimer = Tuning.SeekSeconds;
			}
		}
	}
	return Woken.Num() + Loners;
}

int32 FCrowdSim::GetSummonCount() const
{
	const int32 Cost = FMath::Max(FMath::RoundToInt32(Tuning.BonesPerSkeleton), 1);
	const int32 Room = FMath::Max(Tuning.MaxSkeletons - CountTeam(ECrowdTeam::Army), 0);
	const int32 Count = FMath::Min(BonesHeld / Cost, Room);
	const int32 PerCast = FMath::RoundToInt32(Tuning.SummonMax);
	return PerCast > 0 ? FMath::Min(Count, PerCast) : Count;
}

int32 FCrowdSim::Summon(FVector2D WizardPosition)
{
	const int32 Cost = FMath::Max(FMath::RoundToInt32(Tuning.BonesPerSkeleton), 1);
	const int32 Count = GetSummonCount();
	int32 Raised = 0;
	for (; Raised < Count; ++Raised)
	{
		const double Angle = Random.FRandRange(0.0f, UE_TWO_PI);
		if (!AddSkeleton(WizardPosition + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * AssembleDistance))
		{
			break;
		}
		BonesHeld -= Cost;
	}
	return Raised;
}

int32 FCrowdSim::GetShopCost(ECrowdShopItem Item) const
{
	float Cost = 0.0f;
	switch (Item)
	{
	case ECrowdShopItem::Spear:
		Cost = Tuning.SpearUnlockCost;
		break;
	case ECrowdShopItem::Armor:
		Cost = Tuning.ArmorCost;
		break;
	case ECrowdShopItem::Blades:
		Cost = Tuning.BladesCost;
		break;
	case ECrowdShopItem::Focus:
		Cost = Tuning.FocusCost;
		break;
	case ECrowdShopItem::Marksman:
		Cost = Tuning.MarksmanCost;
		break;
	case ECrowdShopItem::Guard:
	default:
		Cost = Tuning.GuardCost;
		break;
	}
	return FMath::Max(FMath::RoundToInt32(Cost), 0);
}

int32 FCrowdSim::GetUnitWorth(ECrowdKind Kind) const
{
	const float Cost = Kind == ECrowdKind::Skeleton ? Tuning.BonesPerSkeleton : Kind == ECrowdKind::Guard ? Tuning.GuardCost
		: Kind == ECrowdKind::Marksman ? Tuning.MarksmanCost : 0.0f;
	if (Cost <= 0.0f || Tuning.ArmyValueShare <= 0.0f)
	{
		return 0;
	}
	// Always a whole number, and never nothing.
	return FMath::Max(FMath::RoundToInt32(Cost * Tuning.ArmyValueShare), 1);
}

FCrowdShopPrice FCrowdSim::GetShopPrice(ECrowdShopItem Item, int32 Count) const
{
	FCrowdShopPrice Price;
	const int32 Total = FMath::Max(Count, 0) * GetShopCost(Item);
	// Bones first, always.
	Price.Bones = FMath::Min(BonesHeld, Total);
	int32 Rest = Total - Price.Bones;

	// Then the plain skeletons with the wizard. Guards and Marksmen are never sacrificed (director, 2026-11-14).
	int32 Skeletons = 0;
	const int32 Guards = 0;
	for (const FCrowdUnit& Unit : Units)
	{
		if (Unit.Team == ECrowdTeam::Army && !Unit.bCommanded)
		{
			Skeletons += Unit.Kind == ECrowdKind::Skeleton ? 1 : 0;
		}
	}
	const int32 SkeletonWorth = GetUnitWorth(ECrowdKind::Skeleton);
	if (Rest > 0 && SkeletonWorth > 0)
	{
		Price.Skeletons = FMath::Min(Skeletons, FMath::DivideAndRoundUp(Rest, SkeletonWorth));
		Rest -= Price.Skeletons * SkeletonWorth;
	}
	const int32 GuardWorth = GetUnitWorth(ECrowdKind::Guard);
	if (Rest > 0 && GuardWorth > 0)
	{
		// The last one may be worth more than is owed. Nothing comes back.
		Price.Guards = FMath::Min(Guards, FMath::DivideAndRoundUp(Rest, GuardWorth));
		Rest -= Price.Guards * GuardWorth;
	}
	Price.bCanPay = Rest <= 0;
	return Price;
}

int32 FCrowdSim::GetShopMax(ECrowdShopItem Item) const
{
	if (!IsUnitItem(Item))
	{
		// Research is bought once.
		return !IsResearched(Item) && GetShopPrice(Item, 1).bCanPay ? 1 : 0;
	}
	// As many Guards as the bones held pay for and the army has room for. The army only ever pays the rest of ONE
	// purchase: with too few bones for a single Guard, one can still be bought with skeletons. So "Max" never
	// spends the army without the player seeing its price first.
	const int32 Room = FMath::Max(Tuning.MaxSkeletons - CountTeam(ECrowdTeam::Army), 0);
	const int32 Cost = GetShopCost(Item);
	const int32 WithBones = Cost > 0 ? FMath::Min(BonesHeld / Cost, Room) : Room;
	if (WithBones > 0)
	{
		return WithBones;
	}
	return Room > 0 && GetShopPrice(Item, 1).bCanPay ? 1 : 0;
}

int32 FCrowdSim::Buy(ECrowdShopItem Item, int32 Count, FVector2D WizardPosition)
{
	const int32 Wanted = FMath::Min(Count, GetShopMax(Item));
	if (Wanted <= 0)
	{
		return 0;
	}

	// Paid first: bones, then the army units nearest the wizard, who are given up and leave no bones.
	const FCrowdShopPrice Price = GetShopPrice(Item, Wanted);
	BonesHeld -= Price.Bones;
	const auto Sacrifice = [this, WizardPosition](ECrowdKind Kind, int32 Number)
	{
		for (int32 Taken = 0; Taken < Number; ++Taken)
		{
			int32 Best = INDEX_NONE;
			double BestSquared = TNumericLimits<double>::Max();
			for (int32 Index = 0; Index < Units.Num(); ++Index)
			{
				const FCrowdUnit& Unit = Units[Index];
				const double DistanceSquared = FVector2D::DistSquared(Unit.Position, WizardPosition);
				if (Unit.Team == ECrowdTeam::Army && Unit.Kind == Kind && !Unit.bCommanded && DistanceSquared < BestSquared)
				{
					BestSquared = DistanceSquared;
					Best = Index;
				}
			}
			if (Best == INDEX_NONE)
			{
				return;
			}
			Units.RemoveAtSwap(Best);
			++Sacrificed;
		}
	};
	Sacrifice(ECrowdKind::Skeleton, Price.Skeletons);
	Sacrifice(ECrowdKind::Guard, Price.Guards);
	if (Price.Skeletons + Price.Guards > 0)
	{
		// Indices moved; targets are chosen again at the start of the next step.
		for (FCrowdUnit& Unit : Units)
		{
			Unit.TargetIndex = INDEX_NONE;
		}
	}

	int32 Bought = 0;
	if (IsUnitItem(Item))
	{
		const ECrowdKind UnitKind = Item == ECrowdShopItem::Marksman ? ECrowdKind::Marksman : ECrowdKind::Guard;
		for (; Bought < Wanted; ++Bought)
		{
			const double Angle = Random.FRandRange(0.0f, UE_TWO_PI);
			if (!AddSkeleton(WizardPosition + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * AssembleDistance, UnitKind))
			{
				break;
			}
		}
	}
	else
	{
		Bought = 1;
		switch (Item)
		{
		case ECrowdShopItem::Spear:
			bSpearUnlocked = true;
			break;
		case ECrowdShopItem::Armor:
			bArmor = true;
			// The skeletons already in the army put the armor on as well.
			for (FCrowdUnit& Unit : Units)
			{
				if (Unit.Team == ECrowdTeam::Army && Unit.Kind == ECrowdKind::Skeleton)
				{
					Unit.MaxHealth += Tuning.ArmorHealth;
					Unit.Health += Tuning.ArmorHealth;
				}
			}
			break;
		case ECrowdShopItem::Blades:
			bBlades = true;
			break;
		case ECrowdShopItem::Focus:
		default:
			bFocus = true;
			break;
		}
	}
	return Bought;
}

const TCHAR* FCrowdSim::GetShopName(ECrowdShopItem Item)
{
	switch (Item)
	{
	case ECrowdShopItem::Spear:
		return TEXT("Spear");
	case ECrowdShopItem::Armor:
		return TEXT("Armor");
	case ECrowdShopItem::Blades:
		return TEXT("Blades");
	case ECrowdShopItem::Focus:
		return TEXT("Focus");
	case ECrowdShopItem::Marksman:
		return TEXT("Marksman");
	case ECrowdShopItem::Guard:
	default:
		return TEXT("Guard");
	}
}

FCrowdKindStats FCrowdSim::GetStats(ECrowdKind Kind) const
{
	switch (Kind)
	{
	case ECrowdKind::Brute:
		return { Tuning.BruteHealth, Tuning.BruteSpeed, Tuning.BruteDamagePerSecond };
	case ECrowdKind::Commander:
		return { Tuning.CommanderHealth, Tuning.CommanderSpeed, Tuning.CommanderDamagePerSecond };
	case ECrowdKind::Guard:
		return { Tuning.GuardHealth, Tuning.GuardSpeed, Tuning.GuardDamagePerSecond };
	case ECrowdKind::Archer:
		// Its damage is its arrows.
		return { Tuning.ArcherHealth, Tuning.ArcherSpeed, 0.0f };
	case ECrowdKind::Assassin:
		// It never fights the army; on the wizard it counts as several enemies.
		return { Tuning.AssassinHealth, Tuning.AssassinSpeed, 0.0f };
	case ECrowdKind::Marksman:
		// Its damage is its arrows.
		return { Tuning.MarksmanHealth, Tuning.MarksmanSpeed, 0.0f };
	case ECrowdKind::Skeleton:
		// Research: bone armor and bone blades.
		return { Tuning.SkeletonHealth + (bArmor ? Tuning.ArmorHealth : 0.0f), Tuning.SkeletonSpeed,
			Tuning.SkeletonDamagePerSecond + (bBlades ? Tuning.BladesDamage : 0.0f) };
	case ECrowdKind::Grunt:
	default:
		return { Tuning.GruntHealth, Tuning.GruntSpeed, Tuning.GruntDamagePerSecond };
	}
}

float FCrowdSim::GetUnitHeight(ECrowdKind Kind, float WizardHeight) const
{
	switch (Kind)
	{
	case ECrowdKind::Brute:
		return Tuning.BruteSize * WizardHeight;
	case ECrowdKind::Commander:
		return Tuning.CommanderSize * WizardHeight;
	case ECrowdKind::Guard:
		return Tuning.GuardSize * WizardHeight;
	case ECrowdKind::Archer:
		return Tuning.ArcherSize * WizardHeight;
	case ECrowdKind::Assassin:
		return Tuning.AssassinSize * WizardHeight;
	case ECrowdKind::Marksman:
		return Tuning.MarksmanSize * WizardHeight;
	case ECrowdKind::Skeleton:
		return Tuning.SkeletonSize * WizardHeight;
	case ECrowdKind::Grunt:
	default:
		return Tuning.GruntSize * WizardHeight;
	}
}

bool FCrowdSim::CanThrowSpear() const
{
	return bSpearUnlocked
		&& SpearCooldown <= 0.0f
		&& (BonesHeld >= FMath::RoundToInt32(Tuning.SpearBoneCost) || CountSpearSkeletons() >= FMath::RoundToInt32(Tuning.SpearSkeletonCost));
}

bool FCrowdSim::ThrowSpear(FVector2D From, FVector2D Target)
{
	if (!CanThrowSpear())
	{
		return false;
	}

	const int32 BoneCost = FMath::RoundToInt32(Tuning.SpearBoneCost);
	if (BonesHeld >= BoneCost)
	{
		// Bones first: with enough of them, nobody is taken.
		BonesHeld -= BoneCost;
	}
	else
	{
		// Not enough bones: the wizard cuts down the plain skeletons nearest him and makes the spear of theirs. No
		// other kind is ever taken, and nobody who was sent away.
		const int32 Cost = FMath::RoundToInt32(Tuning.SpearSkeletonCost);
		for (int32 Taken = 0; Taken < Cost; ++Taken)
		{
			int32 Best = INDEX_NONE;
			double BestSquared = TNumericLimits<double>::Max();
			for (int32 Index = 0; Index < Units.Num(); ++Index)
			{
				const FCrowdUnit& Unit = Units[Index];
				const double DistanceSquared = FVector2D::DistSquared(Unit.Position, From);
				if (Unit.Team == ECrowdTeam::Army && Unit.Kind == ECrowdKind::Skeleton && !Unit.bCommanded && DistanceSquared < BestSquared)
				{
					BestSquared = DistanceSquared;
					Best = Index;
				}
			}
			if (Best == INDEX_NONE)
			{
				break;
			}
			FCrowdSlash Slash;
			Slash.Position = Units[Best].Position;
			Slashes.Add(Slash);
			Units.RemoveAtSwap(Best);
		}
	}
	// Indices moved; targets are chosen again at the start of the next step.
	for (FCrowdUnit& Unit : Units)
	{
		Unit.TargetIndex = INDEX_NONE;
	}	// Indices moved; targets are chosen again at the start of the next step.
	for (FCrowdUnit& Unit : Units)
	{
		Unit.TargetIndex = INDEX_NONE;
	}
	SpearCooldown = Tuning.SpearCooldown;

	// It lands where he aims, no farther than its range and no farther than the first wall on the way.
	FVector2D Path = Target - From;
	const double Wanted = Path.Size();
	if (Wanted > Tuning.SpearRange)
	{
		Path *= Tuning.SpearRange / Wanted;
	}
	double WallAlpha = 1.0;
	for (const FCrowdObstacle& Obstacle : Obstacles)
	{
		double Alpha = 0.0;
		if (!Obstacle.bLow && Obstacle.SegmentEnters(From, From + Path, BoltWallMargin, Alpha))
		{
			WallAlpha = FMath::Min(WallAlpha, Alpha);
		}
	}

	FCrowdSpear Spear;
	Spear.Position = From;
	Spear.Target = From + Path * WallAlpha;
	Spears.Add(Spear);
	return true;
}

void FCrowdSim::MoveSpears(float DeltaSeconds, FCrowdStepResult& Result)
{
	SpearCooldown = FMath::Max(SpearCooldown - DeltaSeconds, 0.0f);

	for (int32 Index = Slashes.Num() - 1; Index >= 0; --Index)
	{
		Slashes[Index].Age += DeltaSeconds;
		if (Slashes[Index].Age >= Tuning.SlashSeconds)
		{
			Slashes.RemoveAtSwap(Index);
		}
	}
	for (int32 Index = SlamHits.Num() - 1; Index >= 0; --Index)
	{
		SlamHits[Index].Age += DeltaSeconds;
		if (SlamHits[Index].Age >= Tuning.SlamFlashSeconds)
		{
			SlamHits.RemoveAtSwap(Index);
		}
	}
	for (int32 Index = Blasts.Num() - 1; Index >= 0; --Index)
	{
		Blasts[Index].Age += DeltaSeconds;
		if (Blasts[Index].Age >= Tuning.BlastSeconds)
		{
			Blasts.RemoveAtSwap(Index);
		}
	}

	const double RadiusSquared = FMath::Square(static_cast<double>(Tuning.SpearRadius));
	for (int32 Index = Spears.Num() - 1; Index >= 0; --Index)
	{
		FCrowdSpear& Spear = Spears[Index];
		const FVector2D ToTarget = Spear.Target - Spear.Position;
		const double Distance = ToTarget.Size();
		const double Travel = Tuning.SpearSpeed * DeltaSeconds;
		if (Distance > Travel)
		{
			Spear.Position += ToTarget / Distance * Travel;
			continue;
		}

		// It lands and explodes: every enemy in the area takes the same damage. Enough to kill a weak enemy
		// outright, a fraction of a tough one's health.
		for (FCrowdUnit& Unit : Units)
		{
			if (Unit.Team == ECrowdTeam::Enemy && FVector2D::DistSquared(Unit.Position, Spear.Target) <= RadiusSquared)
			{
				Unit.Health -= Tuning.SpearDamage * GetDamageTaken(Unit, Spear.Target);
				Unit.HurtTimer = Tuning.HealthBarSeconds;
				Unit.LastHitFrom = Spear.Target;
				Unit.bWasHit = true;
			}
		}
		FCrowdBlast Blast;
		Blast.Position = Spear.Target;
		Blasts.Add(Blast);
		++Result.SpearsExploded;
		Spears.RemoveAtSwap(Index);
	}
}

bool FCrowdSim::AddSkeleton(FVector2D Position, ECrowdKind Kind)
{
	if (CountTeam(ECrowdTeam::Army) >= Tuning.MaxSkeletons)
	{
		return false;
	}
	FCrowdUnit Unit;
	Unit.Position = Position;
	Unit.Health = GetStats(Kind).Health;
	Unit.MaxHealth = Unit.Health;
	Unit.Team = ECrowdTeam::Army;
	Unit.Kind = Kind;
	Unit.bSettle = true;
	Unit.Id = NextId++;
	// Its first blow is wound up like any other. (An archer's timer is for its arrows.)
	Unit.AttackTimer = Kind == ECrowdKind::Archer ? 0.0f : FMath::Max(Tuning.MeleeInterval, 0.0f) * Tuning.MeleeFirstShare;
	PushOutOfObstacles(Unit.Position);
	Units.Add(Unit);
	return true;
}

bool FCrowdSim::DropBone(FVector2D Position, int32 Value, bool bGuard)
{
	if (Value <= 0 || Bones.Num() >= Tuning.MaxBones)
	{
		return false;
	}
	FCrowdBone Bone;
	Bone.Position = Position;
	Bone.Value = Value;
	Bone.Life = Tuning.BoneLife;
	Bone.bGuard = bGuard;
	Bones.Add(Bone);
	return true;
}

int32 FCrowdSim::GetGroupCount(int32 ArmySize) const
{
	if (ArmySize <= 0 || Tuning.GroupTargetSize <= 0.0f)
	{
		return 0;
	}
	const int32 Wanted = FMath::CeilToInt32(ArmySize / Tuning.GroupTargetSize);
	const int32 Least = FMath::Max(FMath::RoundToInt32(Tuning.GroupsMin), 1);
	const int32 Most = FMath::Max(FMath::RoundToInt32(Tuning.GroupsMax), Least);
	// Never more groups than skeletons.
	return FMath::Min(FMath::Clamp(Wanted, Least, Most), ArmySize);
}

void FCrowdSim::FireBolt(FVector2D From, FVector2D Direction)
{
	const FVector2D Normal = Direction.GetSafeNormal();
	if (Normal.IsNearlyZero())
	{
		return;
	}
	FCrowdBolt Bolt;
	Bolt.Position = From;
	Bolt.Velocity = Normal * Tuning.BoltSpeed;
	// It flies no farther than its range.
	Bolt.Life = Tuning.BoltRange > 0.0f && Tuning.BoltSpeed > 0.0f ? FMath::Min(Tuning.BoltLife, Tuning.BoltRange / Tuning.BoltSpeed) : Tuning.BoltLife;
	Bolts.Add(Bolt);
}

FCrowdStepResult FCrowdSim::Step(float DeltaSeconds, FVector2D WizardPosition)
{
	FCrowdStepResult Result;
	Events.Reset();
	const float ClampedSeconds = FMath::Clamp(DeltaSeconds, 0.0f, Tuning.MaxStep);
	if (ClampedSeconds <= 0.0f)
	{
		return Result;
	}

	// The way around walls is worked out again only when the wizard steps into another square.
	if (Obstacles.Num() > 0 && !Flow.IsBuiltFor(WizardPosition))
	{
		Flow.Build(WizardPosition, Obstacles, Tuning.UnitRadius);
	}

	// How the wizard is moving, for archers to aim ahead of him. A Blink is not a walk: nothing is reckoned from it.
	WizardVelocity = bHasLastWizardPosition && !bWizardInvulnerable ? (WizardPosition - LastWizardPosition) / ClampedSeconds : FVector2D::ZeroVector;
	if (WizardVelocity.SizeSquared() > FMath::Square(LeadSpeedMost))
	{
		WizardVelocity = FVector2D::ZeroVector;
	}
	LastWizardPosition = WizardPosition;
	bHasLastWizardPosition = true;
	for (FCrowdUnit& Unit : Units)
	{
		Unit.StepStart = Unit.Position;
		Unit.RepickTimer = FMath::Max(Unit.RepickTimer - ClampedSeconds, 0.0f);
	}

	Result.SquadsWoken = WakeGarrisons(WizardPosition);
	UpdateSight(ClampedSeconds, WizardPosition);
	UpdateCommands(WizardPosition, Result);
	ChooseTargets(WizardPosition);
	MoveUnits(ClampedSeconds, WizardPosition, Result);
	SeparateUnits();
	KeepOutOfObstacles();
	for (FCrowdUnit& Unit : Units)
	{
		Unit.Velocity = (Unit.Position - Unit.StepStart) / ClampedSeconds;
	}
	ApplyMelee(ClampedSeconds, WizardPosition, Result);
	UpdateFacing(ClampedSeconds, WizardPosition);
	Clock += ClampedSeconds;
	MoveBolts(ClampedSeconds);
	MoveArrows(ClampedSeconds, WizardPosition, Result);
	MoveJavelins(ClampedSeconds, WizardPosition, Result);
	MoveSpears(ClampedSeconds, Result);
	RemoveDead(Result);
	UpdateBones(ClampedSeconds, WizardPosition, Result);
	return Result;
}

int32 FCrowdSim::CountTeam(ECrowdTeam Team) const
{
	int32 Count = 0;
	for (const FCrowdUnit& Unit : Units)
	{
		Count += Unit.Team == Team ? 1 : 0;
	}
	return Count;
}

void FCrowdSim::GetEnemiesTouching(FVector2D WizardPosition, TArray<FVector2D>& OutOffsets) const
{
	OutOffsets.Reset();
	const double TouchSquared = FMath::Square(static_cast<double>(Tuning.TouchRange));
	for (const FCrowdUnit& Unit : Units)
	{
		// An archer does its harm with arrows, not by standing on him.
		if (Unit.Team == ECrowdTeam::Enemy && !Unit.bAsleep && Unit.Kind != ECrowdKind::Archer)
		{
			const FVector2D Offset = Unit.Position - WizardPosition;
			if (Offset.SizeSquared() <= TouchSquared)
			{
				// An assassin on the wizard counts as several enemies, and the commander as more than anything.
				const float Counts = Unit.Kind == ECrowdKind::Assassin ? Tuning.AssassinTouchWeight
					: Unit.Kind == ECrowdKind::Commander ? Tuning.CommanderTouchWeight : 1.0f;
				const int32 Weight = FMath::Max(FMath::RoundToInt32(Counts), 1);
				for (int32 Index = 0; Index < Weight; ++Index)
				{
					OutOffsets.Add(Offset);
				}
			}
		}
	}
}

void FCrowdSim::AddEvent(ECrowdEvent Type, FVector2D Position, ECrowdKind Kind, ECrowdTeam Team)
{
	FCrowdEvent& Event = Events.AddDefaulted_GetRef();
	Event.Type = Type;
	Event.Position = Position;
	Event.Kind = Kind;
	Event.Team = Team;
}

void FCrowdSim::Reset()
{
	Events.Reset();
	Units.Reset();
	Bolts.Reset();
	Arrows.Reset();
	Javelins.Reset();
	NextJavelinId = 1;
	Bones.Reset();
	Spears.Reset();
	Blasts.Reset();
	Slashes.Reset();
	SlamHits.Reset();
	Orders.Reset();
	Sights.Reset();
	GuardsOwed = 0;
	Sacrificed = 0;
	bArmor = false;
	bBlades = false;
	bFocus = false;
	bSpearUnlocked = false;
	SpearCooldown = 0.0f;
	BonesHeld = 0;
	bHasLastWizardPosition = false;
	WizardVelocity = FVector2D::ZeroVector;
}

void FCrowdSim::SetObstacles(const TArray<FCrowdObstacle>& InObstacles)
{
	Obstacles = InObstacles;
	Flow.Invalidate();
	for (FCrowdOrder& Order : Orders)
	{
		Order.Field.Build(Order.Point, Obstacles, Tuning.UnitRadius);
		Order.HuntField.Invalidate();
	}
	for (FCrowdSight& Sight : Sights)
	{
		Sight.SeekField.Build(Sight.LastSeen, Obstacles, Tuning.UnitRadius);
		Sight.bReturnBuilt = false;
	}
	for (FCrowdUnit& Unit : Units)
	{
		PushOutOfObstacles(Unit.Position);
	}
}

bool FCrowdSim::HasClearPath(FVector2D From, FVector2D To) const
{
	// Half a unit of room: a unit that brushes a corner slides along it, so the line need not be a full unit wide.
	const double Margin = Tuning.UnitRadius * 0.5;
	double Alpha = 0.0;
	for (const FCrowdObstacle& Obstacle : Obstacles)
	{
		if (Obstacle.SegmentEnters(From, To, Margin, Alpha))
		{
			return false;
		}
	}
	return true;
}

void FCrowdSim::ChooseTargets(FVector2D WizardPosition)
{
	const double BlockSquared = FMath::Square(static_cast<double>(Tuning.BlockRange));
	const double TouchSquared = FMath::Square(static_cast<double>(Tuning.TouchRange));

	for (int32 Index = 0; Index < Units.Num(); ++Index)
	{
		FCrowdUnit& Unit = Units[Index];
		Unit.TargetIndex = INDEX_NONE;
		const double FromWizardSquared = FVector2D::DistSquared(Unit.Position, WizardPosition);

		if (Unit.Team == ECrowdTeam::Army)
		{
			// The army stays with the wizard: a skeleton attacks only an enemy that is close to it and inside the
			// leash, and one that has strayed past the leash drops everything and comes back.
			// One that is already fighting gets a little more room on both limits. Without it, an enemy standing right
			// on a limit is in reach on one frame and out of reach on the next, and the skeleton shakes back and forth.
			// A unit that was sent somewhere is leashed to that place instead of to the wizard.
			if (Unit.Kind == ECrowdKind::Marksman)
			{
				// It never closes to fight: it keeps its place and shoots from it.
				Unit.bHunting = false;
				Unit.bFighting = false;
				continue;
			}
			if (Unit.bCommanded && Unit.bHunting)
			{
				// Gone after an enemy that shot at its group: nothing else matters until that is over.
				const int32 Order = FindOrder(Unit.CommandPoint);
				if (Orders.IsValidIndex(Order) && Units.IsValidIndex(Orders[Order].HuntIndex))
				{
					Unit.TargetIndex = Orders[Order].HuntIndex;
					Unit.bFighting = true;
					continue;
				}
				Unit.bHunting = false;
			}
			const FVector2D Anchor = Unit.bCommanded ? Unit.CommandPoint : WizardPosition;
			const double Slack = Unit.bFighting ? Tuning.LeashSlack : 0.0;
			const double UnitLeashSquared = FMath::Square(Tuning.LeashRadius + Slack);
			if (FVector2D::DistSquared(Unit.Position, Anchor) <= UnitLeashSquared)
			{
				double BestSquared = FMath::Square(Tuning.ArmyEngageRange + Slack);
				for (int32 OtherIndex = 0; OtherIndex < Units.Num(); ++OtherIndex)
				{
					const FCrowdUnit& Other = Units[OtherIndex];
					if (Other.Team != ECrowdTeam::Enemy || FVector2D::DistSquared(Other.Position, Anchor) > UnitLeashSquared
						|| IsHiddenFromArmy(Other, WizardPosition))
					{
						continue;
					}
					const double DistanceSquared = FVector2D::DistSquared(Unit.Position, Other.Position);
					if (DistanceSquared <= BestSquared && HasClearPath(Unit.Position, Other.Position))
					{
						BestSquared = DistanceSquared;
						Unit.TargetIndex = OtherIndex;
					}
				}
			}

			const bool bFightsNow = Unit.TargetIndex != INDEX_NONE;
			if (Unit.bFighting && !bFightsNow)
			{
				// The fight is over: idle from here instead of running back to an old spot.
				Unit.bSettle = true;
			}
			Unit.bFighting = bFightsNow;
			continue;
		}

		// An enemy at its post stands until its group is woken. An archer never closes to fight, and an assassin
		// never turns on the army.
		if (Unit.bAsleep || Unit.Kind == ECrowdKind::Archer || Unit.Kind == ECrowdKind::Assassin)
		{
			continue;
		}

		// Enemies charge the wizard. One that has reached him stays on him. On the way, one turns on a skeleton
		// only when the skeleton is close and in front of it, which is to say in its way.
		// One that has lost him does not know he is there: it fights whichever of his army is near it, and nothing else.
		const bool bLost = Unit.Alert != ECrowdAlert::Chase;
		if (!bLost && FromWizardSquared <= TouchSquared)
		{
			continue;
		}
		// With the wizard far away, it fights whichever of his army is near it: units sent to its post, or holding a
		// place it passes. With the wizard near, it goes for him and fights only what is in its way.
		// The commander never turns aside for them: he goes for the wizard, and slashes through what blocks him.
		if (Tuning.EnemyEngageRange > 0.0f
			&& (bLost || (Unit.Kind != ECrowdKind::Commander && FromWizardSquared > FMath::Square(static_cast<double>(Tuning.EnemyWizardRange)))))
		{
			double NearestSquared = FMath::Square(static_cast<double>(Tuning.EnemyEngageRange));
			for (int32 OtherIndex = 0; OtherIndex < Units.Num(); ++OtherIndex)
			{
				const FCrowdUnit& Other = Units[OtherIndex];
				if (Other.Team != ECrowdTeam::Army)
				{
					continue;
				}
				const double DistanceSquared = FVector2D::DistSquared(Unit.Position, Other.Position);
				if (DistanceSquared <= NearestSquared && HasClearPath(Unit.Position, Other.Position))
				{
					NearestSquared = DistanceSquared;
					Unit.TargetIndex = OtherIndex;
				}
			}
			if (Unit.TargetIndex != INDEX_NONE)
			{
				continue;
			}
		}
		if (bLost)
		{
			continue;
		}
		const FVector2D ToWizard = (WizardPosition - Unit.Position).GetSafeNormal();
		// A skeleton is in its way when their bodies all but touch.
		double BestSquared = FMath::Max(BlockSquared, FMath::Square(GetReach(Unit.Kind, ECrowdKind::Skeleton) + BlockGap));
		for (int32 OtherIndex = 0; OtherIndex < Units.Num(); ++OtherIndex)
		{
			const FCrowdUnit& Other = Units[OtherIndex];
			if (Other.Team != ECrowdTeam::Army)
			{
				continue;
			}
			const FVector2D Offset = Other.Position - Unit.Position;
			const double DistanceSquared = Offset.SizeSquared();
			if (DistanceSquared > BestSquared)
			{
				continue;
			}
			if (FVector2D::DotProduct(Offset, ToWizard) < Tuning.BlockCone * FMath::Sqrt(DistanceSquared))
			{
				continue;
			}
			BestSquared = DistanceSquared;
			Unit.TargetIndex = OtherIndex;
		}
	}
}

void FCrowdSim::MoveUnits(float DeltaSeconds, FVector2D WizardPosition, FCrowdStepResult& Result)
{
	for (FCrowdUnit& Unit : Units)
	{
		Unit.HurtTimer = FMath::Max(Unit.HurtTimer - DeltaSeconds, 0.0f);
		if (Unit.bAsleep)
		{
			MoveAtPost(Unit, DeltaSeconds);
			continue;
		}
		if (Unit.Kind == ECrowdKind::Archer && Unit.Alert == ECrowdAlert::Chase)
		{
			MoveArcher(Unit, DeltaSeconds, WizardPosition, Result);
			continue;
		}
		if (Unit.bStriking)
		{
			// In a blow, or a slam, it stands where it is: it does not follow what it strikes.
			continue;
		}

		FVector2D Goal = WizardPosition;
		// An enemy closes on the wizard until its body meets his.
		double StopDistance = FMath::Max(static_cast<double>(Tuning.MeleeRange), static_cast<double>(GetBodyRadius(Unit.Kind) + Tuning.WizardBodyRadius));
		double Speed = 0.0;
		bool bBendApproach = false;
		// The way around walls and water. The wizard's field leads to him, so a sent unit follows its order's own:
		// to the place it holds, or to the enemy part of its group has gone after.
		const FCrowdFlowField* Field = &Flow;
		const int32 Order = Unit.bCommanded ? FindOrder(Unit.CommandPoint) : INDEX_NONE;

		if (Units.IsValidIndex(Unit.TargetIndex))
		{
			Goal = Units[Unit.TargetIndex].Position;
			Speed = GetStats(Unit.Kind).Speed * (Unit.Team == ECrowdTeam::Army ? 1.0f : Unit.SpeedScale);
			// It closes until the two bodies meet.
			StopDistance = GetReach(Unit.Kind, Units[Unit.TargetIndex].Kind);
			if (Units[Unit.TargetIndex].Kind == ECrowdKind::Archer)
			{
				// An archer runs: closing only to the edge of reach, the chaser would be left a step behind forever.
				StopDistance *= RunnerCloseShare;
			}
			if (Unit.bCommanded)
			{
				Field = Unit.bHunting && Orders.IsValidIndex(Order) ? &Orders[Order].HuntField : nullptr;
			}
		}
		else if (Unit.bCommanded)
		{
			// Sent to hold a place: it goes there, and walks from spot to spot inside the group's circle.
			Unit.RoamTimer -= DeltaSeconds;
			if (Unit.RoamTimer <= 0.0f)
			{
				Unit.RoamTimer = Random.FRandRange(Tuning.RoamPauseMin, Tuning.RoamPauseMax);
				Unit.RoamOffset = PickCommandOffset(Unit);
			}
			Goal = Unit.CommandPoint + Unit.RoamOffset;
			if (Unit.RepickTimer <= 0.0f && IsSpotTaken(Unit, Goal))
			{
				// One of its own stands there: it stays where it is instead of pushing in.
				Unit.RepickTimer = RepickSeconds;
				Unit.RoamOffset = Unit.Position - Unit.CommandPoint;
				Goal = Unit.Position;
			}
			StopDistance = RoamArriveDistance;
			// Eased: a stroll between spots, a run on the way there.
			const double Fastest = GetStats(Unit.Kind).Speed;
			Speed = FMath::Clamp(FVector2D::Distance(Unit.Position, Goal) * Tuning.RoamEase,
				FMath::Min(static_cast<double>(Tuning.SkeletonRoamSpeed), Fastest), Fastest);
			Field = Orders.IsValidIndex(Order) ? &Orders[Order].Field : nullptr;
		}
		else if (Unit.Team == ECrowdTeam::Army)
		{
			UpdateRoam(Unit, DeltaSeconds, WizardPosition);
			Goal = WizardPosition + Unit.RoamOffset;
			if (Unit.RepickTimer <= 0.0f)
			{
				if (Obstacles.Num() > 0 && (IsInsideObstacle(Goal, Tuning.UnitRadius) || !HasClearPath(WizardPosition, Goal)))
				{
					// Its spot is in a wall, over water, or on the far side of either (the wizard has walked up to one).
					// It takes a spot on open ground on his side instead of pressing at the wall.
					Unit.RepickTimer = RepickSeconds;
					FVector2D Open = FVector2D::ZeroVector;
					Unit.RoamOffset = PickOpenOffset(WizardPosition, Open) ? Open : Unit.Position - WizardPosition;
					Goal = WizardPosition + Unit.RoamOffset;
				}
				else if (IsSpotTaken(Unit, Goal))
				{
					// One of its own stands there: it picks another spot instead of pushing in, and when that one is
					// taken as well it stays where it is.
					Unit.RepickTimer = RepickSeconds;
					Unit.RoamOffset = PickGroupOffset(Unit, WizardPosition);
					Goal = WizardPosition + Unit.RoamOffset;
					if (IsSpotTaken(Unit, Goal) && FVector2D::DistSquared(Unit.Position, WizardPosition) <= FMath::Square(static_cast<double>(Tuning.RoamRadius)))
					{
						Unit.RoamOffset = Unit.Position - WizardPosition;
						Goal = Unit.Position;
					}
				}
			}
			StopDistance = RoamArriveDistance;
			// Eased: a stroll when the spot is near, a run when the wizard has moved on.
			const double ToSpot = FVector2D::Distance(Unit.Position, Goal);
			const double Fastest = Tuning.SkeletonCatchUpSpeed;
			Speed = FMath::Clamp(ToSpot * Tuning.RoamEase, FMath::Min(static_cast<double>(Tuning.SkeletonRoamSpeed), Fastest), Fastest);
			if (Unit.Kind == ECrowdKind::Marksman && Unit.RelocateTimer > 0.0f)
			{
				// An enemy came close: it backs straight away from it, at its own speed.
				Goal = Unit.Position + Unit.RelocateAway * 300.0;
				Speed = GetStats(Unit.Kind).Speed;
				Field = nullptr;
			}
		}
		else if (Unit.Alert != ECrowdAlert::Chase)
		{
			// It has lost him: to where he was last seen, a look about, then back to where it stood.
			const int32 SightIndex = FindSight(GetSightKey(Unit));
			const FCrowdSight* Sight = Sights.IsValidIndex(SightIndex) ? &Sights[SightIndex] : nullptr;
			StopDistance = RoamArriveDistance;
			Field = nullptr;
			if (Unit.Alert == ECrowdAlert::Look || !Sight)
			{
				// It stands and turns its head from side to side of its own bearing.
				const double Angle = Unit.Id * 2.4 + FMath::Sin(Clock * LookRate + Unit.Id) * LookSweep;
				Unit.FaceGoal = FVector2D(FMath::Cos(Angle), FMath::Sin(Angle));
				Goal = Unit.Position;
			}
			else if (Unit.Alert == ECrowdAlert::Seek)
			{
				Goal = Sight->LastSeen;
				Field = &Sight->SeekField;
				Speed = GetStats(Unit.Kind).Speed * Unit.SpeedScale;
			}
			else
			{
				Goal = Unit.ReturnPoint;
				Field = Sight->bReturnBuilt ? &Sight->ReturnField : nullptr;
				Speed = GetStats(Unit.Kind).Speed * Unit.SpeedScale;
			}
		}
		else
		{
			Speed = GetStats(Unit.Kind).Speed * Unit.SpeedScale;
			bBendApproach = true;
		}

		const FVector2D ToGoal = Goal - Unit.Position;
		const double Distance = ToGoal.Size();
		if (Distance <= StopDistance || Distance <= UE_KINDA_SMALL_NUMBER || Speed <= 0.0)
		{
			continue;
		}

		FVector2D Direction = ToGoal / Distance;
		double Travel = FMath::Min(Speed * DeltaSeconds, Distance - StopDistance);
		if (Field && Obstacles.Num() > 0 && !HasClearPath(Unit.Position, Goal))
		{
			// A wall is in the way: follow the field, which leads around walls.
			const FVector2D Around = Field->GetDirection(Unit.Position);
			if (!Around.IsNearlyZero())
			{
				Direction = Around;
				Travel = Speed * DeltaSeconds;
			}
		}
		else if (bBendApproach)
		{
			// Each enemy leans to its own side on the way in, less and less as it closes, so a wave arcs around the wizard.
			const double Fade = FMath::Clamp((Distance - Tuning.FlankNear) / FMath::Max(Tuning.FlankFar - Tuning.FlankNear, 1.0f), 0.0, 1.0);
			Direction = Direction.GetRotated(Unit.Flank * Tuning.EnemyFlankDegrees * Fade);
		}
		if (Unit.Kind == ECrowdKind::Assassin && Tuning.AssassinAvoidRange > 0.0f)
		{
			// It swerves around any of the army in its way, to its own side, harder the closer they are.
			const FVector2D Sideways(-Direction.Y, Direction.X);
			double Swerve = 0.0;
			for (const FCrowdUnit& Other : Units)
			{
				if (Other.Team != ECrowdTeam::Army)
				{
					continue;
				}
				const FVector2D Offset = Other.Position - Unit.Position;
				const double Gap = Offset.Size();
				if (Gap >= Tuning.AssassinAvoidRange || FVector2D::DotProduct(Offset, Direction) <= 0.0)
				{
					continue;
				}
				// Away from the side the skeleton is on; dead ahead, to the side this assassin leans.
				const double Across = FVector2D::DotProduct(Offset, Sideways);
				const double Side = FMath::Abs(Across) > 1.0 ? -FMath::Sign(Across) : (Unit.Flank >= 0.0f ? 1.0 : -1.0);
				Swerve += Side * (1.0 - Gap / Tuning.AssassinAvoidRange) * AssassinSwerve;
			}
			if (Swerve != 0.0)
			{
				Direction = (Direction + Sideways * Swerve).GetSafeNormal();
				Travel = Speed * DeltaSeconds;
			}
		}
		if (Unit.Kind != ECrowdKind::Assassin && Tuning.SeparationRadius > 0.0f && Travel > 0.0)
		{
			// It walks around one of its own side that stands in its way, instead of pushing through.
			const double AvoidRange = Tuning.SeparationRadius * AvoidRangeShare;
			const FVector2D Sideways(-Direction.Y, Direction.X);
			double Swerve = 0.0;
			for (const FCrowdUnit& Other : Units)
			{
				if (Other.Team != Unit.Team || Other.bMoving || Other.Id == Unit.Id)
				{
					continue;
				}
				const FVector2D Offset = Other.Position - Unit.Position;
				const double Gap = Offset.Size();
				// Not one behind it, and not one that stands past where it is going.
				if (Gap >= AvoidRange || Gap >= Distance || FVector2D::DotProduct(Offset, Direction) <= 0.0)
				{
					continue;
				}
				const double Across = FVector2D::DotProduct(Offset, Sideways);
				const double Side = FMath::Abs(Across) > 1.0 ? -FMath::Sign(Across) : (Unit.Id % 2 == 0 ? 1.0 : -1.0);
				Swerve += Side * (1.0 - Gap / AvoidRange) * AvoidSwerve;
			}
			if (Swerve != 0.0)
			{
				Direction = (Direction + Sideways * Swerve).GetSafeNormal();
			}
		}
		Unit.Position += Direction * Travel;
	}
}

bool FCrowdSim::HasClearShot(FVector2D From, FVector2D To) const
{
	double Alpha = 0.0;
	for (const FCrowdObstacle& Obstacle : Obstacles)
	{
		if (!Obstacle.bLow && Obstacle.SegmentEnters(From, To, BoltWallMargin, Alpha))
		{
			return false;
		}
	}
	return true;
}

void FCrowdSim::MoveArcher(FCrowdUnit& Unit, float DeltaSeconds, FVector2D WizardPosition, FCrowdStepResult& Result)
{
	Unit.bAiming = false;
	Unit.AttackTimer = FMath::Max(Unit.AttackTimer - DeltaSeconds, 0.0f);
	const double Speed = GetStats(Unit.Kind).Speed * Unit.SpeedScale;

	// The nearest of the wizard and his army.
	FVector2D Nearest = WizardPosition;
	double NearestSquared = FVector2D::DistSquared(Unit.Position, WizardPosition);
	for (const FCrowdUnit& Other : Units)
	{
		if (Other.Team == ECrowdTeam::Army)
		{
			const double DistanceSquared = FVector2D::DistSquared(Unit.Position, Other.Position);
			if (DistanceSquared < NearestSquared)
			{
				NearestSquared = DistanceSquared;
				Nearest = Other.Position;
			}
		}
	}

	// Charged: it runs, and keeps running until it is well clear. bFighting marks an archer on the run.
	const double FleeSquared = FMath::Square(static_cast<double>(Tuning.ArcherFleeRange));
	if (NearestSquared < FleeSquared)
	{
		Unit.bFighting = true;
	}
	else if (NearestSquared > FleeSquared * FMath::Square(ArcherSafeShare))
	{
		Unit.bFighting = false;
	}
	if (Unit.bFighting)
	{
		FVector2D Away = (Unit.Position - Nearest).GetSafeNormal();
		if (Away.IsNearlyZero())
		{
			Away = FVector2D(1.0, 0.0);
		}
		Unit.Position += Away * (Speed * DeltaSeconds);
		return;
	}

	// It shoots the nearest target in range that no wall hides: the wizard, or one of his army.
	const double RangeSquared = FMath::Square(static_cast<double>(Tuning.ArcherRange));
	bool bHasTarget = false;
	FVector2D Target = WizardPosition;
	FVector2D TargetVelocity = FVector2D::ZeroVector;
	double TargetSquared = RangeSquared;
	const auto Consider = [&](FVector2D Position, FVector2D Velocity)
	{
		const double DistanceSquared = FVector2D::DistSquared(Unit.Position, Position);
		if (DistanceSquared <= TargetSquared && HasClearShot(Unit.Position, Position))
		{
			TargetSquared = DistanceSquared;
			Target = Position;
			TargetVelocity = Velocity;
			bHasTarget = true;
		}
	};
	Consider(WizardPosition, WizardVelocity);
	for (const FCrowdUnit& Other : Units)
	{
		if (Other.Team == ECrowdTeam::Army)
		{
			Consider(Other.Position, Other.Velocity);
		}
	}
	if (bHasTarget)
	{
		Unit.bAiming = true;
		// It aims where the target will be when the arrow gets there, going by how the target moves right now, so a
		// change of direction after the shot dodges it. Reckoned twice, which is close enough.
		FVector2D Ahead = Target;
		if (Tuning.ArrowSpeed > 0.0f && Tuning.ArcherLead > 0.0f)
		{
			for (int32 Pass = 0; Pass < 2; ++Pass)
			{
				Ahead = Target + TargetVelocity * (FVector2D::Distance(Unit.Position, Ahead) / Tuning.ArrowSpeed * Tuning.ArcherLead);
			}
		}
		// And no archer is a perfect shot: each arrow is off a little to one side or the other.
		const FVector2D Aim = (Ahead - Unit.Position).GetSafeNormal().GetRotated(Random.FRandRange(-Tuning.ArcherSpreadDegrees, Tuning.ArcherSpreadDegrees));
		if (Unit.AttackTimer <= 0.0f && !Aim.IsNearlyZero())
		{
			// It is lobbed at that place: over whatever stands between, and down on whoever is there when it lands.
			const double Flight = FVector2D::Distance(Unit.Position, Ahead);
			FCrowdArrow Arrow;
			Arrow.Shooter = Unit.Id;
			Arrow.Position = Unit.Position;
			Arrow.Start = Unit.Position;
			Arrow.Target = Unit.Position + Aim * Flight;
			Arrow.Velocity = Aim * Tuning.ArrowSpeed;
			Arrow.Life = Tuning.ArrowSpeed > 0.0f ? static_cast<float>(Flight / Tuning.ArrowSpeed) + ArrowSpareSeconds : 0.0f;
			Arrows.Add(Arrow);
			Unit.AttackTimer = Tuning.ArcherInterval;
			++Result.ArrowsFired;
			AddEvent(ECrowdEvent::ArrowLoosed, Unit.Position, Unit.Kind, Unit.Team);
		}
		return;
	}

	// Nothing to shoot: it closes on the wizard, around walls, until he is well inside its range.
	const FVector2D ToWizard = WizardPosition - Unit.Position;
	const double Distance = ToWizard.Size();
	const double StopDistance = Tuning.ArcherRange * ArcherApproachShare;
	if (Distance <= StopDistance || Speed <= 0.0)
	{
		return;
	}
	FVector2D Direction = ToWizard / Distance;
	if (Obstacles.Num() > 0 && !HasClearPath(Unit.Position, WizardPosition))
	{
		const FVector2D Around = Flow.GetDirection(Unit.Position);
		if (!Around.IsNearlyZero())
		{
			Direction = Around;
		}
	}
	Unit.Position += Direction * FMath::Min(Speed * DeltaSeconds, Distance - StopDistance);
}

void FCrowdSim::MoveArrows(float DeltaSeconds, FVector2D WizardPosition, FCrowdStepResult& Result)
{
	const double RadiusSquared = FMath::Square(static_cast<double>(Tuning.ArrowRadius));
	for (int32 ArrowIndex = Arrows.Num() - 1; ArrowIndex >= 0; --ArrowIndex)
	{
		FCrowdArrow& Arrow = Arrows[ArrowIndex];
		const FVector2D Start = Arrow.Position;
		// It comes down at its mark and goes no farther.
		const double Left = FVector2D::Distance(Start, Arrow.Target);
		const double Wanted = Arrow.Velocity.Size() * DeltaSeconds;
		const bool bLands = Wanted >= Left;
		const FVector2D Travel = bLands ? Arrow.Target - Start : Arrow.Velocity * DeltaSeconds;
		const double TravelSquared = Travel.SizeSquared();

		// Swept along this step's path, like a Bolt. Nothing past a wall can be hit.
		double WallAlpha = TNumericLimits<double>::Max();
		for (const FCrowdObstacle& Obstacle : Obstacles)
		{
			double Alpha = 0.0;
			if (!Obstacle.bLow && Obstacle.SegmentEnters(Start, Start + Travel, BoltWallMargin, Alpha))
			{
				WallAlpha = FMath::Min(WallAlpha, Alpha);
			}
		}
		const auto Meets = [&](FVector2D Position, double& OutAlpha)
		{
			const FVector2D ToTarget = Position - Start;
			OutAlpha = TravelSquared > UE_KINDA_SMALL_NUMBER ? FMath::Clamp(FVector2D::DotProduct(ToTarget, Travel) / TravelSquared, 0.0, 1.0) : 0.0;
			return (ToTarget - Travel * OutAlpha).SizeSquared() <= RadiusSquared;
		};

		// High over their heads for most of its flight: it can hit only near its mark, on the way down. Whoever is
		// there then is hit, so one who moves out is missed and one who moves in is not.
		const bool bLow = Tuning.ArrowDropDistance <= 0.0f || Left - FMath::Min(Wanted, Left) <= Tuning.ArrowDropDistance;
		double HitAlpha = WallAlpha;
		int32 HitIndex = INDEX_NONE;
		bool bHitWizard = false;
		double Alpha = 0.0;
		if (bLow)
		{
			// During the blink an enemy's arrow flies through him.
			if (!Arrow.bArmy && !bWizardInvulnerable && Meets(WizardPosition, Alpha) && Alpha < HitAlpha)
			{
				HitAlpha = Alpha;
				bHitWizard = true;
			}
			const ECrowdTeam Hits = Arrow.bArmy ? ECrowdTeam::Enemy : ECrowdTeam::Army;
			for (int32 UnitIndex = 0; UnitIndex < Units.Num(); ++UnitIndex)
			{
				const FCrowdUnit& Unit = Units[UnitIndex];
				if (Unit.Team == Hits && Unit.Health > 0.0f && Meets(Unit.Position, Alpha) && Alpha < HitAlpha)
				{
					HitAlpha = Alpha;
					HitIndex = UnitIndex;
					bHitWizard = false;
				}
			}
		}

		if (HitIndex != INDEX_NONE)
		{
			FCrowdUnit& Hit = Units[HitIndex];
			Hit.LastHitFrom = Arrow.Start;
			Hit.bWasHit = true;
			if (Arrow.bArmy)
			{
				// A Marksman's arrow: an archer dies to one; a brute takes a little more than a skeleton's blow, less its armor.
				const float Damage = Hit.Kind == ECrowdKind::Archer ? Hit.Health
					: (Hit.Kind == ECrowdKind::Brute ? Tuning.MarksmanBruteDamage : Tuning.MarksmanDamage) * Arrow.DamageScale * GetDamageTaken(Hit, Arrow.Start);
				Hit.Health -= Damage;
			}
			else
			{
				Hit.Health -= Tuning.ArrowDamage * (IsInPresence(Hit, WizardPosition) ? 1.0f : Tuning.AwayDamageTaken);
				if (Hit.bCommanded)
				{
					// Shot at where they hold: some of the group go after the shooter.
					RespondToShooter(Hit.CommandPoint, Arrow.Shooter);
				}
			}
			Units[HitIndex].HurtTimer = Tuning.HealthBarSeconds;
			Arrows.RemoveAtSwap(ArrowIndex);
			continue;
		}
		if (bHitWizard)
		{
			++Result.ArrowsHitWizard;
			Arrows.RemoveAtSwap(ArrowIndex);
			continue;
		}
		if (WallAlpha <= 1.0 || bLands)
		{
			Arrows.RemoveAtSwap(ArrowIndex);
			continue;
		}
		Arrow.Position = Start + Travel;
		Arrow.Life -= DeltaSeconds;
		if (Arrow.Life <= 0.0f)
		{
			Arrows.RemoveAtSwap(ArrowIndex);
		}
	}
}

float FCrowdSim::GetArrowHeight(const FCrowdArrow& Arrow) const
{
	const double Flight = FVector2D::Distance(Arrow.Start, Arrow.Target);
	if (Flight < 1.0)
	{
		return 0.0f;
	}
	const double Along = FMath::Clamp(FVector2D::Distance(Arrow.Start, Arrow.Position) / Flight, 0.0, 1.0);
	return static_cast<float>(Tuning.ArrowArc * Flight * 4.0 * Along * (1.0 - Along));
}

float FCrowdSim::GetArrowSlope(const FCrowdArrow& Arrow) const
{
	const double Flight = FVector2D::Distance(Arrow.Start, Arrow.Target);
	if (Flight < 1.0)
	{
		return 0.0f;
	}
	const double Along = FMath::Clamp(FVector2D::Distance(Arrow.Start, Arrow.Position) / Flight, 0.0, 1.0);
	return static_cast<float>(Tuning.ArrowArc * 4.0 * (1.0 - 2.0 * Along));
}

void FCrowdSim::UpdateMarksman(FCrowdUnit& Unit, float DeltaSeconds, FVector2D WizardPosition, FCrowdStepResult& Result)
{
	Unit.bStriking = false;
	Unit.AttackTimer = FMath::Max(Unit.AttackTimer - DeltaSeconds, 0.0f);
	Unit.RelocateTimer = FMath::Max(Unit.RelocateTimer - DeltaSeconds, 0.0f);
	if (Tuning.ArrowSpeed <= 0.0f)
	{
		return;
	}

	// Archers first, the nearest of them. With none in range, the enemy with the most health. And the nearest enemy
	// of any kind that has come close to it.
	const double RangeSquared = FMath::Square(static_cast<double>(Tuning.MarksmanRange));
	int32 Best = INDEX_NONE;
	bool bBestArcher = false;
	double BestScore = 0.0;
	int32 Close = INDEX_NONE;
	double CloseSquared = FMath::Square(static_cast<double>(Tuning.MarksmanKeepAway));
	for (int32 Index = 0; Index < Units.Num(); ++Index)
	{
		const FCrowdUnit& Other = Units[Index];
		if (Other.Team != ECrowdTeam::Enemy || Other.Health <= 0.0f || Other.bAsleep || IsHiddenFromArmy(Other, WizardPosition))
		{
			continue;
		}
		const double DistanceSquared = FVector2D::DistSquared(Unit.Position, Other.Position);
		if (DistanceSquared < CloseSquared)
		{
			Close = Index;
			CloseSquared = DistanceSquared;
		}
		if (DistanceSquared > RangeSquared)
		{
			continue;
		}
		const bool bArcher = Other.Kind == ECrowdKind::Archer;
		const double Score = bArcher ? -DistanceSquared : static_cast<double>(Other.Health);
		const bool bBetter = Best == INDEX_NONE || (bArcher && !bBestArcher) || (bArcher == bBestArcher && Score > BestScore);
		if (bBetter && HasClearShot(Unit.Position, Other.Position))
		{
			Best = Index;
			bBestArcher = bArcher;
			BestScore = Score;
		}
	}

	// It tries to keep away from the enemy. Each time one comes close it decides once: back off, or stand its ground.
	if (Close == INDEX_NONE)
	{
		Unit.bThreatened = false;
	}
	else if (!Unit.bThreatened)
	{
		Unit.bThreatened = true;
		if (Tuning.MarksmanRelocateSeconds > 0.0f && Random.FRand() < Tuning.MarksmanRelocateChance)
		{
			const FVector2D Away = Unit.Position - Units[Close].Position;
			Unit.RelocateAway = Away.IsNearlyZero() ? (Unit.Position - WizardPosition).GetSafeNormal() : Away.GetSafeNormal();
			Unit.RelocateTimer = Tuning.MarksmanRelocateSeconds;
		}
	}
	if (Unit.RelocateTimer > 0.0f)
	{
		// Backing off: it does not shoot while it moves.
		return;
	}
	if (Best == INDEX_NONE)
	{
		return;
	}
	// It stands to shoot: from when it starts to draw until the arrow leaves it does not follow the wizard.
	if (Unit.AttackTimer <= Tuning.MarksmanDraw)
	{
		Unit.bStriking = true;
		Unit.FaceGoal = (Units[Best].Position - Unit.Position).GetSafeNormal();
	}
	if (Unit.AttackTimer > 0.0f)
	{
		return;
	}

	// Aimed where the target will be when the arrow lands, going by how it moves now.
	const FCrowdUnit& Target = Units[Best];
	FVector2D Ahead = Target.Position;
	for (int32 Pass = 0; Pass < 2; ++Pass)
	{
		Ahead = Target.Position + Target.Velocity * (FVector2D::Distance(Unit.Position, Ahead) / Tuning.ArrowSpeed * Tuning.ArcherLead);
	}
	const FVector2D Aim = (Ahead - Unit.Position).GetSafeNormal();
	if (Aim.IsNearlyZero())
	{
		return;
	}
	const double Flight = FVector2D::Distance(Unit.Position, Ahead);
	FCrowdArrow Arrow;
	Arrow.Shooter = Unit.Id;
	Arrow.Position = Unit.Position;
	Arrow.Start = Unit.Position;
	Arrow.Target = Ahead;
	Arrow.Velocity = Aim * Tuning.ArrowSpeed;
	Arrow.Life = static_cast<float>(Flight / Tuning.ArrowSpeed) + ArrowSpareSeconds;
	Arrow.bArmy = true;
	Arrow.DamageScale = IsInPresence(Unit, WizardPosition) ? 1.0f : Tuning.AwayDamageShare;
	Arrows.Add(Arrow);
	Unit.AttackTimer = Tuning.MarksmanInterval;
	Unit.FaceGoal = Aim;
	++Result.ArmyArrowsFired;
	AddEvent(ECrowdEvent::ArrowLoosed, Unit.Position, Unit.Kind, Unit.Team);
}

FVector2D FCrowdSim::GetThrowAimPoint(const FCrowdUnit& Unit, FVector2D WizardPosition) const
{
	if (!Unit.bThrowLeads || Tuning.CommanderSpearSpeed <= 0.0f)
	{
		return WizardPosition;
	}
	// Where he will be when the spear gets there, if he keeps going as he is: the rest of the draw, then the flight.
	// Reckoned twice, which is close enough.
	const double Wait = FMath::Max(Tuning.CommanderSpearWindup - FMath::Max(Unit.ThrowWind, 0.0f), 0.0f);
	FVector2D Ahead = WizardPosition;
	for (int32 Pass = 0; Pass < 2; ++Pass)
	{
		Ahead = WizardPosition + WizardVelocity * (Wait + FVector2D::Distance(Unit.Position, Ahead) / Tuning.CommanderSpearSpeed);
	}
	return Ahead;
}

bool FCrowdSim::GetStrike(const FCrowdUnit& Unit, float& OutTimeToBlow) const
{
	OutTimeToBlow = 0.0f;
	if (Unit.Health <= 0.0f || Unit.bAsleep)
	{
		return false;
	}
	if (Unit.Team == ECrowdTeam::Enemy && (Unit.Kind == ECrowdKind::Brute || Unit.Kind == ECrowdKind::Commander))
	{
		if (IsSlamming(Unit))
		{
			const FSlamShape Shape = MakeSlamShape(Tuning, Unit);
			OutTimeToBlow = Shape.Grow + Shape.Hold - Unit.SlamTime;
			return true;
		}
		if (Unit.BlowAgo >= 0.0f)
		{
			OutTimeToBlow = -Unit.BlowAgo;
			return true;
		}
		return false;
	}
	if (Unit.Kind == ECrowdKind::Archer)
	{
		OutTimeToBlow = Unit.AttackTimer;
		return Unit.bAiming;
	}
	if (Unit.Kind == ECrowdKind::Marksman)
	{
		// It has shot and the next arrow is coming: it draws again. With nothing to shoot at, its timer runs out and it stands.
		OutTimeToBlow = Unit.AttackTimer;
		return Unit.AttackTimer > 0.0f;
	}
	OutTimeToBlow = Unit.AttackTimer;
	return Unit.Kind == ECrowdKind::Assassin ? Unit.bSwinging : Unit.bStriking;
}

bool FCrowdSim::GetThrow(const FCrowdUnit& Unit, float& OutTimeToThrow) const
{
	OutTimeToThrow = 0.0f;
	if (Unit.Kind != ECrowdKind::Commander || Unit.Health <= 0.0f)
	{
		return false;
	}
	if (IsWindingThrow(Unit))
	{
		OutTimeToThrow = Tuning.CommanderSpearWindup - Unit.ThrowWind;
		return true;
	}
	if (Unit.ThrowAgo >= 0.0f)
	{
		OutTimeToThrow = -Unit.ThrowAgo;
		return true;
	}
	return false;
}

void FCrowdSim::UpdateAssassinSwing(FCrowdUnit& Unit, float DeltaSeconds, FVector2D WizardPosition)
{
	// Being on the wizard is what an assassin does to him (the meter counts it). Its swing changes none of that: it
	// is what is seen and heard. It swings while it is on him; a swing that ends with him gone, or mid-blink, misses.
	const float Interval = Tuning.MeleeInterval;
	if (Interval <= 0.0f)
	{
		Unit.bSwinging = false;
		return;
	}
	const bool bInReach = !Unit.bAsleep && FVector2D::DistSquared(Unit.Position, WizardPosition) <= FMath::Square(static_cast<double>(Tuning.TouchRange));
	if (!Unit.bSwinging)
	{
		if (!bInReach)
		{
			Unit.AttackTimer = Interval * Tuning.MeleeFirstShare;
			return;
		}
		Unit.bSwinging = true;
	}
	Unit.AttackTimer -= DeltaSeconds;
	if (Unit.AttackTimer > 0.0f)
	{
		return;
	}
	AddEvent(bInReach && !bWizardInvulnerable ? ECrowdEvent::AssassinBlow : ECrowdEvent::AssassinMiss, Unit.Position, Unit.Kind, Unit.Team);
	Unit.bSwinging = bInReach;
	Unit.AttackTimer = bInReach ? Unit.AttackTimer + Interval : Interval * Tuning.MeleeFirstShare;
}

bool FCrowdSim::UpdateThrow(FCrowdUnit& Unit, float DeltaSeconds, FVector2D WizardPosition, FCrowdStepResult& Result)
{
	Unit.ThrowTimer = FMath::Max(Unit.ThrowTimer - DeltaSeconds, 0.0f);
	if (Unit.ThrowAgo >= 0.0f)
	{
		// Following the throw through.
		Unit.ThrowAgo += DeltaSeconds;
		if (Unit.ThrowAgo >= ThrowFollowSeconds)
		{
			Unit.ThrowAgo = -1.0f;
		}
	}
	if (Unit.bAsleep || Unit.Alert != ECrowdAlert::Chase || Tuning.CommanderSpearInterval <= 0.0f || Tuning.CommanderSpearSpeed <= 0.0f)
	{
		Unit.ThrowWind = -1.0f;
		return false;
	}
	const FVector2D ToWizard = WizardPosition - Unit.Position;
	if (Unit.ThrowWind < 0.0f)
	{
		// He starts one when he is not in a slash, the wizard is far enough and near enough, and no wall is between them.
		const double Distance = ToWizard.Size();
		if (IsSlamming(Unit) || Unit.ThrowTimer > 0.0f || bWizardInvulnerable
			|| Distance < Tuning.CommanderSpearMinRange || Distance > Tuning.CommanderSpearRange || !HasClearShot(Unit.Position, WizardPosition))
		{
			return false;
		}
		Unit.ThrowWind = 0.0f;
		Unit.ThrowAim = FVector2D::ZeroVector;
		// Sometimes he throws at where the wizard is going, not where he is.
		Unit.bThrowLeads = Tuning.CommanderSpearLeadChance > 0.0f && Random.FRand() < Tuning.CommanderSpearLeadChance;
	}

	// He stands while he draws back. The way the spear will go follows the wizard, then holds for the last moment:
	// that is the warning, and stepping out of it then is the dodge.
	Unit.bStriking = true;
	if (!IsThrowLocked(Unit) || Unit.ThrowAim.IsNearlyZero())
	{
		const FVector2D ToAim = GetThrowAimPoint(Unit, WizardPosition) - Unit.Position;
		Unit.ThrowAim = ToAim.IsNearlyZero() ? (ToWizard.IsNearlyZero() ? FVector2D(1.0, 0.0) : ToWizard.GetSafeNormal()) : ToAim.GetSafeNormal();
	}
	Unit.FaceGoal = Unit.ThrowAim;
	Unit.ThrowWind += DeltaSeconds;
	if (Unit.ThrowWind < Tuning.CommanderSpearWindup)
	{
		return true;
	}

	// Thrown the way the warning showed: a step out of it, or a blink, gets him out of its way.
	FCrowdJavelin Javelin;
	Javelin.Position = Unit.Position;
	Javelin.Velocity = Unit.ThrowAim * Tuning.CommanderSpearSpeed;
	Javelin.Life = static_cast<float>(Tuning.CommanderSpearRange * JavelinReachShare / Tuning.CommanderSpearSpeed);
	Javelin.Id = NextJavelinId++;
	Javelins.Add(Javelin);
	Unit.ThrowWind = -1.0f;
	Unit.ThrowAgo = 0.0f;
	Unit.ThrowTimer = Tuning.CommanderSpearInterval;
	++Result.JavelinsThrown;
	AddEvent(ECrowdEvent::SpearThrown, Unit.Position, Unit.Kind, Unit.Team);
	Events.Last().Id = Javelin.Id;
	return true;
}

void FCrowdSim::MoveJavelins(float DeltaSeconds, FVector2D WizardPosition, FCrowdStepResult& Result)
{
	const double RadiusSquared = FMath::Square(static_cast<double>(Tuning.CommanderSpearRadius));
	for (int32 Index = Javelins.Num() - 1; Index >= 0; --Index)
	{
		FCrowdJavelin& Javelin = Javelins[Index];
		const FVector2D Start = Javelin.Position;
		const FVector2D Travel = Javelin.Velocity * DeltaSeconds;
		const double TravelSquared = Travel.SizeSquared();
		double WallAlpha = TNumericLimits<double>::Max();
		for (const FCrowdObstacle& Obstacle : Obstacles)
		{
			double Alpha = 0.0;
			if (!Obstacle.bLow && Obstacle.SegmentEnters(Start, Start + Travel, BoltWallMargin, Alpha))
			{
				WallAlpha = FMath::Min(WallAlpha, Alpha);
			}
		}
		// It goes through the army, hitting each of them once: only the wizard stops it, and a blink lets it through him.
		const FVector2D ToWizard = WizardPosition - Start;
		const double Along = TravelSquared > UE_KINDA_SMALL_NUMBER ? FMath::Clamp(FVector2D::DotProduct(ToWizard, Travel) / TravelSquared, 0.0, 1.0) : 0.0;
		const bool bHitsWizard = !bWizardInvulnerable && Along < WallAlpha && (ToWizard - Travel * Along).SizeSquared() <= RadiusSquared;
		const double Reach = bHitsWizard ? Along : FMath::Min(WallAlpha, 1.0);
		for (FCrowdUnit& Unit : Units)
		{
			if (Unit.Team != ECrowdTeam::Army || Unit.Health <= 0.0f || Javelin.HitIds.Contains(Unit.Id))
			{
				continue;
			}
			const FVector2D ToUnit = Unit.Position - Start;
			const double UnitAlong = TravelSquared > UE_KINDA_SMALL_NUMBER ? FMath::Clamp(FVector2D::DotProduct(ToUnit, Travel) / TravelSquared, 0.0, 1.0) : 0.0;
			if (UnitAlong <= Reach && (ToUnit - Travel * UnitAlong).SizeSquared() <= RadiusSquared)
			{
				Javelin.HitIds.Add(Unit.Id);
				Unit.Health -= Tuning.CommanderSpearDamage * GetDamageTaken(Unit, Start);
				Unit.HurtTimer = Tuning.HealthBarSeconds;
				Unit.LastHitFrom = Start;
				Unit.bWasHit = true;
				++Result.JavelinsHitArmy;
			}
		}
		if (bHitsWizard)
		{
			++Result.JavelinsHitWizard;
			Javelins.RemoveAtSwap(Index);
			continue;
		}
		Javelin.Position = Start + Travel;
		Javelin.Life -= DeltaSeconds;
		if (WallAlpha <= 1.0 || Javelin.Life <= 0.0f)
		{
			Javelins.RemoveAtSwap(Index);
		}
	}
}

float FCrowdSim::GetThrowShare(const FCrowdUnit& Unit) const
{
	if (!IsWindingThrow(Unit))
	{
		return 0.0f;
	}
	return Tuning.CommanderSpearWindup > 0.0f ? FMath::Clamp(Unit.ThrowWind / Tuning.CommanderSpearWindup, 0.0f, 1.0f) : 1.0f;
}

bool FCrowdSim::IsThrowLocked(const FCrowdUnit& Unit) const
{
	return IsWindingThrow(Unit) && Unit.ThrowWind >= Tuning.CommanderSpearWindup - Tuning.CommanderSpearLock;
}

double FCrowdSim::GetThrowReach(const FCrowdUnit& Unit) const
{
	const FVector2D Path = Unit.ThrowAim * static_cast<double>(Tuning.CommanderSpearRange);
	double WallAlpha = 1.0;
	for (const FCrowdObstacle& Obstacle : Obstacles)
	{
		double Alpha = 0.0;
		if (!Obstacle.bLow && Obstacle.SegmentEnters(Unit.Position, Unit.Position + Path, BoltWallMargin, Alpha))
		{
			WallAlpha = FMath::Min(WallAlpha, Alpha);
		}
	}
	return Path.Size() * WallAlpha;
}

bool FCrowdSim::IsCommanderNear(FVector2D WizardPosition) const
{
	const double RadiusSquared = FMath::Square(static_cast<double>(Tuning.CommanderAuraRadius));
	for (const FCrowdUnit& Unit : Units)
	{
		if (Unit.Team == ECrowdTeam::Enemy && Unit.Kind == ECrowdKind::Commander && !Unit.bAsleep && Unit.Health > 0.0f
			&& FVector2D::DistSquared(Unit.Position, WizardPosition) <= RadiusSquared)
		{
			return true;
		}
	}
	return false;
}

float FCrowdSim::GetShown(const FCrowdUnit& Unit, FVector2D WizardPosition) const
{
	if (Unit.Kind != ECrowdKind::Assassin)
	{
		return 1.0f;
	}
	const double Distance = FVector2D::Distance(Unit.Position, WizardPosition);
	if (Distance >= Tuning.AssassinRevealRange)
	{
		return 0.0f;
	}
	return static_cast<float>(FMath::Clamp((Tuning.AssassinRevealRange - Distance) / FMath::Max(Tuning.AssassinRevealFade, 1.0f), 0.0, 1.0));
}

bool FCrowdSim::IsHiddenFromArmy(const FCrowdUnit& Unit, FVector2D WizardPosition) const
{
	return Unit.Kind == ECrowdKind::Assassin
		&& FVector2D::DistSquared(Unit.Position, WizardPosition) > FMath::Square(static_cast<double>(Tuning.AssassinArmyRange));
}

int32 FCrowdSim::CountHunting() const
{
	int32 Count = 0;
	for (const FCrowdUnit& Unit : Units)
	{
		Count += Unit.Team == ECrowdTeam::Army && Unit.bCommanded && Unit.bHunting ? 1 : 0;
	}
	return Count;
}

int32 FCrowdSim::GetArmyBoneValue() const
{
	// Each unit counts for a whole number: what it would pay at the shrine.
	int32 Worth = 0;
	for (const FCrowdUnit& Unit : Units)
	{
		// Only plain skeletons count (director, 2026-11-14): a Guard or a Marksman is never turned back into bones.
		Worth += Unit.Team == ECrowdTeam::Army && Unit.Kind == ECrowdKind::Skeleton ? GetUnitWorth(Unit.Kind) : 0;
	}
	return Worth;
}

int32 FCrowdSim::FindOrder(FVector2D Point) const
{
	for (int32 Index = 0; Index < Orders.Num(); ++Index)
	{
		if (Orders[Index].Point == Point)
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

int32 FCrowdSim::FindUnit(int32 Id) const
{
	for (int32 Index = 0; Index < Units.Num(); ++Index)
	{
		if (Units[Index].Id == Id)
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

bool FCrowdSim::IsInsideObstacle(FVector2D Position, double Margin) const
{
	for (const FCrowdObstacle& Obstacle : Obstacles)
	{
		if (Obstacle.Contains(Position, Margin))
		{
			return true;
		}
	}
	return false;
}

FVector2D FCrowdSim::PickCommandOffset(const FCrowdUnit& Unit)
{
	for (int32 Try = 0; Try < RoamPickTries; ++Try)
	{
		// Even over the group's circle.
		const double Angle = Random.FRandRange(0.0f, UE_TWO_PI);
		const FVector2D Offset = FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * (Unit.CommandRadius * FMath::Sqrt(static_cast<double>(Random.FRand())));
		// Not a spot in a wall, over water, or on the far side of either.
		if (Obstacles.Num() == 0
			|| (!IsInsideObstacle(Unit.CommandPoint + Offset, Tuning.UnitRadius) && HasClearPath(Unit.CommandPoint, Unit.CommandPoint + Offset)))
		{
			return Offset;
		}
	}
	return FVector2D::ZeroVector;
}

void FCrowdSim::RespondToShooter(FVector2D Point, int32 ShooterId)
{
	const int32 OrderIndex = FindOrder(Point);
	if (!Orders.IsValidIndex(OrderIndex) || Orders[OrderIndex].HuntId != 0 || Tuning.CommandRespondShare <= 0.0f)
	{
		return;
	}
	const int32 Shooter = FindUnit(ShooterId);
	if (Shooter == INDEX_NONE || Units[Shooter].Team != ECrowdTeam::Enemy
		|| FVector2D::DistSquared(Units[Shooter].Position, Point) > FMath::Square(static_cast<double>(Tuning.CommandRespondRange)))
	{
		return;
	}

	// A share of the group goes, those nearest the shooter. The rest hold the place.
	TArray<int32, TInlineAllocator<64>> Group;
	for (int32 Index = 0; Index < Units.Num(); ++Index)
	{
		const FCrowdUnit& Unit = Units[Index];
		if (Unit.Team == ECrowdTeam::Army && Unit.bCommanded && Unit.CommandPoint == Point && Unit.Health > 0.0f)
		{
			Group.Add(Index);
		}
	}
	if (Group.Num() == 0)
	{
		return;
	}
	const FVector2D ShooterPosition = Units[Shooter].Position;
	Group.Sort([this, ShooterPosition](int32 Left, int32 Right)
	{
		return FVector2D::DistSquared(Units[Left].Position, ShooterPosition) < FVector2D::DistSquared(Units[Right].Position, ShooterPosition);
	});
	const int32 Wanted = FMath::Clamp(FMath::CeilToInt32(Group.Num() * Tuning.CommandRespondShare), 1, Group.Num());
	for (int32 Index = 0; Index < Wanted; ++Index)
	{
		Units[Group[Index]].bHunting = true;
	}
	Orders[OrderIndex].HuntId = ShooterId;
	Orders[OrderIndex].HuntIndex = Shooter;
}

void FCrowdSim::EndHunt(FCrowdOrder& Order)
{
	Order.HuntId = 0;
	Order.HuntIndex = INDEX_NONE;
	for (FCrowdUnit& Unit : Units)
	{
		if (Unit.Team == ECrowdTeam::Army && Unit.bCommanded && Unit.CommandPoint == Order.Point && Unit.bHunting)
		{
			// Back to the place it holds.
			Unit.bHunting = false;
			Unit.bFighting = false;
		}
	}
}

float FCrowdSim::GetBodyRadius(ECrowdKind Kind) const
{
	return GetUnitHeight(Kind, WizardTall) * Tuning.BodyShare * (Kind == ECrowdKind::Brute ? Tuning.BruteBodyScale : 1.0f);
}

double FCrowdSim::GetReach(ECrowdKind Attacker, ECrowdKind Target) const
{
	// Two bodies cannot stand closer than their radii, so a blow reaches that far at least.
	return FMath::Max(static_cast<double>(Tuning.MeleeRange), static_cast<double>(GetBodyRadius(Attacker) + GetBodyRadius(Target)));
}

float FCrowdSim::GetDamageTaken(const FCrowdUnit& Target, FVector2D From) const
{
	if (Target.Kind != ECrowdKind::Brute || Target.Facing.IsNearlyZero())
	{
		return 1.0f;
	}
	// How far round from its front the blow comes: 1 dead ahead, -1 dead behind.
	const double Front = FVector2D::DotProduct(Target.Facing, (From - Target.Position).GetSafeNormal());
	if (Front >= Tuning.BruteArc)
	{
		return Tuning.BruteFrontTaken;
	}
	return Front <= -Tuning.BruteArc ? Tuning.BruteBackTaken : 1.0f;
}

float FCrowdSim::GetSlamDamage(const FCrowdUnit& Unit, FVector2D WizardPosition) const
{
	// A guard, and a skeleton in bone armor, lose a set share of their full health, wherever the wizard is.
	if (Unit.Kind == ECrowdKind::Guard)
	{
		return Unit.MaxHealth * (1.0f - FMath::Clamp(Tuning.BruteSlamGuardLeft, 0.0f, 1.0f));
	}
	if (Unit.Kind == ECrowdKind::Skeleton && bArmor)
	{
		return Unit.MaxHealth * (1.0f - FMath::Clamp(Tuning.BruteSlamArmoredLeft, 0.0f, 1.0f));
	}
	return Tuning.BruteSlamDamage * (IsInPresence(Unit, WizardPosition) ? 1.0f : Tuning.AwayDamageTaken);
}

float FCrowdSim::GetSlamShare(const FCrowdUnit& Unit) const
{
	if (!IsSlamming(Unit))
	{
		return 0.0f;
	}
	const FSlamShape Shape = MakeSlamShape(Tuning, Unit);
	if (Shape.Grow <= 0.0f || Unit.SlamTime >= Shape.Grow)
	{
		return 1.0f;
	}
	return FMath::Lerp(FMath::Clamp(Tuning.BruteSlamStartShare, 0.0f, 1.0f), 1.0f, Unit.SlamTime / Shape.Grow);
}

float FCrowdSim::GetSlamRadius(const FCrowdUnit& Unit) const
{
	return MakeSlamShape(Tuning, Unit).Radius;
}

bool FCrowdSim::IsInSweep(const FCrowdUnit& Unit, FVector2D Place) const
{
	const FVector2D Offset = Place - Unit.Position;
	const double Distance = Offset.Size();
	if (Distance > Tuning.BruteSweepRadius)
	{
		return false;
	}
	if (Distance < UE_KINDA_SMALL_NUMBER || Unit.Facing.IsNearlyZero())
	{
		return true;
	}
	const double HalfArc = FMath::DegreesToRadians(FMath::Clamp(Tuning.BruteSweepArc, 0.0f, 360.0f) * 0.5);
	return FVector2D::DotProduct(Offset / Distance, Unit.Facing.GetSafeNormal()) >= FMath::Cos(HalfArc);
}

int32 FCrowdSim::CountSlamming() const
{
	int32 Count = 0;
	for (const FCrowdUnit& Unit : Units)
	{
		Count += IsSlamming(Unit) ? 1 : 0;
	}
	return Count;
}

void FCrowdSim::UpdateSlam(FCrowdUnit& Unit, float DeltaSeconds, FVector2D WizardPosition, FCrowdStepResult& Result)
{
	// A brute's slam and the commander's slash are one thing with different numbers. A brute's sweep is the same
	// again, with a third of a circle around the brute in place of a circle ahead of it.
	FSlamShape Shape = MakeSlamShape(Tuning, Unit);
	const bool bCommander = Unit.Kind == ECrowdKind::Commander;
	const bool bCanSweep = !bCommander && Tuning.BruteSlamEvery > 0.0f;
	Unit.HeavyTimer = FMath::Max(Unit.HeavyTimer - DeltaSeconds, 0.0f);
	Unit.bStriking = false;
	if (Unit.BlowAgo >= 0.0f)
	{
		// Following through after a slam that landed, for as long as it rests.
		Unit.BlowAgo += DeltaSeconds;
		if (Unit.BlowAgo >= Shape.Rest)
		{
			Unit.BlowAgo = -1.0f;
		}
	}
	if (Unit.bAsleep)
	{
		Unit.SlamTime = -1.0f;
		Unit.BlowAgo = -1.0f;
		return;
	}
	if (Unit.SlamTime < 0.0f)
	{
		// Resting between slams.
		Unit.AttackTimer = FMath::Max(Unit.AttackTimer - DeltaSeconds, 0.0f);
		// What it goes for: one of the army in its way, or the wizard.
		const bool bHasTarget = Units.IsValidIndex(Unit.TargetIndex);
		if (!bHasTarget && Unit.Alert != ECrowdAlert::Chase)
		{
			// It has lost the wizard and none of his army is by it: nothing to slam.
			return;
		}
		const FVector2D Aim = bHasTarget ? Units[Unit.TargetIndex].Position : WizardPosition;
		const FVector2D ToAim = (Aim - Unit.Position).GetSafeNormal();
		// The circle lies straight ahead of it. It starts one only when what it goes for is well inside.
		// A brute's front is where it faces, and it turns slowly. The commander turns as he strikes.
		const FVector2D Front = bCommander || Unit.Facing.IsNearlyZero() ? ToAim : Unit.Facing;
		const FSlamShape Heavy = MakeSlamShape(Tuning, Unit.Kind);
		FVector2D Center = Unit.Position + Front * Heavy.Distance;
		bool bInFront = !Front.IsNearlyZero()
			&& FVector2D::DistSquared(Aim, Center) <= FMath::Square(static_cast<double>(Heavy.Radius * Tuning.BruteSlamAimShare));
		bool bSweepNow = false;
		if (bCanSweep && (!bInFront || Unit.HeavyTimer > 0.0f))
		{
			// No slam is ready, or what it goes for is not in the circle: the sweep, when that is well inside its reach.
			FCrowdUnit Standing = Unit;
			Standing.Facing = Front;
			bSweepNow = !Front.IsNearlyZero() && FVector2D::DistSquared(Aim, Unit.Position) <= FMath::Square(static_cast<double>(Tuning.BruteSweepRadius * Tuning.BruteSlamAimShare))
				&& IsInSweep(Standing, Aim);
			bInFront = bSweepNow;
			Center = Unit.Position;
		}
		if (!bInFront || Unit.AttackTimer > 0.0f || (!bHasTarget && bWizardInvulnerable))
		{
			if (bHasTarget && FVector2D::DistSquared(Aim, Unit.Position) <= FMath::Square(GetReach(Unit.Kind, Units[Unit.TargetIndex].Kind) + MeleeSlack))
			{
				// Standing by one of the army, it turns to it (a brute slowly: it can be got behind).
				Unit.FaceGoal = ToAim;
			}
			return;
		}
		Unit.SlamTime = 0.0f;
		Unit.BlowAgo = -1.0f;
		Unit.SlamCenter = Center;
		Unit.Facing = Front;
		Unit.bSweep = bSweepNow;
		if (!bSweepNow)
		{
			Unit.HeavyTimer = Tuning.BruteSlamEvery;
		}
		Shape = MakeSlamShape(Tuning, Unit);
	}

	// From the first of the circle until the slam lands it stands, and it does not turn.
	Unit.bStriking = true;
	Unit.FaceGoal = Unit.Facing;
	Unit.SlamTime += DeltaSeconds;
	if (Unit.SlamTime < Shape.Grow + Shape.Hold)
	{
		return;
	}

	// It lands: everything of the wizard's inside the circle is hit, and he is too if he stands in it.
	const double RadiusSquared = FMath::Square(static_cast<double>(Shape.Radius));
	int32 Struck = 0;
	const bool bSweep = Unit.Kind == ECrowdKind::Brute && Unit.bSweep;
	for (FCrowdUnit& Other : Units)
	{
		if (Other.Team != ECrowdTeam::Army || (bSweep ? !IsInSweep(Unit, Other.Position) : FVector2D::DistSquared(Other.Position, Unit.SlamCenter) > RadiusSquared))
		{
			continue;
		}
		++Struck;
		Other.LastHitFrom = Unit.Position;
		Other.bWasHit = true;
		Other.Health -= bCommander
			? Tuning.CommanderSlashDamage * (IsInPresence(Other, WizardPosition) ? 1.0f : Tuning.AwayDamageTaken)
			: bSweep ? Tuning.BruteSweepDamage * (IsInPresence(Other, WizardPosition) ? 1.0f : Tuning.AwayDamageTaken)
			: GetSlamDamage(Other, WizardPosition);
		Other.HurtTimer = Tuning.HealthBarSeconds;
	}
	const bool bHitWizard = !bWizardInvulnerable && (bSweep ? IsInSweep(Unit, WizardPosition) : FVector2D::DistSquared(WizardPosition, Unit.SlamCenter) <= RadiusSquared);
	if (bSweep)
	{
		Result.SweepsHitWizard += bHitWizard ? 1 : 0;
		++Result.SweepsLanded;
		Unit.SlamTime = -1.0f;
		Unit.AttackTimer = Shape.Rest;
		Unit.BlowAgo = 0.0f;
		return;
	}
	if (bCommander)
	{
		Result.SlashesHitWizard += bHitWizard ? 1 : 0;
		++Result.SlashesLanded;
	}
	else
	{
		Result.SlamsHitWizard += bHitWizard ? 1 : 0;
		++Result.SlamsLanded;
		if (Struck == 0 && !bHitWizard)
		{
			AddEvent(ECrowdEvent::SlamMiss, Unit.SlamCenter, Unit.Kind, Unit.Team);
		}
	}
	FCrowdBlast Hit;
	Hit.Position = Unit.SlamCenter;
	Hit.Radius = Shape.Radius;
	SlamHits.Add(Hit);
	Unit.SlamTime = -1.0f;
	Unit.AttackTimer = Shape.Rest;
	Unit.BlowAgo = 0.0f;
}

void FCrowdSim::UpdateFacing(float DeltaSeconds, FVector2D WizardPosition)
{
	for (FCrowdUnit& Unit : Units)
	{
		// It faces what it strikes (set where blows are dealt); else the way it walks; an enemy that stands, the wizard.
		FVector2D Goal = Unit.FaceGoal;
		if (Goal.IsNearlyZero())
		{
			if (Unit.Velocity.SizeSquared() > FMath::Square(FacingSpeed))
			{
				Goal = Unit.Velocity.GetSafeNormal();
			}
			else if (Unit.Team == ECrowdTeam::Enemy && !Unit.bAsleep && Unit.Alert == ECrowdAlert::Chase && !Unit.Facing.IsNearlyZero())
			{
				Goal = (WizardPosition - Unit.Position).GetSafeNormal();
			}
		}
		Unit.FaceGoal = FVector2D::ZeroVector;
		if (Goal.IsNearlyZero())
		{
			continue;
		}
		if (Unit.Kind != ECrowdKind::Brute || Unit.Facing.IsNearlyZero())
		{
			Unit.Facing = Goal;
			continue;
		}
		// A brute turns slowly, so it can be got behind.
		const float Now = FMath::RadiansToDegrees(static_cast<float>(FMath::Atan2(Unit.Facing.Y, Unit.Facing.X)));
		const float Wanted = FMath::RadiansToDegrees(static_cast<float>(FMath::Atan2(Goal.Y, Goal.X)));
		const float Turned = FMath::DegreesToRadians(FMath::FixedTurn(Now, Wanted, Tuning.BruteTurnDegrees * DeltaSeconds));
		Unit.Facing = FVector2D(FMath::Cos(Turned), FMath::Sin(Turned));
	}
}

int32 FCrowdSim::CountSpearSkeletons() const
{
	int32 Count = 0;
	for (const FCrowdUnit& Unit : Units)
	{
		Count += Unit.Team == ECrowdTeam::Army && Unit.Kind == ECrowdKind::Skeleton && !Unit.bCommanded ? 1 : 0;
	}
	return Count;
}

int32 FCrowdSim::GetFallenBoneValue(ECrowdKind Kind) const
{
	const float Cost = Kind == ECrowdKind::Skeleton ? Tuning.BonesPerSkeleton : Kind == ECrowdKind::Guard ? Tuning.GuardCost
		: Kind == ECrowdKind::Marksman ? Tuning.MarksmanCost : 0.0f;
	if (Cost <= 0.0f || Tuning.ArmyDropShare <= 0.0f)
	{
		return 0;
	}
	// Always a whole number, and never nothing.
	return FMath::Max(FMath::RoundToInt32(Cost * Tuning.ArmyDropShare), 1);
}

bool FCrowdSim::IsInPresence(const FCrowdUnit& Unit, FVector2D WizardPosition) const
{
	return Tuning.PresenceRadius <= 0.0f
		|| FVector2D::DistSquared(Unit.Position, WizardPosition) <= FMath::Square(static_cast<double>(Tuning.PresenceRadius));
}

void FCrowdSim::SetView(FVector2D Min, FVector2D Max)
{
	bHasView = true;
	ViewMin = FVector2D(FMath::Min(Min.X, Max.X), FMath::Min(Min.Y, Max.Y));
	ViewMax = FVector2D(FMath::Max(Min.X, Max.X), FMath::Max(Min.Y, Max.Y));
}

bool FCrowdSim::IsInView(FVector2D Position) const
{
	return bHasView && Position.X >= ViewMin.X && Position.X <= ViewMax.X && Position.Y >= ViewMin.Y && Position.Y <= ViewMax.Y;
}

bool FCrowdSim::IsSpotTaken(const FCrowdUnit& Unit, FVector2D Spot) const
{
	const double Radius = Tuning.SeparationRadius;
	const double FromSpotSquared = FVector2D::DistSquared(Unit.Position, Spot);
	if (Radius <= 0.0 || FromSpotSquared <= FMath::Square(RoamArriveDistance) || FromSpotSquared > FMath::Square(Radius * TakenCheckShare))
	{
		return false;
	}
	for (const FCrowdUnit& Other : Units)
	{
		if (Other.Team == Unit.Team && Other.Id != Unit.Id && !Other.bMoving
			&& FVector2D::DistSquared(Other.Position, Spot) < FMath::Square(FMath::Max(Radius * TakenRadiusShare,
				static_cast<double>(GetBodyRadius(Unit.Kind) + GetBodyRadius(Other.Kind)) * TakenBodyShare)))
		{
			return true;
		}
	}
	return false;
}

bool FCrowdSim::PickOpenOffset(FVector2D WizardPosition, FVector2D& OutOffset)
{
	const double InnerSquared = FMath::Square(static_cast<double>(Tuning.RoamInnerRadius));
	const double OuterSquared = FMath::Square(static_cast<double>(FMath::Max(Tuning.RoamRadius, Tuning.RoamInnerRadius)));
	for (int32 Try = 0; Try < OpenPickTries; ++Try)
	{
		const double Angle = Random.FRandRange(0.0f, UE_TWO_PI);
		const double Radius = FMath::Sqrt(FMath::Lerp(InnerSquared, OuterSquared, static_cast<double>(Random.FRand())));
		const FVector2D Offset = FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius;
		// Open ground on the wizard's side of every wall and of the water.
		if (!IsInsideObstacle(WizardPosition + Offset, Tuning.UnitRadius) && HasClearPath(WizardPosition, WizardPosition + Offset))
		{
			OutOffset = Offset;
			return true;
		}
	}
	return false;
}

int32 FCrowdSim::CountCommandable(ECrowdCommandKind Kind) const
{
	int32 Count = 0;
	for (const FCrowdUnit& Unit : Units)
	{
		Count += Unit.Team == ECrowdTeam::Army && !Unit.bCommanded && MatchesCommandKind(Unit, Kind) ? 1 : 0;
	}
	return Count;
}

int32 FCrowdSim::CountCommanded() const
{
	int32 Count = 0;
	for (const FCrowdUnit& Unit : Units)
	{
		Count += Unit.Team == ECrowdTeam::Army && Unit.bCommanded ? 1 : 0;
	}
	return Count;
}

float FCrowdSim::GetCommandRadius(int32 Count) const
{
	return static_cast<float>(FMath::Min(
		FMath::Max(static_cast<double>(Tuning.CommandScatter), Tuning.CommandSpacing * FMath::Sqrt(static_cast<double>(FMath::Max(Count, 0)))),
		Tuning.CommandRejoinRadius * CommandRadiusShare));
}

int32 FCrowdSim::Command(FVector2D Point, int32 Count, ECrowdCommandKind Kind)
{
	// A place in a wall or over water becomes the nearest open ground.
	PushOutOfObstacles(Point);

	// Those nearest the place go.
	TArray<int32, TInlineAllocator<64>> Chosen;
	for (int32 Index = 0; Index < Units.Num(); ++Index)
	{
		const FCrowdUnit& Unit = Units[Index];
		if (Unit.Team == ECrowdTeam::Army && !Unit.bCommanded && MatchesCommandKind(Unit, Kind))
		{
			Chosen.Add(Index);
		}
	}
	Chosen.Sort([this, Point](int32 Left, int32 Right)
	{
		return FVector2D::DistSquared(Units[Left].Position, Point) < FVector2D::DistSquared(Units[Right].Position, Point);
	});
	const int32 Sent = FMath::Clamp(Count, 0, Chosen.Num());
	if (Sent <= 0)
	{
		return 0;
	}

	if (FindOrder(Point) == INDEX_NONE)
	{
		FCrowdOrder& Order = Orders.AddDefaulted_GetRef();
		Order.Point = Point;
		if (Obstacles.Num() > 0)
		{
			Order.Field.Build(Point, Obstacles, Tuning.UnitRadius);
		}
	}

	// The group spreads over a circle that grows with its size, so it does not stand as one clump in the middle.
	const double Radius = GetCommandRadius(Sent);
	const double FirstAngle = Random.FRandRange(0.0f, UE_TWO_PI);
	for (int32 Index = 0; Index < Sent; ++Index)
	{
		FCrowdUnit& Unit = Units[Chosen[Index]];
		Unit.bCommanded = true;
		Unit.bCommandArmed = false;
		Unit.bHunting = false;
		Unit.CommandPoint = Point;
		Unit.CommandRadius = static_cast<float>(Radius);
		Unit.bFighting = false;
		Unit.TargetIndex = INDEX_NONE;
		Unit.Group = INDEX_NONE;
		// Its own first spot: evenly over the circle, like the seeds of a sunflower. It picks others as it idles.
		const double Angle = FirstAngle + Index * GoldenAngle;
		Unit.RoamOffset = FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * (Radius * FMath::Sqrt((Index + 0.5) / Sent));
		if (Obstacles.Num() > 0
			&& (IsInsideObstacle(Point + Unit.RoamOffset, Tuning.UnitRadius) || !HasClearPath(Point, Point + Unit.RoamOffset)))
		{
			Unit.RoamOffset = PickCommandOffset(Unit);
		}
		Unit.RoamTimer = Random.FRandRange(Tuning.RoamPauseMin, Tuning.RoamPauseMax);
	}
	return Sent;
}

int32 FCrowdSim::Recall()
{
	int32 Count = 0;
	for (FCrowdUnit& Unit : Units)
	{
		if (Unit.Team == ECrowdTeam::Army && Unit.bCommanded)
		{
			Unit.bCommanded = false;
			Unit.bHunting = false;
			Unit.bSettle = true;
			++Count;
		}
	}
	return Count;
}

void FCrowdSim::UpdateCommands(FVector2D WizardPosition, FCrowdStepResult& Result)
{
	for (FCrowdUnit& Unit : Units)
	{
		if (Unit.Team != ECrowdTeam::Army || !Unit.bCommanded)
		{
			continue;
		}
		const double Distance = FVector2D::Distance(WizardPosition, Unit.CommandPoint);
		if (!Unit.bCommandArmed)
		{
			// Sent to a place beside him, they hold it until he has been away.
			Unit.bCommandArmed = Distance > Tuning.CommandRejoinRadius + CommandArmSlack;
		}
		else if (Distance <= Tuning.CommandRejoinRadius)
		{
			Unit.bCommanded = false;
			Unit.bHunting = false;
			Unit.bSettle = true;
			++Result.Rejoined;
		}
	}

	// An order stands while anyone holds its place. A hunt lasts until the enemy is dead, has got away, or nobody is left to chase it.
	for (int32 OrderIndex = Orders.Num() - 1; OrderIndex >= 0; --OrderIndex)
	{
		FCrowdOrder& Order = Orders[OrderIndex];
		int32 Holding = 0;
		int32 Hunting = 0;
		for (const FCrowdUnit& Unit : Units)
		{
			if (Unit.Team == ECrowdTeam::Army && Unit.bCommanded && Unit.CommandPoint == Order.Point)
			{
				++Holding;
				Hunting += Unit.bHunting ? 1 : 0;
			}
		}
		if (Holding == 0)
		{
			Orders.RemoveAtSwap(OrderIndex);
			continue;
		}
		Order.HuntIndex = INDEX_NONE;
		if (Order.HuntId == 0)
		{
			continue;
		}
		const int32 Hunted = FindUnit(Order.HuntId);
		if (Hunted == INDEX_NONE || Hunting == 0
			|| FVector2D::DistSquared(Units[Hunted].Position, Order.Point) > FMath::Square(static_cast<double>(Tuning.CommandRespondRange)))
		{
			EndHunt(Order);
			continue;
		}
		Order.HuntIndex = Hunted;
		if (Obstacles.Num() > 0 && !Order.HuntField.IsBuiltFor(Units[Hunted].Position))
		{
			Order.HuntField.Build(Units[Hunted].Position, Obstacles, Tuning.UnitRadius);
		}
	}
}

void FCrowdSim::GetCommandPoints(TArray<FVector2D>& OutPoints) const
{
	OutPoints.Reset();
	for (const FCrowdUnit& Unit : Units)
	{
		if (Unit.Team == ECrowdTeam::Army && Unit.bCommanded)
		{
			OutPoints.AddUnique(Unit.CommandPoint);
		}
	}
}

const TCHAR* FCrowdSim::GetCommandKindName(ECrowdCommandKind Kind)
{
	switch (Kind)
	{
	case ECrowdCommandKind::Skeleton:
		return TEXT("Skeleton");
	case ECrowdCommandKind::Guard:
		return TEXT("Guard");
	case ECrowdCommandKind::Any:
	default:
		return TEXT("Any");
	}
}

void FCrowdSim::UpdateRoam(FCrowdUnit& Unit, float DeltaSeconds, FVector2D WizardPosition)
{
	if (Unit.bSettle && GetGroupCount(CountTeam(ECrowdTeam::Army)) > 0)
	{
		// Raised just now, or its fight is over: it joins a group, or goes back to its own.
		Unit.bSettle = false;
		Unit.RoamOffset = PickGroupOffset(Unit, WizardPosition);
		Unit.RoamTimer = Random.FRandRange(Tuning.RoamPauseMin, Tuning.RoamPauseMax);
		return;
	}
	if (Unit.bSettle)
	{
		Unit.bSettle = false;
		const FVector2D Offset = Unit.Position - WizardPosition;
		const double Distance = Offset.Size();
		if (Distance > Tuning.RoamRadius)
		{
			// Too far out to stay: come in to the edge of the circle on the side it is already on.
			Unit.RoamOffset = Offset / Distance * Tuning.RoamRadius;
		}
		else if (Distance >= Tuning.RoamInnerRadius)
		{
			Unit.RoamOffset = Offset;
		}
		else
		{
			Unit.RoamOffset = PickRoamOffset(WizardPosition);
		}
		Unit.RoamTimer = Random.FRandRange(Tuning.RoamPauseMin, Tuning.RoamPauseMax);
		return;
	}

	Unit.RoamTimer -= DeltaSeconds;
	if (Unit.RoamTimer <= 0.0f)
	{
		Unit.RoamOffset = PickGroupOffset(Unit, WizardPosition);
		Unit.RoamTimer = Random.FRandRange(Tuning.RoamPauseMin, Tuning.RoamPauseMax);
	}
}

FVector2D FCrowdSim::PickRoamOffset(FVector2D WizardPosition)
{
	const double InnerSquared = FMath::Square(static_cast<double>(Tuning.RoamInnerRadius));
	const double OuterSquared = FMath::Square(static_cast<double>(FMath::Max(Tuning.RoamRadius, Tuning.RoamInnerRadius)));
	FVector2D Offset = FVector2D::ZeroVector;
	for (int32 Try = 0; Try < RoamPickTries; ++Try)
	{
		// Even over the ring between the inner and outer radius.
		const double Angle = Random.FRandRange(0.0f, UE_TWO_PI);
		const double Radius = FMath::Sqrt(FMath::Lerp(InnerSquared, OuterSquared, static_cast<double>(Random.FRand())));
		Offset = FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius;

		// A spot behind a wall would send the skeleton away from the wizard: try another.
		if (Obstacles.Num() == 0 || HasClearPath(WizardPosition, WizardPosition + Offset))
		{
			break;
		}
	}
	return Offset;
}

FVector2D FCrowdSim::GetGroupMiddle(int32 Group, int32 GroupCount) const
{
	// The groups stand evenly around the wizard, like walls on every side of him.
	const double Angle = GroupFirstAngle + UE_DOUBLE_TWO_PI * Group / FMath::Max(GroupCount, 1);
	return FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Tuning.GroupDistance;
}

FVector2D FCrowdSim::PickGroupOffset(FCrowdUnit& Unit, FVector2D WizardPosition)
{
	const int32 ArmySize = CountTeam(ECrowdTeam::Army);
	const int32 GroupCount = GetGroupCount(ArmySize);
	if (GroupCount <= 0)
	{
		Unit.Group = INDEX_NONE;
		return PickRoamOffset(WizardPosition);
	}

	TArray<int32, TInlineAllocator<8>> Sizes;
	Sizes.SetNumZeroed(GroupCount);
	for (const FCrowdUnit& Other : Units)
	{
		if (Other.Team == ECrowdTeam::Army && Sizes.IsValidIndex(Other.Group))
		{
			++Sizes[Other.Group];
		}
	}

	// A skeleton stays in its group while the group is no larger than an even share. Otherwise it moves to the
	// emptiest group, the nearest one when several are as empty. So groups stay even and none grows large.
	const int32 EvenShare = FMath::DivideAndRoundUp(ArmySize, GroupCount);
	if (!Sizes.IsValidIndex(Unit.Group) || Sizes[Unit.Group] > EvenShare)
	{
		if (Sizes.IsValidIndex(Unit.Group))
		{
			--Sizes[Unit.Group];
		}
		const FVector2D FromWizard = Unit.Position - WizardPosition;
		int32 Best = 0;
		double BestDistanceSquared = TNumericLimits<double>::Max();
		for (int32 Group = 0; Group < GroupCount; ++Group)
		{
			const double DistanceSquared = FVector2D::DistSquared(FromWizard, GetGroupMiddle(Group, GroupCount));
			if (Sizes[Group] < Sizes[Best] || (Sizes[Group] == Sizes[Best] && DistanceSquared < BestDistanceSquared))
			{
				Best = Group;
				BestDistanceSquared = DistanceSquared;
			}
		}
		Unit.Group = Best;
	}

	// Somewhere near the middle of its group, so the group shifts a little but holds together.
	const double Angle = Random.FRandRange(0.0f, UE_TWO_PI);
	const double Radius = Tuning.GroupShuffle * FMath::Sqrt(static_cast<double>(Random.FRand()));
	const FVector2D Offset = GetGroupMiddle(Unit.Group, GroupCount) + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius;

	// A spot behind a wall would send the skeleton away from the wizard: take a free spot instead.
	if (Obstacles.Num() > 0 && !HasClearPath(WizardPosition, WizardPosition + Offset))
	{
		return PickRoamOffset(WizardPosition);
	}
	return Offset;
}

void FCrowdSim::SeparateUnits()
{
	if (Tuning.SeparationRadius <= 0.0f)
	{
		return;
	}
	// Each kind takes the room of its body. Two units keep their two radii apart, or SeparationRadius if that is more.
	float Bodies[7] = {};
	float Widest = 0.0f;
	for (int32 Kind = 0; Kind < UE_ARRAY_COUNT(Bodies); ++Kind)
	{
		Bodies[Kind] = GetBodyRadius(static_cast<ECrowdKind>(Kind));
		Widest = FMath::Max(Widest, Bodies[Kind]);
	}
	const double WidestSquared = FMath::Square(FMath::Max(static_cast<double>(Tuning.SeparationRadius), Widest * 2.0));
	for (FCrowdUnit& Unit : Units)
	{
		Unit.bMoving = FVector2D::DistSquared(Unit.Position, Unit.StepStart) > FMath::Square(StandingMove);
	}
	for (int32 Index = 0; Index < Units.Num(); ++Index)
	{
		for (int32 OtherIndex = Index + 1; OtherIndex < Units.Num(); ++OtherIndex)
		{
			FVector2D Offset = Units[OtherIndex].Position - Units[Index].Position;
			const double DistanceSquared = Offset.SizeSquared();
			if (DistanceSquared >= WidestSquared)
			{
				continue;
			}
			const double Radius = FMath::Max(static_cast<double>(Tuning.SeparationRadius),
				static_cast<double>(Bodies[static_cast<int32>(Units[Index].Kind)] + Bodies[static_cast<int32>(Units[OtherIndex].Kind)]));
			if (DistanceSquared >= Radius * Radius)
			{
				continue;
			}
			double Distance = FMath::Sqrt(DistanceSquared);
			if (Distance < UE_KINDA_SMALL_NUMBER)
			{
				const double Angle = (Index + OtherIndex) * GoldenAngle;
				Offset = FVector2D(FMath::Cos(Angle), FMath::Sin(Angle));
				Distance = 1.0;
			}
			// Half the overlap in all, softened so a packed crowd settles instead of jittering. Shared evenly, except
			// between two of one side where one stands and one walks: the walker gives way, all of it.
			FCrowdUnit& First = Units[Index];
			FCrowdUnit& Second = Units[OtherIndex];
			double FirstShare = 0.5;
			// Softened, so a packed crowd settles instead of jittering. Two that stroll about a post are put clear of
			// each other at once, so they slide past instead of through.
			double Softness = 0.5;
			if (First.bStriking != Second.bStriking)
			{
				// One in a blow stands its ground: the other gives way, all of it.
				FirstShare = First.bStriking ? 0.0 : 1.0;
			}
			else if (First.Team == Second.Team && First.bMoving != Second.bMoving)
			{
				FirstShare = First.bMoving ? 1.0 : 0.0;
			}
			else if (First.bMoving && Second.bMoving && First.bAsleep && Second.bAsleep)
			{
				// Two strolling about a post.
				Softness = 1.0;
			}
			const FVector2D Push = Offset / Distance * ((Radius - Distance) * Softness);
			First.Position -= Push * FirstShare;
			Second.Position += Push * (1.0 - FirstShare);
		}
	}
}

void FCrowdSim::KeepOutOfObstacles()
{
	if (Obstacles.Num() == 0)
	{
		return;
	}
	for (FCrowdUnit& Unit : Units)
	{
		PushOutOfObstacles(Unit.Position);
		if (!IsInsideObstacle(Unit.Position, 0.0))
		{
			Unit.LastFree = Unit.Position;
			Unit.bHasLastFree = true;
		}
		else if (Unit.bHasLastFree)
		{
			// Still in a wall or over water (pressed in by a crowd where boxes meet): back to where it last stood clear.
			Unit.Position = Unit.LastFree;
		}
	}
}

void FCrowdSim::PushOutOfObstacles(FVector2D& Position) const
{
	const double Radius = Tuning.UnitRadius;
	for (int32 Try = 0; Try < PushOutTries; ++Try)
	{
		const FCrowdObstacle* Inside = nullptr;
		for (const FCrowdObstacle& Obstacle : Obstacles)
		{
			if (Obstacle.Contains(Position, Radius))
			{
				Inside = &Obstacle;
				break;
			}
		}
		if (!Inside)
		{
			return;
		}
		// The four ways out of this box. The nearest one that is clear of every other box is taken; where boxes sit
		// side by side (a river is a row of them), the nearest side alone would lead from one into the next.
		const FVector2D Reach = Inside->HalfSize + FVector2D(Radius + PushOutGap, Radius + PushOutGap);
		const FVector2D Exits[] = {
			FVector2D(Inside->Center.X - Reach.X, Position.Y), FVector2D(Inside->Center.X + Reach.X, Position.Y),
			FVector2D(Position.X, Inside->Center.Y - Reach.Y), FVector2D(Position.X, Inside->Center.Y + Reach.Y) };
		FVector2D Best = Exits[0];
		double BestScore = TNumericLimits<double>::Max();
		for (const FVector2D& Exit : Exits)
		{
			// An exit into another box counts as far away, so it is taken only when no exit is clear.
			const double Score = FVector2D::DistSquared(Exit, Position) + (IsInsideObstacle(Exit, Radius) ? 1.0e12 : 0.0);
			if (Score < BestScore)
			{
				BestScore = Score;
				Best = Exit;
			}
		}
		Position = Best;
	}
}

void FCrowdSim::ApplyMelee(float DeltaSeconds, FVector2D WizardPosition, FCrowdStepResult& Result)
{
	const float Interval = FMath::Max(Tuning.MeleeInterval, 0.0f);
	for (int32 Index = 0; Index < Units.Num(); ++Index)
	{
		FCrowdUnit& Unit = Units[Index];
		// An archer's timer is for its arrows; it never strikes.
		if (Unit.Kind == ECrowdKind::Archer)
		{
			continue;
		}
		if (Unit.Kind == ECrowdKind::Assassin && Unit.Team == ECrowdTeam::Enemy)
		{
			UpdateAssassinSwing(Unit, DeltaSeconds, WizardPosition);
			continue;
		}
		// A Marksman shoots from where it stands.
		if (Unit.Kind == ECrowdKind::Marksman)
		{
			UpdateMarksman(Unit, DeltaSeconds, WizardPosition, Result);
			continue;
		}
		// A brute does not trade blows: it slams. The commander slashes the same way, and throws his spear.
		if ((Unit.Kind == ECrowdKind::Brute || Unit.Kind == ECrowdKind::Commander) && Unit.Team == ECrowdTeam::Enemy)
		{
			if (Unit.Kind == ECrowdKind::Commander && UpdateThrow(Unit, DeltaSeconds, WizardPosition, Result))
			{
				continue;
			}
			UpdateSlam(Unit, DeltaSeconds, WizardPosition, Result);
			continue;
		}
		const bool bWasStriking = Unit.bStriking;
		Unit.bStriking = false;
		if (!Units.IsValidIndex(Unit.TargetIndex))
		{
			// Nothing to strike: the next blow is wound up from the start.
			Unit.AttackTimer = Interval * Tuning.MeleeFirstShare;
			Unit.StrikeId = 0;
			continue;
		}
		if (FVector2D::DistSquared(Unit.Position, Units[Unit.TargetIndex].Position) > FMath::Square(GetReach(Unit.Kind, Units[Unit.TargetIndex].Kind) + MeleeSlack))
		{
			if (Unit.StrikeId == Units[Unit.TargetIndex].Id)
			{
				// It had started a blow at this one, which has stepped out of reach. It goes after it with the blow
				// still winding up, so one that runs is struck when it is caught. (It stands for every blow, so
				// without this a runner would never be hit.)
				Unit.AttackTimer = FMath::Max(Unit.AttackTimer - DeltaSeconds, 0.0f);
			}
			else
			{
				// Out of reach of something new: the next blow is wound up from the start.
				Unit.AttackTimer = Interval * Tuning.MeleeFirstShare;
			}
			continue;
		}
		Unit.StrikeId = Units[Unit.TargetIndex].Id;
		Unit.bStriking = true;
		// It takes its aim as the blow starts and holds it until the blow has landed: it does not turn in a blow.
		const FVector2D ToTarget = (Units[Unit.TargetIndex].Position - Unit.Position).GetSafeNormal();
		Unit.FaceGoal = bWasStriking && !Unit.Facing.IsNearlyZero() ? Unit.Facing : ToTarget;
		// One blow at a time: the whole of what it does in an interval, when the interval is up.
		float Damage = GetStats(Unit.Kind).DamagePerSecond;
		if (Interval > 0.0f)
		{
			Unit.AttackTimer -= DeltaSeconds;
			if (Unit.AttackTimer > 0.0f)
			{
				continue;
			}
			Unit.AttackTimer += Interval;
			Damage *= Interval;
			// The blow has landed: it aims again for the next.
			Unit.FaceGoal = ToTarget;
		}
		else
		{
			Damage *= DeltaSeconds;
		}
		FCrowdUnit& Target = Units[Unit.TargetIndex];
		// Away from the wizard's presence the army hits less hard and is hurt more.
		if (Unit.Team == ECrowdTeam::Army && !IsInPresence(Unit, WizardPosition))
		{
			Damage *= Tuning.AwayDamageShare;
		}
		if (Target.Team == ECrowdTeam::Army && !IsInPresence(Target, WizardPosition))
		{
			Damage *= Tuning.AwayDamageTaken;
		}
		Target.Health -= Damage * GetDamageTaken(Target, Unit.Position);
		Target.HurtTimer = Tuning.HealthBarSeconds;
		Target.LastHitFrom = Unit.Position;
		Target.bWasHit = true;
		if (Unit.Team == ECrowdTeam::Army && Interval > 0.0f)
		{
			AddEvent(ECrowdEvent::ArmyBlow, Target.Position, Target.Kind, Target.Team);
		}
	}
}

void FCrowdSim::MoveBolts(float DeltaSeconds)
{
	const double RadiusSquared = FMath::Square(static_cast<double>(Tuning.BoltRadius));
	for (int32 BoltIndex = Bolts.Num() - 1; BoltIndex >= 0; --BoltIndex)
	{
		FCrowdBolt& Bolt = Bolts[BoltIndex];
		const FVector2D Start = Bolt.Position;
		const FVector2D Travel = Bolt.Velocity * DeltaSeconds;
		const double TravelSquared = Travel.SizeSquared();

		// Swept test along this frame's path, so a fast bolt cannot skip over an enemy.
		// Where along the path of this frame the bolt meets a wall, if it does. Nothing past that point can be hit.
		double WallAlpha = TNumericLimits<double>::Max();
		for (const FCrowdObstacle& Obstacle : Obstacles)
		{
			double Alpha = 0.0;
			if (!Obstacle.bLow && !Obstacle.bBoltPasses && Obstacle.SegmentEnters(Start, Start + Travel, BoltWallMargin, Alpha))
			{
				WallAlpha = FMath::Min(WallAlpha, Alpha);
			}
		}

		int32 HitIndex = INDEX_NONE;
		double HitAlpha = WallAlpha;
		for (int32 UnitIndex = 0; UnitIndex < Units.Num(); ++UnitIndex)
		{
			const FCrowdUnit& Unit = Units[UnitIndex];
			if (Unit.Team != ECrowdTeam::Enemy || Unit.Health <= 0.0f)
			{
				continue;
			}
			const FVector2D ToUnit = Unit.Position - Start;
			const double Alpha = TravelSquared > UE_KINDA_SMALL_NUMBER
				? FMath::Clamp(FVector2D::DotProduct(ToUnit, Travel) / TravelSquared, 0.0, 1.0)
				: 0.0;
			if ((ToUnit - Travel * Alpha).SizeSquared() <= RadiusSquared && Alpha < HitAlpha)
			{
				HitAlpha = Alpha;
				HitIndex = UnitIndex;
			}
		}

		if (HitIndex != INDEX_NONE)
		{
			Units[HitIndex].Health -= GetBoltDamage() * GetDamageTaken(Units[HitIndex], Start);
			Units[HitIndex].HurtTimer = Tuning.HealthBarSeconds;
			Units[HitIndex].LastHitFrom = Start;
			Units[HitIndex].bWasHit = true;
			AddEvent(ECrowdEvent::BoltHit, Units[HitIndex].Position, Units[HitIndex].Kind, Units[HitIndex].Team);
			Bolts.RemoveAtSwap(BoltIndex);
			continue;
		}
		if (WallAlpha <= 1.0)
		{
			AddEvent(ECrowdEvent::BoltWall, Start + Travel * WallAlpha);
			Bolts.RemoveAtSwap(BoltIndex);
			continue;
		}

		Bolt.Position = Start + Travel;
		Bolt.Life -= DeltaSeconds;
		if (Bolt.Life <= 0.0f)
		{
			Bolts.RemoveAtSwap(BoltIndex);
		}
	}
}

void FCrowdSim::RemoveDead(FCrowdStepResult& Result)
{
	// A death alerts the dead enemy's group. Without this, an enemy killed in one hit is gone before anyone sees it was hurt.
	TArray<int32, TInlineAllocator<8>> Alerted;
	for (int32 Index = Units.Num() - 1; Index >= 0; --Index)
	{
		const FCrowdUnit& Unit = Units[Index];
		if (Unit.Health > 0.0f)
		{
			continue;
		}
		{
			FCrowdEvent& Death = Events.AddDefaulted_GetRef();
			Death.Type = ECrowdEvent::Death;
			Death.Position = Unit.Position;
			Death.Kind = Unit.Kind;
			Death.Team = Unit.Team;
			Death.Facing = Unit.Facing;
			// Something that died of nothing that came from anywhere fell to a blow from the front.
			Death.From = Unit.bWasHit ? Unit.LastHitFrom : Unit.Position + Unit.Facing;
			Death.Id = Unit.Id;
		}
		if (Unit.Team == ECrowdTeam::Enemy)
		{
			if (Unit.Squad != INDEX_NONE)
			{
				Alerted.AddUnique(Unit.Squad);
			}
			++Result.EnemiesKilled;
			Result.CommandersKilled += Unit.Kind == ECrowdKind::Commander ? 1 : 0;
			// A kill leaves bones where the enemy fell. The wizard has to walk there to have them.
			// Each kind of enemy is worth its own number of bones, left as one pile.
			DropBone(Unit.Position, GetBoneValue(Unit.Kind));
		}
		else
		{
			++Result.SkeletonsLost;
			// One of the army that falls in a fight gives back part of what it cost.
			DropBone(Unit.Position, GetFallenBoneValue(Unit.Kind));
		}
		Units.RemoveAtSwap(Index);
	}

	// Indices moved; targets are chosen again at the start of the next step.
	for (FCrowdUnit& Unit : Units)
	{
		Unit.TargetIndex = INDEX_NONE;
	}

	for (const int32 Squad : Alerted)
	{
		bool bAnyWoken = false;
		for (FCrowdUnit& Unit : Units)
		{
			if (Unit.bAsleep && Unit.Squad == Squad)
			{
				// One of them was killed: they go for where the wizard is.
				Rouse(Unit, LastWizardPosition);
				bAnyWoken = true;
			}
		}
		Result.SquadsWoken += bAnyWoken ? 1 : 0;
	}
}

void FCrowdSim::UpdateBones(float DeltaSeconds, FVector2D WizardPosition, FCrowdStepResult& Result)
{
	const double PickupSquared = FMath::Square(static_cast<double>(Tuning.BonePickupRadius));
	for (int32 Index = Bones.Num() - 1; Index >= 0; --Index)
	{
		FCrowdBone& Bone = Bones[Index];
		if (FVector2D::DistSquared(Bone.Position, WizardPosition) <= PickupSquared)
		{
			if (Bone.bGuard)
			{
				GuardsOwed += Bone.Value;
			}
			else
			{
				BonesHeld += Bone.Value;
			}
			Result.BonesCollected += Bone.Value;
			AddEvent(ECrowdEvent::BonePicked, Bone.Position);
			Bones.RemoveAtSwap(Index);
			continue;
		}
		if (IsInView(Bone.Position))
		{
			// On screen it does not fade; its time starts again when it leaves the screen.
			Bone.Life = Tuning.BoneLife;
			continue;
		}
		Bone.Life -= DeltaSeconds;
		if (Bone.Life <= 0.0f)
		{
			Bones.RemoveAtSwap(Index);
		}
	}

	// Plain bones are held until the wizard casts Summon the Afterlife or spends them at the shrine.
	// A guard pile assembles a Guard. With a full army it waits until there is room.
	while (GuardsOwed > 0)
	{
		const double Angle = Random.FRandRange(0.0f, UE_TWO_PI);
		if (!AddSkeleton(WizardPosition + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * AssembleDistance, ECrowdKind::Guard))
		{
			break;
		}
		--GuardsOwed;
		++Result.SkeletonsRaised;
	}
}
