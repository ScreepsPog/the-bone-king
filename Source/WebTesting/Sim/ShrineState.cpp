#include "Sim/ShrineState.h"

bool FShrineState::IsInRange(FVector2D WizardPosition) const
{
	return FVector2D::DistSquared(WizardPosition, Position) <= FMath::Square(static_cast<double>(Range));
}

bool FShrineState::TryOpen(FVector2D WizardPosition)
{
	if (IsInRange(WizardPosition))
	{
		bOpen = true;
	}
	return bOpen;
}

bool FShrineState::Update(FVector2D WizardPosition)
{
	if (bOpen && !IsInRange(WizardPosition))
	{
		bOpen = false;
		return true;
	}
	return false;
}

float FShrineState::GetTimeScale(float OpenScale) const
{
	return bOpen ? FMath::Clamp(OpenScale, 0.05f, 1.0f) : 1.0f;
}

bool FShrineState::AllowsAbilities() const
{
	return !bOpen;
}
