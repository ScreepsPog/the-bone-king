#include "Sim/KingdomGate.h"

void FKingdomGate::Place(FVector2D InCenter, FVector2D InHalfSize, float InYaw)
{
	bPlaced = true;
	Center = InCenter;
	HalfSize = InHalfSize.GetAbs();
	Yaw = InYaw;
	Reset();
}

void FKingdomGate::Reset()
{
	bOpen = false;
	bEntered = false;
	EnteredSeconds = 0.0f;
}

bool FKingdomGate::Contains(FVector2D Point) const
{
	// The point in the gate's own frame: turned back by its yaw.
	const FVector2D Local = (Point - Center).GetRotated(-Yaw);
	return FMath::Abs(Local.X) <= HalfSize.X && FMath::Abs(Local.Y) <= HalfSize.Y;
}

bool FKingdomGate::Update(bool bCommanderDead, FVector2D WizardPosition, bool bCanEnter, float Seconds)
{
	if (!bPlaced)
	{
		return false;
	}
	if (bEntered)
	{
		EnteredSeconds += FMath::Max(Seconds, 0.0f);
		return false;
	}
	bOpen = bOpen || bCommanderDead;
	if (bOpen && bCanEnter && Contains(WizardPosition))
	{
		bEntered = true;
		EnteredSeconds = 0.0f;
		return true;
	}
	return false;
}
