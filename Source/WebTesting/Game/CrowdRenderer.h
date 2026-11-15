#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Game/DeathPick.h"
#include "Sim/LevelLayout.h"
#include "CrowdRenderer.generated.h"

class FCrowdSim;
struct FCrowdObstacle;
class UInstancedStaticMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;
class USkeletalMeshComponent;
class USkeletalMesh;
class UMaterialInstanceDynamic;
class UAnimSequence;
class UAnimSequenceBase;
struct FCrowdTuning;
enum class ECrowdKind : uint8;

/** Draws the whole crowd with one instanced component per kind of unit. It has no tick: the crowd manager refreshes it. */
UCLASS()
class ACrowdRenderer : public AActor
{
	GENERATED_BODY()

public:
	ACrowdRenderer();

	/** In the editor (not in a game) this draws the Outskirts as it will be laid out, so buildings can be placed against it. */
	virtual void OnConstruction(const FTransform& Transform) override;

	/**
	 * The plain boxes that stand in for houses. Switch it off once real buildings are placed: the boxes are then
	 * neither drawn nor in anyone's way.
	 */
	UPROPERTY(EditAnywhere, Category = "Level", meta = (DisplayName = "Show House Boxes"))
	bool bShowHouseBoxes = true;

	/**
	 * On: the level's pieces (houses, trees, river, roads, fields) are the Level Piece actors placed in the level, and
	 * the layout made in code is not used for them. Off: the pieces come from code and nothing in the level moves them.
	 */
	UPROPERTY(EditAnywhere, Category = "Level", meta = (DisplayName = "Use Placed Pieces"))
	bool bUsePlacedPieces = false;

	bool UsesPlacedPieces() const { return bUsePlacedPieces; }

#if WITH_EDITOR
	/** Puts every piece of the code-made layout into the level as its own actor, once, and switches Use Placed Pieces on. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Level", meta = (DisplayName = "Place Pieces As Actors"))
	void PlacePiecesAsActors();

	/**
	 * Puts the shrine, every post, the commander and every patrol road of the code-made layout into the level as
	 * actors, once, and moves the player start to where the wizard begins.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Level", meta = (DisplayName = "Place Systems As Actors"))
	void PlaceSystemsAsActors();

	/** Puts the kingdom gate of the code-made layout into the level as an actor, once. Move, turn and size it there. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Level", meta = (DisplayName = "Place Gate As Actor"))
	void PlaceGateAsActor();

	/** Gives a mesh one solid box the size of its bounds, when it has no solid shape of its own. True when it has one after. */
	UFUNCTION(BlueprintCallable, Category = "Level")
	static bool MakeMeshSolid(UStaticMesh* Mesh);

	/** Builds every placed piece, post and shrine again from its settings: after their materials or code have changed. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Level", meta = (DisplayName = "Refresh Placed Actors"))
	void RefreshPlacedActors();

	/** Places that must stay clear in the Outskirts: the start, the shrine, the commander, every post and every point of a patrol's road. For level tools. */
	UFUNCTION(BlueprintCallable, Category = "Level")
	static TArray<FVector2D> GetLayoutKeyPoints();
#endif

	bool AreHouseBoxesShown() const { return bShowHouseBoxes; }
	void SetHouseBoxesShown(bool bShown) { bShowHouseBoxes = bShown; bPiecesChanged = true; }

	/** Buildings placed in the level by hand: each gets an unseen box that stops the wizard. Shown at the next refresh. */
	void SetBuildingBlockers(const TArray<FCrowdObstacle>& Boxes) { BuildingBlockers = Boxes; bPiecesChanged = true; }

	/** Copies every unit and bolt position from the simulation into the instances. Units are sized beside a wizard WizardHeight tall. */
	void Refresh(const FCrowdSim& Sim, float GroundZ, float WizardHeight, FVector2D WizardPosition);

	/** The walls to draw. They also block the wizard. Shown at the next refresh. */
	void SetObstacles(const TArray<FCrowdObstacle>& Obstacles);

	/** Where the shrine stands. Shown at the next refresh. */
	void SetShrine(FVector2D Position) { ShrinePosition = Position; bShrineChanged = true; }

	/** False when the level has a shrine of its own placed in it: the stand-in pillar is then not drawn. */
	void SetShrineShown(bool bShown);

	/** The pieces of the level to draw, each kind as its own basic shape and color. Those that block also stop the wizard. Shown at the next refresh. */
	void SetPieces(const TArray<FLevelPiece>& InPieces) { Pieces = InPieces; bPiecesChanged = true; }

