#include "Game/WizardGameMode.h"

#include "Game/WizardHUD.h"
#include "Game/WizardPawn.h"
#include "Game/WizardPlayerController.h"

AWizardGameMode::AWizardGameMode()
{
	DefaultPawnClass = AWizardPawn::StaticClass();
	PlayerControllerClass = AWizardPlayerController::StaticClass();
	HUDClass = AWizardHUD::StaticClass();
}
