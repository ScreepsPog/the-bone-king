#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LevelActors.generated.h"

class USplineComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

namespace LevelRead
{
	/** Where a placed actor stands, read from what the level saved, so it is right before the level is in a running world. */
	FTransform GetPlacedTransform(const AActor& Actor);
}

/**
 * The shrine, placed in the level. The ring on the ground is its range: how close the wizard must be to use it. The
 * ring pulses in the editor exactly as it does in the game (switch Realtime on in the viewport to see it move).
 */
UCLASS()
class AShrineActor : public AActor
{
	GENERATED_BODY()

public:
	AShrineActor();

	virtual void OnConstruction(const FTransform& Transform) override;

	/** The wizard can use the shrine within this distance of its middle, and its menu closes past it. */
	UPROPERTY(EditAnywhere, Category = "Shrine", meta = (ClampMin = "100.0"))
	float Range = 700.0f;

	/** The shrine's bounds on the ground, from the level's saved data. Units and Bolts stop at it. */
	FBox GetBlockingBounds() const;

	UStaticMeshComponent* GetMesh() const { return Mesh; }

private:
	UPROPERTY(VisibleAnywhere, Category = "Shrine")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, Category = "Shrine")
	TObjectPtr<UStaticMeshComponent> RangeRing;
};

/**
 * A post: a group of enemies that stands here until it notices the wizard. The disc is the ground the group stands
 * and walks about on; the words over it say who is posted. Both are seen in the editor only.
 */
UCLASS()
class AEnemyPostActor : public AActor
{
	GENERATED_BODY()

public:
	AEnemyPostActor();

	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(EditAnywhere, Category = "Post", meta = (ClampMin = "0", ClampMax = "60"))
	int32 Grunts = 6;

	UPROPERTY(EditAnywhere, Category = "Post", meta = (ClampMin = "0", ClampMax = "30"))
	int32 Archers = 0;

	UPROPERTY(EditAnywhere, Category = "Post", meta = (ClampMin = "0", ClampMax = "20"))
	int32 Brutes = 0;

	/** The commander stands here, and those above are his escort. One post in a level: taking him takes the area. */
	UPROPERTY(EditAnywhere, Category = "Post")
	bool bCommander = false;

	/** The group stands and walks about within this distance of the post. Too small for the group and they jostle. */
	UPROPERTY(EditAnywhere, Category = "Post", meta = (ClampMin = "100.0"))
	float Radius = 350.0f;

	int32 GetCount() const { return Grunts + Archers + Brutes + (bCommander ? 1 : 0); }

	/** The least radius in which a group of this many has room to walk about without bumping. */
	static float GetRoomFor(int32 Count);

private:
#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Area;

	UPROPERTY()
	TObjectPtr<UTextRenderComponent> Label;
#endif
};

/**
 * The kingdom gate: the ground the wizard walks onto to finish the game. It is shut while the commander lives. The
 * marking on the ground is seen in the editor always, and in the game once the gate is open. Turn the actor so its
 * arrow (X) points the way through; Width runs across it.
 */
UCLASS()
class AKingdomGateActor : public AActor
{
	GENERATED_BODY()

public:
	AKingdomGateActor();

	virtual void OnConstruction(const FTransform& Transform) override;

	/** How wide the gate's ground is, across the way through, before the actor's scale. */
	UPROPERTY(EditAnywhere, Category = "Gate", meta = (ClampMin = "100.0"))
	float Width = 600.0f;

	/** How deep it is, along the way through, before the actor's scale. */
	UPROPERTY(EditAnywhere, Category = "Gate", meta = (ClampMin = "100.0"))
	float Depth = 300.0f;

	/** Half the depth and half the width on the ground, with the actor's scale, from the level's saved data. */
	FVector2D GetHalfSize() const;

	/** Shows the marking in the game (the gate is open) or hides it (shut). */
	void SetOpenShown(bool bOpen);

private:
	UPROPERTY(VisibleAnywhere, Category = "Gate")
	TObjectPtr<UStaticMeshComponent> Area;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UTextRenderComponent> Label;
#endif
};

/**
 * A wall: a box nothing walks, shoots or blinks through. Seen, moved and sized in the editor. In the game the layout
 * actor draws every wall (cut away around the cursor, so what is behind one can be seen), and this box is hidden.
 * A wall's box lies along the level's X and Y: one that is turned takes up the box around it.
 */
UCLASS()
class AWallActor : public AActor
{
	GENERATED_BODY()

public:
	AWallActor();

	virtual void OnConstruction(const FTransform& Transform) override;

	/** How long and how thick the wall is on the ground, before the actor's scale. */
	UPROPERTY(EditAnywhere, Category = "Wall", meta = (ClampMin = "10.0"))
	FVector2D Size = FVector2D(400.0, 60.0);

	/** How tall it is drawn in the editor. In the game every wall is as tall as the layout actor draws them. */
	UPROPERTY(EditAnywhere, Category = "Wall", meta = (ClampMin = "10.0"))
	float Height = 160.0f;

	/** The wall's box on the ground: its middle and half its size, with the actor's place, turn and scale, from the level's saved data. */
	void GetBox(FVector2D& OutCenter, FVector2D& OutHalfSize) const;

	/** Half the size of the box, along X and Y, around a wall of this size turned by Yaw (degrees) and scaled. */
	static FVector2D MakeHalfSize(FVector2D Size, float YawDegrees, FVector2D Scale);

private:
	UPROPERTY(VisibleAnywhere, Category = "Wall")
	TObjectPtr<UStaticMeshComponent> Body;
};

/**
 * The battlefield: a box on the ground with no wall. Outside it the wizard is warned to return, and the mission
 * fails if he stays out too long or goes too far (BattlefieldWarnSeconds, BattlefieldFarDistance). Seen in the
 * editor as a wire box; size it there. A level with one has no walls around it.
 */
UCLASS()
class ABattlefieldActor : public AActor
{
	GENERATED_BODY()

public:
	ABattlefieldActor();

	virtual void OnConstruction(const FTransform& Transform) override;

	/** How long and how wide the battlefield is, along the level's X and Y, before the actor's scale. */
	UPROPERTY(EditAnywhere, Category = "Battlefield", meta = (ClampMin = "500.0"))
	FVector2D Size = FVector2D(10000.0, 10000.0);

	/** The battlefield's box on the ground: its middle and half its size, from the level's saved data. */
	void GetBox(FVector2D& OutCenter, FVector2D& OutHalfSize) const;

private:
	UPROPERTY(VisibleAnywhere, Category = "Battlefield")
	TObjectPtr<class UBoxComponent> Edge;
};

/** A road a patrol walks end to end and back: the spline's points, in order. */
UCLASS()
class APatrolRouteActor : public AActor
{
	GENERATED_BODY()

public:
	APatrolRouteActor();

	/** Grunts in the patrol that walks this road. 0: none. */
	UPROPERTY(EditAnywhere, Category = "Patrol", meta = (ClampMin = "0", ClampMax = "30"))
	int32 PatrolSize = 5;

	/** The road's points on the ground. */
	TArray<FVector2D> GetPoints() const;

	void SetPoints(const TArray<FVector2D>& Points);

private:
	UPROPERTY(VisibleAnywhere, Category = "Patrol")
	TObjectPtr<USplineComponent> Road;
};