	/**
	 * Draws the army's plain skeletons as the skeleton model (true) or as simple shapes. The model is used only when
	 * its assets loaded.
	 */
	void SetArmyModelShown(bool bShown) { bArmyModelWanted = bShown; }
	bool IsArmyModelShown() const { return bArmyModelWanted && bArmyModelLoaded; }
	int32 GetArmyModelsDrawn() const { return ArmyModelsDrawn; }
	/** Hidden figures that have the swing animation to play: six for each kind that strikes, 0 before the models are set up. */
	int32 GetSwingsReady() const;

	/** What drawing the models has cost since the last reset: for finding hitches. Times are in milliseconds. */
	struct FModelCost
	{
		int32 Frames = 0;
		double FrameMs = 0.0;
		double FramePeakMs = 0.0;
		double ModelMs = 0.0;
		double ModelPeakMs = 0.0;
		int32 Creates = 0;
		double CreateMs = 0.0;
		int32 Switches = 0;
		double SwitchMs = 0.0;
		int32 Flips = 0;
		double FlipMs = 0.0;
	};
	/** The cost over the last full three seconds: what the debug overlay shows. */
	const FModelCost& GetModelCostWindow() const { return WindowShown; }
	/**
	 * A test switch for finding hitches: 0 normal; 1 figures never change the pose they copy (all stand); 2 figures
	 * that are not needed stay shown, parked under the ground, in place of being hidden.
	 */
	void SetPerfTest(int32 Mode) { PerfTest = Mode; }
	int32 GetPerfTest() const { return PerfTest; }
	static constexpr int32 PerfTestCount = 3;
	const FModelCost& GetModelCost() const { return ModelCost; }
	void ResetModelCost() { ModelCost = FModelCost(); LastModelTime = 0.0; }

	/**
	 * Where buildings and walls are cut away so the player can see behind them: a circle around CursorUV (0 to 1
	 * across and down the screen), RadiusShare of the screen's height wide. Aspect is the screen's width over its
	 * height. A radius of 0 switches it off.
	 */
	void SetSeeThrough(FVector2D CursorUV, float Aspect, float RadiusShare);

	/** How many materials are cut away around the cursor: 0 when the see-through material is missing. */
	int32 GetSeeThroughCount() const { return SeeThroughMaterials.Num(); }

	/** The kinds of unit that have a model: the plain skeleton, the Guard, the grunt, the assassin, the archer, the brute. */
	static constexpr int32 ModelKindCount = 8;

	/** A kind has up to this many attacks: two blows to choose between, or (the commander) a slash and a throw. */
	static constexpr int32 ModelSwingSets = 2;

	/** The most figures drawn as models at once. Those past it (the enemies farthest from the wizard) are simple shapes. */
	static constexpr int32 ModelCap = 140;

	/**
	 * Which swing of several, all the same swing started at even steps through one blow's interval, lands its blow
	 * nearest to when this unit's blow lands. Clock is the simulation's clock, TimeToBlow the seconds until the unit's
	 * next blow, Interval the seconds between blows, HitShare how far through the swing the blow lands (0 to 1).
	 */
	static int32 PickSwing(double Clock, float TimeToBlow, float Interval, float HitShare, int32 SwingCount);

	/**
	 * How far through an attack animation (0 to 1) its blow lands: where its notify named "Fire" is. The director
	 * puts a "Fire" notify on each attack at the moment the sword should hit or the arrow leave. Fallback when the
	 * animation has none.
	 */
	static float GetFireShare(const UAnimSequenceBase* Animation, float Fallback);

	/**
	 * Seconds into an animation of its first notify with one of these names (the name of a plain notify, or of a
	 * montage notify). Below zero when it has none.
	 */
	static float GetNotifySeconds(const UAnimSequenceBase* Animation, TConstArrayView<FName> Names);

	/** What each kind of figure holds and by which socket, for a tester: "Skeleton:SM_Steel_Sword@Sword_Socket ...". */
	FString GetWeaponSummary() const;

	/** The material and shape each effect is drawn with, for a tester: "Bolt:M_FireBolt/Sphere Slam:M_GroundMark/Plane ...". */
	FString GetEffectSummary() const;

