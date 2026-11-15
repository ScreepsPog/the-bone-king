#include "Game/WizardShapes.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "WebTesting.h"

const TCHAR* const WizardShapes::FireBoltPath = TEXT("/Game/Level/M_FireBolt.M_FireBolt");
const TCHAR* const WizardShapes::GroundMarkPath = TEXT("/Game/Level/M_GroundMark.M_GroundMark");

UMaterialInstanceDynamic* WizardShapes::Effect(UPrimitiveComponent* Component, const TCHAR* Path, const FLinearColor& Fallback)
{
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, Path);
	if (!Component || !Base)
	{
		return Tint(Component, Fallback);
	}
	UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Base, Component);
	Component->SetMaterial(0, Material);
	return Material;
}

UStaticMesh* WizardShapes::Load(const TCHAR* Name)
{
	const FString Path = FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"), Name, Name);
	UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *Path);
	if (!Mesh)
	{
		UE_LOG(LogWizard, Error, TEXT("Basic shape not found: %s"), *Path);
	}
	return Mesh;
}

UMaterialInstanceDynamic* WizardShapes::TintSeeThrough(UPrimitiveComponent* Component, const FLinearColor& Color)
{
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Level/M_SeeThrough.M_SeeThrough"));
	if (!Component || !Base)
	{
		return Tint(Component, Color);
	}
	UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Base, Component);
	Material->SetVectorParameterValue(TEXT("Color"), Color);
	Component->SetMaterial(0, Material);
	return Material;
}

UMaterialInstanceDynamic* WizardShapes::Tint(UPrimitiveComponent* Component, const FLinearColor& Color)
{
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (!Component || !Base)
	{
		UE_LOG(LogWizard, Error, TEXT("Basic shape material not found"));
		return nullptr;
	}
	UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Base, Component);
	Material->SetVectorParameterValue(TEXT("Color"), Color);
	Component->SetMaterial(0, Material);
	return Material;
}
