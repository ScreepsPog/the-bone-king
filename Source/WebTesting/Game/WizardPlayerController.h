#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "WizardPlayerController.generated.h"

UCLASS()
class AWizardPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AWizardPlayerController();

	/** The point under the mouse on the wizard's ground plane. False when there is no mouse position or no pawn. */
	bool GetAimPoint(FVector& OutWorldPoint) const;
};