	/**
	 * The seconds one turn of a kind's attack animation is stretched over, so that its "Fire" moment lands with the
	 * simulation's blow: the time between blows for those that strike or shoot again and again, and for a slam the
	 * time that puts the animation's start at the slam's start.
	 */
	static float GetStrikeInterval(const FCrowdTuning& Tuning, ECrowdKind Kind, float HitShare);

#if WITH_EDITOR
	/**
	 * Gives every building mesh used in the level one solid box that is its walls without its eaves (the share of its
	 * bounds the simulation blocks), in place of whatever it had. Returns how many meshes were changed; save them after.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Level", meta = (DisplayName = "Tighten Building Collision"))
	int32 TightenBuildingCollision();

	/**
	 * Gives the bridge model collision made for walking, in place of what it had: a deck of flat ramps that follows
	 * the top of the model from end to end (and runs on down into the ground past each end, so there is no lip to
	 * catch on), and one thin rail down each side where the model has rails. Returns how many boxes it made
	 * (0 when the model could not be read); save the mesh after.
	 */
	UFUNCTION(BlueprintCallable, Category = "Level")
	static int32 RebuildBridgeCollision();

	/**
	 * Reads the landscape's paint into a grid Size cells each way: a cell is dirt when the layers named in
	 * DirtLayers weigh more there than all the others. Returns "MinX,MinY,MaxX,MaxY|" and then one character a cell,
	 * row after row ('1' dirt, '0' grass); empty when the landscape could not be read. For
	 * Tools/Level/bake_ground_paint.py, which writes it into the game's settings.
	 */
	UFUNCTION(BlueprintCallable, Category = "Level")
	static FString BakeGroundPaint(AActor* LandscapeActor, int32 Size, const TArray<FString>& DirtLayers);

	/** Builds every placed piece and wall again from its settings, after the code that dresses them has changed. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Level", meta = (DisplayName = "Refresh Placed Pieces"))
	void RefreshPlacedPieces();

	/** Puts every wall of the code-made layout into the level as a Wall actor, once. Move and size them there. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Level", meta = (DisplayName = "Place Walls As Actors"))
	void PlaceWallsAsActors();
#endif

	/** How far through its swing (0 to 1) swing number Swing of SwingCount is at this time on the clock. */
	static float GetSwingShare(double Clock, float Interval, int32 Swing, int32 SwingCount);

	/**
	 * How one attack animation is laid over the time the simulation gives a blow. A blow that comes again and again
	 * (a sword, a bow) turns the whole animation once for each blow. A blow that is wound up and then rested from (a
	 * slam, a slash, a thrown spear) stretches the animation up to its "Fire" moment over the wind-up, and plays the
	 * rest at its own speed.
	 */
	struct FSwingTiming
	{
		// Seconds one turn takes, and how far into the turn the blow lands.
		float Cycle = 1.0f;
		float BlowAt = 0.45f;
		// Seconds into the animation that its blow lands, and how long the animation is.
		float HitTime = 0.45f;
		float Length = 1.0f;
		bool bOneShot = false;

		/** Where in the animation (seconds) a figure is, Seconds into the turn. */
		float PositionAt(float Seconds) const;
		/** Seconds after the blow that a wound-up attack has played out: after that the figure stands again. */
		float GetTail() const { return FMath::Max(Cycle - BlowAt, 0.0f); }
	};

	/** The timing of a kind's attack (Set 0 is its first, 1 its second) for an animation Length long whose blow is HitShare through it. */
	static FSwingTiming MakeSwingTiming(const FCrowdTuning& Tuning, ECrowdKind Kind, int32 Set, float HitShare, float Length);

	/** The animation at a path: the montage beside it with "Anim_" before its name when the director has made one, else the animation itself. */
	static UAnimSequenceBase* LoadAttack(const TCHAR* Path);

	/** Figures lying where they died, or falling. */
	int32 GetCorpsesShown() const { return Corpses.Num(); }
	int32 GetDeathsShown() const { return DeathsShown; }
	/** The last death shown: "front", "behind" or "side" (where the blow came from), or "none". */
	const TCHAR* GetLastDeathName() const { return DeathsShown > 0 ? FDeathPick::Name(LastDeath) : TEXT("none"); }
	/** Seconds into a death that the body meets the ground: its "Fall" notify, or the measured time. */
	float GetDeathFallSeconds(EDeathFall Fall) const;

