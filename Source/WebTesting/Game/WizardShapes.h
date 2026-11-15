#pragma once

#include "CoreMinimal.h"

class UMaterialInstanceDynamic;
class UPrimitiveComponent;
class UStaticMesh;

/** The first build draws everything with the engine's basic shapes, loaded on demand and tinted. */
namespace WizardShapes
{
	/** Name is one of Cylinder, Sphere, Cube, Cone, Plane. */
	UStaticMesh* Load(const TCHAR* Name);

	/** Gives the component its own tinted copy of the basic shape material. */
	UMaterialInstanceDynamic* Tint(UPrimitiveComponent* Component, const FLinearColor& Color);

	/**
	 * The same, with the material that is cut away in a circle around the cursor (its Cursor parameter), for things
	 * tall enough to hide the fight behind them. Falls back to the plain material when that one is missing.
	 */
	UMaterialInstanceDynamic* TintSeeThrough(UPrimitiveComponent* Component, const FLinearColor& Color);

	/**
	 * Gives the component its own copy of an effect material (a path in /Game/Level) for the caller to set up.
	 * Falls back to the plain material in the fallback color when that one is missing.
	 */
	UMaterialInstanceDynamic* Effect(UPrimitiveComponent* Component, const TCHAR* Path, const FLinearColor& Fallback);

	/** The Bolt's ball of fire, and the marks that lie on the ground (warnings, where the spear lands). */
	extern const TCHAR* const FireBoltPath;
	extern const TCHAR* const GroundMarkPath;
}
