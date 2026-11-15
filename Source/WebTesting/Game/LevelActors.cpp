#include "Game/LevelActors.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Game/WizardShapes.h"
#include "Materials/MaterialInterface.h"

namespace
{
	const TCHAR* ShrineRangeMaterialPath = TEXT("/Game/Level/M_ShrineRange.M_ShrineRange");
	const TCHAR* PostAreaMaterialPath = TEXT("/Game/Level/M_PostArea.M_PostArea");
	// The ring and the disc lie this far over the actor, clear of the ground and of roads.
	const double RingLift = 14.0;
	const double LabelLift = 260.0;
	// Room to walk about: so much ground for each of a group, and never less than this.
	const float PostRoomPerUnit = 115.0f;
	const float PostRoomLeast = 300.0f;
}

FTransform LevelRead::GetPlacedTransform(const AActor& Actor)
{
	// The root has no parent, so what the level saved for it is where it stands in the world.
	const USceneComponent* Root = Actor.GetRootComponent();
	return Root ? FTransform(Root->GetRelativeRotation(), Root->GetRelativeLocation(), Root->GetRelativeScale3D()) : FTransform::Identity;
}

AShrineActor::AShrineActor()
{
	PrimaryActorTick.bCanEverTick = false;
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Static);
	SetRootComponent(Root);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetMobility(EComponentMobility::Static);
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));

	RangeRing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Range Ring"));
	RangeRing->SetupAttachment(Root);
	RangeRing->SetMobility(EComponentMobility::Static);
	RangeRing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RangeRing->SetCastShadow(false);
}

void AShrineActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// A plane 100 units across, scaled to the range, with the material that draws the pulsing ring on it.
	RangeRing->SetStaticMesh(WizardShapes::Load(TEXT("Plane")));
	if (UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, ShrineRangeMaterialPath))
	{
		RangeRing->SetMaterial(0, Material);
	}
	const FVector Scale = GetActorScale3D().GetAbs();
	RangeRing->SetRelativeLocation(FVector(0.0, 0.0, RingLift));
	// The range is in the world's units whatever the actor's scale.
	RangeRing->SetRelativeScale3D(FVector(Range * 2.0 / 100.0 / FMath::Max(Scale.X, 0.01), Range * 2.0 / 100.0 / FMath::Max(Scale.Y, 0.01), 1.0));
}

FBox AShrineActor::GetBlockingBounds() const
{
	const UStaticMesh* Asset = Mesh ? Mesh->GetStaticMesh() : nullptr;
	const FTransform Placed = LevelRead::GetPlacedTransform(*this);
	if (!Asset)
	{
		return FBox(Placed.GetLocation() - FVector(100.0), Placed.GetLocation() + FVector(100.0));
	}
	return Asset->GetBoundingBox().TransformBy(Placed);
}

AEnemyPostActor::AEnemyPostActor()
{
	PrimaryActorTick.bCanEverTick = false;
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Static);
	SetRootComponent(Root);

#if WITH_EDITORONLY_DATA
	Area = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Area"));
	Area->SetupAttachment(Root);
	Area->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Area->SetCastShadow(false);
	Area->SetHiddenInGame(true);
	Area->bIsEditorOnly = true;

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(Root);
	Label->SetHiddenInGame(true);
	Label->bIsEditorOnly = true;
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetVerticalAlignment(EVRTA_TextCenter);
	Label->SetWorldSize(90.0f);
	Label->SetTextRenderColor(FColor(255, 120, 90));
#endif
}

float AEnemyPostActor::GetRoomFor(int32 Count)
{
	return FMath::Max(PostRoomLeast, PostRoomPerUnit * FMath::Sqrt(static_cast<float>(FMath::Max(Count, 1))));
}

void AEnemyPostActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
#if WITH_EDITOR
	if (!Area || !Label)
	{
		return;
	}
	Area->SetStaticMesh(WizardShapes::Load(TEXT("Plane")));
	if (UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, PostAreaMaterialPath))
	{
		Area->SetMaterial(0, Material);
	}
	const FVector Scale = GetActorScale3D().GetAbs();
	Area->SetRelativeLocation(FVector(0.0, 0.0, RingLift));
	Area->SetRelativeScale3D(FVector(Radius * 2.0 / 100.0 / FMath::Max(Scale.X, 0.01), Radius * 2.0 / 100.0 / FMath::Max(Scale.Y, 0.01), 1.0));

	// Lying flat, read from above, the way the level is looked at.
	FString Text = bCommander ? TEXT("Commander") : TEXT("Post");
	const auto Add = [&Text](int32 Count, const TCHAR* One, const TCHAR* Many)
	{
		if (Count > 0)
		{
			Text += FString::Printf(TEXT("\n%d %s"), Count, Count == 1 ? One : Many);
		}
	};
	Add(Grunts, TEXT("grunt"), TEXT("grunts"));
	Add(Archers, TEXT("archer"), TEXT("archers"));
	Add(Brutes, TEXT("brute"), TEXT("brutes"));
	if (Radius < GetRoomFor(GetCount()))
	{
		Text += TEXT("\n(tight)");
	}
	Label->SetText(FText::FromString(Text));
	Label->SetRelativeLocation(FVector(0.0, 0.0, LabelLift));
	Label->SetRelativeRotation(FRotator(90.0, 180.0, 0.0));
#endif
}

AKingdomGateActor::AKingdomGateActor()
{
	PrimaryActorTick.bCanEverTick = false;
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Static);
	SetRootComponent(Root);

	Area = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Area"));
	Area->SetupAttachment(Root);
	Area->SetMobility(EComponentMobility::Movable);
	Area->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Area->SetCastShadow(false);
	// Shut when a run starts.
	Area->SetHiddenInGame(true);

#if WITH_EDITORONLY_DATA
	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(Root);
	Label->SetHiddenInGame(true);
	Label->bIsEditorOnly = true;
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetVerticalAlignment(EVRTA_TextCenter);
	Label->SetWorldSize(90.0f);
	Label->SetTextRenderColor(FColor(255, 215, 90));
#endif
}

void AKingdomGateActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// A plane 100 units across, stretched to the gate's ground, with the shrine's pulsing ring on it until it has a look of its own.
	Area->SetStaticMesh(WizardShapes::Load(TEXT("Plane")));
	if (UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, ShrineRangeMaterialPath))
	{
		Area->SetMaterial(0, Material);
	}
	Area->SetRelativeLocation(FVector(0.0, 0.0, RingLift));
	Area->SetRelativeScale3D(FVector(Depth / 100.0, Width / 100.0, 1.0));
#if WITH_EDITOR
	if (Label)
	{
		Label->SetText(FText::FromString(TEXT("Kingdom Gate\nopens when the commander is dead")));
		Label->SetRelativeLocation(FVector(0.0, 0.0, LabelLift));
		Label->SetRelativeRotation(FRotator(90.0, 180.0, 0.0));
	}
#endif
}

FVector2D AKingdomGateActor::GetHalfSize() const
{
	const FVector Scale = LevelRead::GetPlacedTransform(*this).GetScale3D().GetAbs();
	return FVector2D(0.5 * Depth * Scale.X, 0.5 * Width * Scale.Y);
}

void AKingdomGateActor::SetOpenShown(bool bOpen)
{
	Area->SetHiddenInGame(!bOpen);
}

AWallActor::AWallActor()
{
	PrimaryActorTick.bCanEverTick = false;
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Static);
	SetRootComponent(Root);

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(Root);
	Body->SetMobility(EComponentMobility::Static);
	// The layout actor's walls are the solid ones in the game, and the ones drawn there.
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetHiddenInGame(true);
	Body->SetCastShadow(false);
}

void AWallActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// A cube 100 units across, stretched to the wall and stood on the ground.
	Body->SetStaticMesh(WizardShapes::Load(TEXT("Cube")));
	Body->SetRelativeLocation(FVector(0.0, 0.0, Height * 0.5));
	Body->SetRelativeScale3D(FVector(Size.X / 100.0, Size.Y / 100.0, Height / 100.0));
}