	/** No more than this many lie on the ground at once: the oldest gives up its place. */
	static constexpr int32 CorpseCap = 24;
	/** A body lies this long after its fall, then sinks into the ground over CorpseSinkSeconds (slow at first). */
	static constexpr float CorpseRestSeconds = 2.5f;
	/**
	 * Where in its death (seconds) a body is, Age seconds after it died: it plays once and then HOLDS just short of
	 * the end. (Left to play out, a montage ends by letting go of the figure, which stood it up in its bind pose.)
	 */
	static float GetCorpseSeconds(float Age, float Length) { return FMath::Clamp(Age, 0.0f, FMath::Max(Length - CorpseHoldBefore, 0.0f)); }
	static constexpr float CorpseHoldBefore = 0.04f;
	static constexpr float CorpseSinkSeconds = 1.5f;

private:
	void SetUpShapes(float GroundZ);
	void SetUpArmyModels();
	void RefreshArmyModels(const FCrowdSim& Sim, float GroundZ, float WizardHeight, FVector2D WizardPosition, TBitArray<>& OutModeled);

	/** Standing, strolling, running: figures that run the animation blueprint. Then the swings: six for each of a kind's attacks. */
	static constexpr int32 ModelGaitCount = 3;
	static constexpr int32 ModelSwingCount = 6;
	static constexpr int32 ModelLeaderCount = ModelGaitCount + ModelSwingCount * ModelSwingSets;

	void SetUpDeaths();
	void RefreshCorpses(const FCrowdSim& Sim, float GroundZ, float WizardHeight);

	// The three deaths, by where the blow came from, and when in each the body meets the ground.
	UPROPERTY()
	TObjectPtr<UAnimSequenceBase> DeathAnimations[static_cast<int32>(EDeathFall::Count)];
	float DeathFallSeconds[static_cast<int32>(EDeathFall::Count)] = {};

	struct FCorpse
	{
		int32 Figure = INDEX_NONE;
		float Age = 0.0f;
		float Length = 0.0f;
		FVector Place = FVector::ZeroVector;
		float Height = 0.0f;
	};
	TArray<FCorpse> Corpses;
	// Figures for the dead: made when first needed, then used again.
	UPROPERTY()
	TArray<TObjectPtr<USkeletalMeshComponent>> CorpseFigures;
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> CorpseWeapons;
	TArray<bool> CorpseBusy;
	int32 DeathsShown = 0;
	EDeathFall LastDeath = EDeathFall::FromFront;
	bool bDeathsTried = false;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> SeeThroughMaterials;
	FLinearColor SeeThroughCursor = FLinearColor(-10.0f, -10.0f, 1.78f, 0.0f);

	// The same circle for the materials of what stands in the level (houses, trees): they read it from this collection.
	UPROPERTY()
	TObjectPtr<class UMaterialParameterCollection> SeeThroughCollection;

	// Bridges: what the crowd walks on top of. Their boxes on the ground, and the height of the deck at a place.
	TArray<TWeakObjectPtr<AActor>> FloorActors;
	TArray<FBox2D> FloorBoxes;
	void FindFloors();
	double GetFloorZ(FVector2D Position, double GroundZ) const;

	/**
	 * The models. For each kind, a few hidden figures do the animating: three run the animation blueprint (standing,
	 * strolling, running) and six play the kind's attack, each started a sixth of a blow's interval after the last.
	 * Every figure on screen copies the pose of one of them, so a hundred figures cost nine animations a kind. One
	 * that is striking copies the swing whose blow lands when its own does. (All that stroll step together. A note
	 * for later: bake the animations into the mesh, or give each its own, when they should differ.)
	 */
	UPROPERTY()
	TObjectPtr<USkeletalMesh> ModelMeshes[ModelKindCount];

	// For each kind: the attack its swing figures play (the unarmed swing where a kind has none of its own), and how
	// far through it the blow lands (its "Fire" notify).
	UPROPERTY()
	TObjectPtr<UAnimSequenceBase> AttackAnimations[ModelKindCount * ModelSwingSets];
	float AttackHitShares[ModelKindCount * ModelSwingSets] = {};

	// For each kind: what its figures hold (a bow), or null.
	UPROPERTY()
	TObjectPtr<UStaticMesh> WeaponMeshes[ModelKindCount];

	// For each kind: a second thing it holds only while it throws it (the commander's spear), or null; and the
	// sockets the two are held by.
	UPROPERTY()
	TObjectPtr<UStaticMesh> SecondWeaponMeshes[ModelKindCount];
	FName WeaponSockets[ModelKindCount];
	FName SecondWeaponSockets[ModelKindCount];

	// For each figure in ModelPool: the second thing it holds, or null.
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> ModelSecondWeapons;

	// For each figure in ModelPool: what it holds, or null.
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> ModelWeapons;

