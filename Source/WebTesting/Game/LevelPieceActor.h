#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Sim/LevelLayout.h"
#include "LevelPieceActor.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

/**
 * One piece of a level, placed in the editor: a house box, a tree, a stretch of river, a road, a field. Move it, turn
 * it, scale it, change its kind or delete it. What is seen in the editor is what is in the game: the same mesh, with
 * its kind's material from /Game/Level (MI_Piece_<kind>: a texture and a tint). When a run starts the game reads
 * every piece in the level, and those that block (houses, trees, water) stop units.
 */
UCLASS()
class ALevelPieceActor : public AActor
{
	GENERATED_BODY()

public:
	ALevelPieceActor();

	virtual void OnConstruction(const FTransform& Transform) override;

	/** What it is. Houses, medical buildings, docks, coal, trees and river block; the rest are ground. */
	UPROPERTY(EditAnywhere, Category = "Level Piece")
	ELevelPiece Kind = ELevelPiece::House;

	/** How wide and deep it is, in units, before the actor's own scale. For a tree or a lamp: the ground it blocks. */
	UPROPERTY(EditAnywhere, Category = "Level Piece", meta = (ClampMin = "10.0"))
	FVector2D Size = FVector2D(400.0, 400.0);

	/** The piece as the game reads it: where the actor is, its size times its scale, its turn. */
	FLevelPiece MakePiece() const;

	/** Sets the actor from a piece of a layout. */
	void SetPiece(const FLevelPiece& Piece);

private:
	UPROPERTY(VisibleAnywhere, Category = "Level Piece")
	TObjectPtr<UStaticMeshComponent> Body;

	// A tree's top, a lamp's glow.
	UPROPERTY(VisibleAnywhere, Category = "Level Piece")
	TObjectPtr<UStaticMeshComponent> Top;

	// Water: the unseen wall that keeps the wizard out of it.
	UPROPERTY(VisibleAnywhere, Category = "Level Piece")
	TObjectPtr<UBoxComponent> Blocker;
};
