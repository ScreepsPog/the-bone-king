#pragma once

#include "CoreMinimal.h"
#include "Sim/LevelLayout.h"

/** How each kind of level piece looks, shared by the game's picture and by the piece actors placed in a level. */
namespace LevelPieceLook
{
	// A basic shape, a color, and a height. A height of zero is a flat plate on the ground, lifted by Lift so plates
	// do not flicker through each other.
	struct FPieceLook
	{
		const TCHAR* Shape;
		FLinearColor Color;
		double Height;
		double Lift;
	};

	inline const FPieceLook& GetPieceLook(ELevelPiece Kind)
	{
		static const FPieceLook Looks[] = {
			/* House */ { TEXT("Cube"), FLinearColor(0.2f, 0.17f, 0.15f), 420.0, 0.0 },
			/* Medical */ { TEXT("Cube"), FLinearColor(0.62f, 0.58f, 0.52f), 480.0, 0.0 },
			/* Dock */ { TEXT("Cube"), FLinearColor(0.25f, 0.14f, 0.06f), 90.0, 0.0 },
			/* Coal */ { TEXT("Cube"), FLinearColor(0.02f, 0.02f, 0.025f), 200.0, 0.0 },
			/* Tree */ { TEXT("Cylinder"), FLinearColor(0.13f, 0.08f, 0.04f), 230.0, 0.0 },
			/* River */ { TEXT("Cube"), FLinearColor(0.03f, 0.16f, 0.5f), 0.0, 7.0 },
			/* Bridge */ { TEXT("Cube"), FLinearColor(0.3f, 0.2f, 0.1f), 0.0, 10.0 },
			/* Path */ { TEXT("Cube"), FLinearColor(0.24f, 0.17f, 0.1f), 0.0, 5.0 },
			/* Farmland */ { TEXT("Cube"), FLinearColor(0.2f, 0.36f, 0.06f), 0.0, 3.5 },
			/* Graveyard */ { TEXT("Cube"), FLinearColor(0.1f, 0.13f, 0.12f), 0.0, 4.0 },
			/* Lamp */ { TEXT("Cylinder"), FLinearColor(0.03f, 0.03f, 0.03f), 300.0, 0.0 },
		};
		static_assert(UE_ARRAY_COUNT(Looks) == static_cast<int32>(ELevelPiece::Count), "one look for each kind of piece");
		return Looks[static_cast<int32>(Kind)];
	}

	inline const double PlateThickness = 3.0;
	inline const double TreeTrunkAcross = 50.0;
	inline const FLinearColor TreeTopColor(0.04f, 0.17f, 0.07f);
	inline const FVector TreeTopScale(2.4, 2.4, 3.4);
	inline const double LampPostAcross = 14.0;
	inline const FLinearColor LampGlowColor(1.0f, 0.8f, 0.15f);
	inline const FVector LampGlowScale(0.5, 0.5, 0.5);
}