FVector2D AWallActor::MakeHalfSize(FVector2D Size, float YawDegrees, FVector2D Scale)
{
	const FVector2D Half(0.5 * Size.X * FMath::Abs(Scale.X), 0.5 * Size.Y * FMath::Abs(Scale.Y));
	const double Cos = FMath::Abs(FMath::Cos(FMath::DegreesToRadians(static_cast<double>(YawDegrees))));
	const double Sin = FMath::Abs(FMath::Sin(FMath::DegreesToRadians(static_cast<double>(YawDegrees))));
	return FVector2D(Half.X * Cos + Half.Y * Sin, Half.X * Sin + Half.Y * Cos);
}

void AWallActor::GetBox(FVector2D& OutCenter, FVector2D& OutHalfSize) const
{
	const FTransform Placed = LevelRead::GetPlacedTransform(*this);
	OutCenter = FVector2D(Placed.GetLocation().X, Placed.GetLocation().Y);
	OutHalfSize = MakeHalfSize(Size, static_cast<float>(Placed.Rotator().Yaw), FVector2D(Placed.GetScale3D().X, Placed.GetScale3D().Y));
}

ABattlefieldActor::ABattlefieldActor()
{
	PrimaryActorTick.bCanEverTick = false;
	Edge = CreateDefaultSubobject<UBoxComponent>(TEXT("Edge"));
	Edge->SetMobility(EComponentMobility::Static);
	// Nothing is stopped by it: it is only where the warning starts.
	Edge->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Edge->SetHiddenInGame(true);
	Edge->ShapeColor = FColor(255, 150, 40);
	Edge->SetLineThickness(12.0f);
	SetRootComponent(Edge);
}

void ABattlefieldActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Edge->SetBoxExtent(FVector(Size.X * 0.5, Size.Y * 0.5, 150.0));
}

void ABattlefieldActor::GetBox(FVector2D& OutCenter, FVector2D& OutHalfSize) const
{
	const FTransform Placed = LevelRead::GetPlacedTransform(*this);
	OutCenter = FVector2D(Placed.GetLocation().X, Placed.GetLocation().Y);
	OutHalfSize = AWallActor::MakeHalfSize(Size, static_cast<float>(Placed.Rotator().Yaw), FVector2D(Placed.GetScale3D().X, Placed.GetScale3D().Y));
}

APatrolRouteActor::APatrolRouteActor()
{
	PrimaryActorTick.bCanEverTick = false;
	Road = CreateDefaultSubobject<USplineComponent>(TEXT("Road"));
	Road->SetMobility(EComponentMobility::Static);
	Road->SetClosedLoop(false);
#if WITH_EDITORONLY_DATA
	Road->EditorUnselectedSplineSegmentColor = FLinearColor(1.0f, 0.35f, 0.1f);
	Road->EditorSelectedSplineSegmentColor = FLinearColor(1.0f, 0.9f, 0.2f);
	Road->bShouldVisualizeScale = false;
#endif
	SetRootComponent(Road);
}

TArray<FVector2D> APatrolRouteActor::GetPoints() const
{
	// Local points through the saved transform: right before the level is in a running world as well.
	const FTransform Placed = LevelRead::GetPlacedTransform(*this);
	TArray<FVector2D> Points;
	const int32 Count = Road ? Road->GetNumberOfSplinePoints() : 0;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const FVector World = Placed.TransformPosition(Road->GetLocationAtSplinePoint(Index, ESplineCoordinateSpace::Local));
		Points.Add(FVector2D(World.X, World.Y));
	}
	return Points;
}

void APatrolRouteActor::SetPoints(const TArray<FVector2D>& Points)
{
	if (Points.Num() == 0)
	{
		return;
	}
	const double Height = GetActorLocation().Z;
	SetActorLocation(FVector(Points[0].X, Points[0].Y, Height));
	TArray<FVector> Local;
	for (const FVector2D& Point : Points)
	{
		Local.Add(FVector(Point.X - Points[0].X, Point.Y - Points[0].Y, 0.0));
	}
	Road->SetSplinePoints(Local, ESplineCoordinateSpace::Local, false);
	// Straight from point to point: a patrol walks lines, not curves.
	for (int32 Index = 0; Index < Local.Num(); ++Index)
	{
		Road->SetSplinePointType(Index, ESplinePointType::Linear, false);
	}
	Road->UpdateSpline();
}
