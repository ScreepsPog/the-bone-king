#pragma once

#include "Sim/CrowdSim.h"

namespace CrowdTest
{
	inline const FVector2D Origin(0.0, 0.0);

	/** Enemies walk a straight line at exactly the kind's speed. Call before adding enemies. */
	inline void MakeEnemiesPlain(FCrowdSim& Sim)
	{
		Sim.Tuning.EnemyFlankDegrees = 0.0f;
		Sim.Tuning.EnemySpeedVariation = 0.0f;
	}

	/** Skeletons stand where they are put and do no damage. */
	inline void FreezeArmy(FCrowdSim& Sim)
	{
		Sim.Tuning.SkeletonSpeed = 0.0f;
		Sim.Tuning.SkeletonRoamSpeed = 0.0f;
		Sim.Tuning.SkeletonCatchUpSpeed = 0.0f;
		Sim.Tuning.SkeletonDamagePerSecond = 0.0f;
	}

	inline const FCrowdUnit* First(const FCrowdSim& Sim, ECrowdTeam Team)
	{
		for (const FCrowdUnit& Unit : Sim.GetUnits())
		{
			if (Unit.Team == Team)
			{
				return &Unit;
			}
		}
		return nullptr;
	}

	inline int32 CountTouching(const FCrowdSim& Sim, FVector2D Wizard)
	{
		TArray<FVector2D> Offsets;
		Sim.GetEnemiesTouching(Wizard, Offsets);
		return Offsets.Num();
	}

	/** True when any unit's middle is closer to any wall than its radius allows (2 units of tolerance). */
	inline bool AnyUnitInWall(const FCrowdSim& Sim)
	{
		for (const FCrowdUnit& Unit : Sim.GetUnits())
		{
			for (const FCrowdObstacle& Obstacle : Sim.GetObstacles())
			{
				if (Obstacle.Contains(Unit.Position, Sim.Tuning.UnitRadius - 2.0))
				{
					return true;
				}
			}
		}
		return false;
	}

	inline FCrowdObstacle Wall(double CenterX, double CenterY, double HalfX, double HalfY)
	{
		FCrowdObstacle Obstacle;
		Obstacle.Center = FVector2D(CenterX, CenterY);
		Obstacle.HalfSize = FVector2D(HalfX, HalfY);
		return Obstacle;
	}
}
