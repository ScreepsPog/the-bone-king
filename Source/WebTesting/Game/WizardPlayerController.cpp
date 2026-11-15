#include "Game/WizardPlayerController.h"

#include "GameFramework/Pawn.h"

AWizardPlayerController::AWizardPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;
}

bool AWizardPlayerController::GetAimPoint(FVector& OutWorldPoint) const
{
	const APawn* ControlledPawn = GetPawn();
	FVector Origin;
	FVector Direction;
	if (!ControlledPawn || !DeprojectMousePositionToWorld(Origin, Direction))
	{
		return false;
	}

	const FVector PawnLocation = ControlledPawn->GetActorLocation();
	if (Direction.Z < -UE_KINDA_SMALL_NUMBER)
	{
		const double Distance = (PawnLocation.Z - Origin.Z) / Direction.Z;
		if (Distance > 0.0)
		{
			OutWorldPoint = Origin + Direction * Distance;
			return true;
		}
	}

	// The mouse is at or above the horizon (Third Person): aim far along the view direction instead.
	const FVector Flat = FVector(Direction.X, Direction.Y, 0.0).GetSafeNormal();
	if (Flat.IsNearlyZero())
	{
		return false;
	}
	OutWorldPoint = PawnLocation + Flat * 5000.0;
	return true;
}