	// ModelLeaderCount for each kind, in order.
	UPROPERTY()
	TArray<TObjectPtr<USkeletalMeshComponent>> ModelLeaders;

	// Every figure that can be shown, of every kind.
	UPROPERTY()
	TArray<TObjectPtr<USkeletalMeshComponent>> ModelPool;

	struct FArmyModelState
	{
		float Yaw = 0.0f;
		float Speed = 0.0f;
		uint32 Stamp = 0;
		// Which of its kind's attacks it is in, and how far through the turn it was last frame (below zero: it was not striking).
		int32 Set = 0;
		float LastPhase = -1.0f;
	};
	// By unit Id: which way each figure faces and how fast it has been going.
	TMap<int32, FArmyModelState> ArmyStates;
	// For each figure in ModelPool: the hidden figure it copies now.
	TArray<int32> ModelPoolLeader;
	// For each kind: its figures in ModelPool.
	TArray<int32> ModelSlots[ModelKindCount];
	float ModelAssetHeights[ModelKindCount] = { 150.0f, 175.0f, 170.0f, 165.0f, 165.0f, 240.0f, 160.0f, 286.0f };
	bool bModelLoaded[ModelKindCount] = {};
	uint32 ArmyStamp = 0;
	int32 ArmyModelsDrawn = 0;
	FModelCost ModelCost;
	FModelCost Window;
	FModelCost WindowShown;
	double WindowStart = 0.0;
	int32 PerfTest = 0;
	double LastModelTime = 0.0;
	bool bArmyModelWanted = true;
	bool bArmyModelLoaded = false;
	bool bArmyModelTried = false;

	void RebuildPieces(float GroundZ);
	static void Apply(UInstancedStaticMeshComponent* Component, const TArray<FTransform>& Transforms);

	UPROPERTY(VisibleAnywhere, Category = "Crowd")
	TObjectPtr<UInstancedStaticMeshComponent> Skeletons;

	UPROPERTY(VisibleAnywhere, Category = "Crowd")
	TObjectPtr<UInstancedStaticMeshComponent> Grunts;

	UPROPERTY(VisibleAnywhere, Category = "Crowd")
	TObjectPtr<UInstancedStaticMeshComponent> Bolts;

	UPROPERTY(VisibleAnywhere, Category = "Crowd")
	TObjectPtr<UInstancedStaticMeshComponent> Brutes;

	UPROPERTY(VisibleAnywhere, Category = "Crowd")
	TObjectPtr<UInstancedStaticMeshComponent> Guards;

	UPROPERTY(VisibleAnywhere, Category = "Crowd")
	TObjectPtr<UInstancedStaticMeshComponent> Commanders;

	UPROPERTY(VisibleAnywhere, Category = "Crowd")
	TObjectPtr<UInstancedStaticMeshComponent> Archers;

	/** Only the assassins close enough to the wizard to be seen. */
	UPROPERTY(VisibleAnywhere, Category = "Crowd")
	TObjectPtr<UInstancedStaticMeshComponent> Assassins;

	UPROPERTY(VisibleAnywhere, Category = "Crowd")
	TObjectPtr<UInstancedStaticMeshComponent> Arrows;

	// The Marksmen's arrows: their own model. (Arrows holds the enemy archers' arrows and the commander's spear.)
	UPROPERTY(VisibleAnywhere, Category = "Crowd")
	TObjectPtr<UInstancedStaticMeshComponent> ArmyArrows;

	// The commander's thrown spear: its own model.
	UPROPERTY(VisibleAnywhere, Category = "Crowd")
	TObjectPtr<UInstancedStaticMeshComponent> Javelins;

	UPROPERTY(VisibleAnywhere, Category = "Crowd")
	TObjectPtr<UInstancedStaticMeshComponent> Spears;

	UPROPERTY(VisibleAnywhere, Category = "Crowd")
	TObjectPtr<UInstancedStaticMeshComponent> Blasts;

	/** The cut where a skeleton was taken for the spear. */
	UPROPERTY(VisibleAnywhere, Category = "Crowd")
	TObjectPtr<UInstancedStaticMeshComponent> Slashes;

	/** A brute's slam: the red circle while it grows, the circle at full size, and the flash where one landed. */
	UPROPERTY(VisibleAnywhere, Category = "Crowd")
	TObjectPtr<UInstancedStaticMeshComponent> SlamRings;

	UPROPERTY(VisibleAnywhere, Category = "Crowd")
	TObjectPtr<UInstancedStaticMeshComponent> SlamFulls;

