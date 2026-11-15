#include "Game/LevelPieceActor.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Game/LevelActors.h"
#include "Game/LevelPieceLook.h"
#include "Game/WizardShapes.h"
#include "Materials/MaterialInterface.h"

namespace
{
	// Water stops feet, not Bolts: an unseen box this tall over a stretch of river blocks the wizard's body only.
	const double PieceWaterBlockerHeight = 200.0;

	// Each kind has a material of its own in the project: a texture and a tint that can be changed there.
	UMaterialInterface* LoadPieceMaterial(const TCHAR* Name)
	{
		const FString Path = FString::Printf(TEXT("/Game/Level/MI_Piece_%s.MI_Piece_%s"), Name, Name);
		return LoadObject<UMaterialInterface>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
	}

	void Dress(UStaticMeshComponent* Component, const TCHAR* Shape, const TCHAR* MaterialName, const FLinearColor& Fallback)
	{
		Component->SetStaticMesh(WizardShapes::Load(Shape));
		if (UMaterialInterface* Material = LoadPieceMaterial(MaterialName))
		{
			Component->SetMaterial(0, Material);
		}
		else
		{
			WizardShapes::Tint(Component, Fallback);
		}
	}
}

ALevelPieceActor::ALevelPieceActor()
{
	PrimaryActorTick.bCanEverTick = false;
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Static);
	SetRootComponent(Root);

	const auto MakeShape = [this, Root](const TCHAR* Name)
	{
		UStaticMeshComponent* Component = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Component->SetupAttachment(Root);
		Component->SetMobility(EComponentMobility::Static);
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return Component;
	};
	Body = MakeShape(TEXT("Body"));
	Top = MakeShape(TEXT("Top"));

	Blocker = CreateDefaultSubobject<UBoxComponent>(TEXT("Blocker"));
	Blocker->SetupAttachment(Root);
	Blocker->SetMobility(EComponentMobility::Static);
	Blocker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Blocker->SetCollisionObjectType(ECC_WorldStatic);
	Blocker->SetCollisionResponseToAllChannels(ECR_Ignore);
	Blocker->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	Blocker->ShapeColor = FColor(60, 140, 255);
}

void ALevelPieceActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	using namespace LevelPieceLook;
	// Basic shapes are 100 units across with the pivot in the middle.
	const FPieceLook& Look = GetPieceLook(Kind);
	Dress(Body, Look.Shape, FLevelLayout::GetPieceName(Kind), Look.Color);
	Top->SetVisibility(false);
	Top->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	const bool bTall = Look.Height > 0.0;
	// What stands up is solid; what lies on the ground is walked over and casts no shadow.
	Body->SetCollisionProfileName(bTall ? TEXT("BlockAll") : TEXT("NoCollision"));
	Body->SetCastShadow(bTall);
	if (Kind == ELevelPiece::Tree)
	{
		Body->SetRelativeLocation(FVector(0.0, 0.0, Look.Height * 0.5));
		Body->SetRelativeScale3D(FVector(TreeTrunkAcross / 100.0, TreeTrunkAcross / 100.0, Look.Height / 100.0));
		Dress(Top, TEXT("Cone"), TEXT("TreeTop"), TreeTopColor);
		Top->SetRelativeLocation(FVector(0.0, 0.0, Look.Height + TreeTopScale.Z * 50.0 - 20.0));
		Top->SetRelativeScale3D(TreeTopScale);
		Top->SetVisibility(true);
	}
	else if (Kind == ELevelPiece::Lamp)
	{
		Body->SetCollisionProfileName(TEXT("NoCollision"));
		Body->SetRelativeLocation(FVector(0.0, 0.0, Look.Height * 0.5));
		Body->SetRelativeScale3D(FVector(LampPostAcross / 100.0, LampPostAcross / 100.0, Look.Height / 100.0));
		Dress(Top, TEXT("Sphere"), TEXT("LampGlow"), LampGlowColor);
		Top->SetCastShadow(false);
		Top->SetRelativeLocation(FVector(0.0, 0.0, Look.Height + 10.0));
		Top->SetRelativeScale3D(LampGlowScale);
		Top->SetVisibility(true);
	}
	else if (bTall)
	{
		Body->SetRelativeLocation(FVector(0.0, 0.0, Look.Height * 0.5));
		Body->SetRelativeScale3D(FVector(Size.X / 100.0, Size.Y / 100.0, Look.Height / 100.0));
	}
	else
	{
		Body->SetRelativeLocation(FVector(0.0, 0.0, Look.Lift));
		Body->SetRelativeScale3D(FVector(Size.X / 100.0, Size.Y / 100.0, PlateThickness / 100.0));
	}

	const bool bWater = FLevelLayout::IsLow(Kind);
	Blocker->SetCollisionEnabled(bWater ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	Blocker->SetBoxExtent(bWater ? FVector(Size.X * 0.5, Size.Y * 0.5, PieceWaterBlockerHeight * 0.5) : FVector(1.0, 1.0, 1.0));
	Blocker->SetRelativeLocation(FVector(0.0, 0.0, bWater ? PieceWaterBlockerHeight * 0.5 : 0.0));
	Blocker->SetVisibility(bWater);
	if (Kind == ELevelPiece::Bridge)
	{
		// The bridge is a model placed over the piece (the level's "Bridge Model" actors). The piece is the way across.
		Body->SetVisibility(false);
	}
	if (Kind == ELevelPiece::River)
	{
		// The river's water is one smooth mesh along all its pieces (the level's "River Water"). A piece keeps what
		// stops feet, and shows as its blue box in the editor.
		Body->SetVisibility(false);
	}
}

FLevelPiece ALevelPieceActor::MakePiece() const
{
	const FTransform Placed = LevelRead::GetPlacedTransform(*this);
	const FVector Location = Placed.GetLocation();
	const FVector Scale = Placed.GetScale3D().GetAbs();
	FLevelPiece Piece;
	Piece.Kind = Kind;
	Piece.Center = FVector2D(Location.X, Location.Y);
	Piece.HalfSize = FVector2D(Size.X * Scale.X, Size.Y * Scale.Y) * 0.5;
	Piece.Yaw = static_cast<float>(Placed.Rotator().Yaw);
	Piece.HeightScale = static_cast<float>(Scale.Z);
	return Piece;
}

void ALevelPieceActor::SetPiece(const FLevelPiece& Piece)
{
	Kind = Piece.Kind;
	Size = Piece.HalfSize * 2.0;
	const FVector Location = GetActorLocation();
	SetActorLocationAndRotation(FVector(Piece.Center.X, Piece.Center.Y, Location.Z), FRotator(0.0, Piece.Yaw, 0.0));
	SetActorScale3D(FVector(1.0, 1.0, Piece.HeightScale));
}