	// The lane of the commander's spear throw: dark while it follows the wizard, bright once it holds.
	UPROPERTY(VisibleAnywhere, Category = "Crowd")
	TObjectPtr<UInstancedStaticMeshComponent> ThrowLanes;

	// The reach of a brute's sweep: a slice of a circle on the ground, and its material (the slice's width is set on it).
	UPROPERTY(VisibleAnywhere, Category = "Wizard")
	TObjectPtr<UInstancedStaticMeshComponent> SweepFans;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> SweepFanMaterial;

	UPROPERTY(VisibleAnywhere, Category = "Crowd")
	TObjectPtr<UInstancedStaticMeshComponent> ThrowLocks;

	UPROPERTY(VisibleAnywhere, Category = "Crowd")
	TObjectPtr<UInstancedStaticMeshComponent> SlamFlashes;

	/** The weak point on each brute's back. */
	UPROPERTY(VisibleAnywhere, Category = "Crowd")
	TObjectPtr<UInstancedStaticMeshComponent> BruteBacks;

	UPROPERTY(VisibleAnywhere, Category = "Crowd")
	TObjectPtr<UInstancedStaticMeshComponent> Bones;

	UPROPERTY(VisibleAnywhere, Category = "Crowd")
	TObjectPtr<UInstancedStaticMeshComponent> Walls;

	UPROPERTY(VisibleAnywhere, Category = "Crowd")
	TObjectPtr<UStaticMeshComponent> Ground;

	/** One instanced component for each kind of level piece. */
	UPROPERTY(VisibleAnywhere, Category = "Level")
	TObjectPtr<UInstancedStaticMeshComponent> PieceMeshes[static_cast<int32>(ELevelPiece::Count)];

	UPROPERTY(VisibleAnywhere, Category = "Level")
	TObjectPtr<UInstancedStaticMeshComponent> TreeTops;

	UPROPERTY(VisibleAnywhere, Category = "Level")
	TObjectPtr<UInstancedStaticMeshComponent> LampGlows;

	/** Never drawn: stops the wizard at water, which is flat. */
	UPROPERTY(VisibleAnywhere, Category = "Level")
	TObjectPtr<UInstancedStaticMeshComponent> WaterBlockers;

	/** The shrine: a low dais and a glowing pillar. Shapes for now. */
	UPROPERTY(VisibleAnywhere, Category = "Crowd")
	TObjectPtr<UStaticMeshComponent> ShrineBase;

	UPROPERTY(VisibleAnywhere, Category = "Crowd")
	TObjectPtr<UStaticMeshComponent> ShrinePillar;

	FVector2D ShrinePosition = FVector2D::ZeroVector;

	TArray<FTransform> SkeletonTransforms;
	TArray<FTransform> GruntTransforms;
	TArray<FTransform> BoltTransforms;
	TArray<FTransform> BoneTransforms;
	TArray<FTransform> BruteTransforms;
	TArray<FTransform> GuardTransforms;
	TArray<FTransform> CommanderTransforms;
	TArray<FTransform> ArcherTransforms;
	TArray<FTransform> AssassinTransforms;
	TArray<FTransform> ArrowTransforms;
	TArray<FTransform> ArmyArrowTransforms;
	TArray<FTransform> JavelinTransforms;
	bool bJavelinModel = false;
	// True where the director's model is used in place of the plain shape: the enemy arrow, the Marksman's arrow, the wizard's spear, the bone pile.
	bool bArrowModel = false;
	bool bArmyArrowModel = false;
	bool bSpearModel = false;
	bool bBoneModel = false;
	TArray<FTransform> SpearTransforms;
	TArray<FTransform> BlastTransforms;
	TArray<FTransform> SlashTransforms;
	TArray<FTransform> BruteBackTransforms;
	TArray<FTransform> SlamRingTransforms;
	TArray<FTransform> SlamFullTransforms;
	TArray<FTransform> ThrowLaneTransforms;
	TArray<FTransform> SweepFanTransforms;
	TArray<FTransform> ThrowLockTransforms;
	TArray<FTransform> SlamFlashTransforms;
	TArray<FTransform> WallTransforms;
	TArray<FVector4> WallBoxes;
	bool bWallsChanged = false;
	TArray<FLevelPiece> Pieces;
	TArray<FCrowdObstacle> BuildingBlockers;
	bool bPiecesChanged = false;
	bool bShrineChanged = false;
	bool bShapesReady = false;
};
