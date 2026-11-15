#include "Game/WizardPawn.h"
#include "Game/WizardAudioSubsystem.h"
#include "Engine/GameInstance.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Game/CrowdRenderer.h"
#include "Game/WizardAnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Game/CrowdSubsystem.h"
#include "Engine/GameInstance.h"
#include "Game/WizardHUD.h"
#include "Game/WizardSettingsSubsystem.h"
#include "Game/WizardPlayerController.h"
#include "Game/WizardShapes.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "UObject/UnrealType.h"

namespace
{
	const float CapsuleRadius = 40.0f;
	const float CapsuleHalfHeight = 92.0f;
	const float FloorClearance = 2.0f;
	const FLinearColor WizardColor(0.35f, 0.08f, 0.65f);
	const FLinearColor SkullColor(0.9f, 0.88f, 0.8f);

	// The Wizard King (Tripo, rigged to the UE5 mannequin skeleton) and the mannequin's default animation blueprint.
	const TCHAR* ModelMeshPath = TEXT("/Game/Wizard/SKM_Wizard_King.SKM_Wizard_King");
	const TCHAR* ModelAnimPath = TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C");
	// The director's casting animation, with a "Fire" notify where the Bolt should leave.
	const TCHAR* BoltAnimPath = TEXT("/Game/Animations/Wizard/Anim_Wizard_Bolt.Anim_Wizard_Bolt");

	// Kept across restarts of the run, like the map.
	bool bStartModelShown = true;
	float StartModelHeight = AWizardPawn::ModelHeightDefault;
}

// The start menu is shown once for each time the game is opened: a restart loads the level again, but not the menu.
static TWeakObjectPtr<const UGameInstance> WizardStartMenuShownFor;

AWizardPawn::AWizardPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	UCapsuleComponent* Capsule = GetCapsuleComponent();
	Capsule->InitCapsuleSize(CapsuleRadius, CapsuleHalfHeight);
	USkeletalMeshComponent* Model = GetMesh();

	Visual = CreateDefaultSubobject<USceneComponent>(TEXT("Visual"));
	Visual->SetupAttachment(Capsule);

	// Basic shapes are 100 units across with the pivot in the middle.
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(Visual);
	Body->SetRelativeScale3D(FVector(0.6, 0.6, 1.2));
	Body->SetRelativeLocation(FVector(0.0, 0.0, 60.0 - CapsuleHalfHeight));
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Head = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Head"));
	Head->SetupAttachment(Visual);
	Head->SetRelativeScale3D(FVector(0.5));
	Head->SetRelativeLocation(FVector(0.0, 0.0, 145.0 - CapsuleHalfHeight));
	Head->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// A mannequin mesh faces +Y with its feet at the origin; the wizard faces +X and his middle is the capsule's.
	Model->SetupAttachment(Visual);
	Model->SetRelativeLocation(FVector(0.0, 0.0, -CapsuleHalfHeight));
	Model->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	Model->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Model->SetVisibility(false);

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Spring Arm"));
	SpringArm->SetupAttachment(Capsule);
	SpringArm->SetUsingAbsoluteRotation(true);
	SpringArm->bDoCollisionTest = false;
	SpringArm->TargetArmLength = CameraArmLength;
	SpringArm->SetWorldRotation(FRotator(CameraPitch, CameraYaw, 0.0f));

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->MaxWalkSpeed = WalkSpeed;
	Movement->MaxAcceleration = 4000.0f;
	Movement->BrakingDecelerationWalking = 8000.0f;
	// He faces the aim, not the way he walks; the animation blueprint then picks sideways and backward steps.
	Movement->bOrientRotationToMovement = false;
	// Nothing in a level is meant to be climbed: low walls and the water's edge stop him, as before.
	Movement->MaxStepHeight = 0.0f;

	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	bUseControllerRotationYaw = false;
}

FWizardKeyMap& WizardKeys()
{
	static FWizardKeyMap Map;
	return Map;
}

TArray<FWizardKeyBinding> AWizardPawn::GetKeyBindings()
{
	return WizardKeys().All();
}

FKey AWizardPawn::GetKey(FName Action)
{
	return WizardKeys().Get(Action);
}

void AWizardPawn::GetCameraTarget(EWizardCameraMode Mode, float& OutPitch, float& OutArmLength)
{
	switch (Mode)
	{
	case EWizardCameraMode::TopDown:
		OutPitch = -89.0f;
		OutArmLength = 2000.0f;
		break;
	case EWizardCameraMode::ThirdPerson:
		OutPitch = -15.0f;
		OutArmLength = 500.0f;
		break;
	case EWizardCameraMode::Angled:
	default:
		OutPitch = -55.0f;
		OutArmLength = 1600.0f;
		break;
	}
}

FString AWizardPawn::GetCameraModeName(EWizardCameraMode Mode)
{
	switch (Mode)
	{
	case EWizardCameraMode::TopDown:
		return TEXT("Top Down");
	case EWizardCameraMode::ThirdPerson:
		return TEXT("Third Person");
	case EWizardCameraMode::Angled:
	default:
		return TEXT("Angled");
	}
}

bool AWizardPawn::ParseCameraMode(const FString& Text, EWizardCameraMode& OutMode)
{
	if (Text.Equals(TEXT("Angled"), ESearchCase::IgnoreCase))
	{
		OutMode = EWizardCameraMode::Angled;
		return true;
	}
	if (Text.Equals(TEXT("TopDown"), ESearchCase::IgnoreCase))
	{
		OutMode = EWizardCameraMode::TopDown;
		return true;
	}
	if (Text.Equals(TEXT("ThirdPerson"), ESearchCase::IgnoreCase))
	{
		OutMode = EWizardCameraMode::ThirdPerson;
		return true;
	}
	return false;
}

void AWizardPawn::SetCameraMode(EWizardCameraMode Mode)
{
	if (Mode == CameraMode)
	{
		return;
	}
	CameraMode = Mode;
	BlendFromPitch = CameraPitch;
	BlendFromArmLength = CameraArmLength;
	BlendElapsed = 0.0f;
}

float AWizardPawn::GetModelScale(float Height, float AssetHeight)
{
	if (AssetHeight <= UE_KINDA_SMALL_NUMBER)
	{
		return 1.0f;
	}
	return FMath::Clamp(Height, ModelHeightMin, ModelHeightMax) / AssetHeight;
}

void AWizardPawn::SetModelShown(bool bShown)
{
	bStartModelShown = bShown;
	ApplyModel();
}

bool AWizardPawn::IsModelShown() const
{
	return bModelLoaded && bStartModelShown;
}

FString AWizardPawn::GetModelName() const
{
	const USkeletalMesh* Asset = IsModelShown() ? GetMesh()->GetSkeletalMeshAsset() : nullptr;
	return Asset ? Asset->GetName() : TEXT("shapes");
}

bool AWizardPawn::IsModelMoving() const
{
	const UAnimInstance* Anim = IsModelShown() ? GetMesh()->GetAnimInstance() : nullptr;
	return Anim && AnimShouldMove && AnimShouldMove->GetPropertyValue_InContainer(Anim);
}

void AWizardPawn::SetModelHeight(float Height)
{
	StartModelHeight = FMath::Clamp(Height, ModelHeightMin, ModelHeightMax);
	ApplyModel();
}

float AWizardPawn::GetModelHeight() const
{
	return StartModelHeight;
}

void AWizardPawn::LoadModel()
{
	// The wizard is always on screen, so his model is loaded with him.
	USkeletalMesh* Asset = LoadObject<USkeletalMesh>(nullptr, ModelMeshPath);
	if (!Asset)
	{
		return;
	}
	GetMesh()->SetSkeletalMeshAsset(Asset);
	ModelAssetHeight = Asset->GetBounds().BoxExtent.Z * 2.0f;
	bModelLoaded = true;

	if (UClass* AnimClass = LoadClass<UAnimInstance>(nullptr, ModelAnimPath))
	{
		GetMesh()->SetAnimInstanceClass(AnimClass);
		AnimShouldMove = CastField<FBoolProperty>(AnimClass->FindPropertyByName(TEXT("ShouldMove")));
	}

	BoltAnimation = LoadObject<UAnimMontage>(nullptr, BoltAnimPath);
	BoltAnimLength = BoltAnimation ? BoltAnimation->GetPlayLength() : 0.0f;
	// With no "Fire" notify the animation is not used, and the Bolt leaves at once.
	BoltFireTime = ACrowdRenderer::GetFireShare(BoltAnimation, 0.0f) * BoltAnimLength;
	// Overwhelmed: the director's pick (2026-11-14 22:30 PST), or a montage of it when one is made.
	FallAnimation = ACrowdRenderer::LoadAttack(TEXT("/Game/Paragon_Picks/02_Player_Fall_Overwhelmed/A_Khaimera_Stunned_Start.A_Khaimera_Stunned_Start"));
}

EWizardMusic AWizardPawn::GetWantedMusic() const
{
	return FWizardSoundRules::MusicFor(Menu.IsOpen() || (SettingsScreen.IsOpen() && SettingsScreen.IsFromStart()), Crowd && Crowd->IsRunWon());
}

void AWizardPawn::ApplyModel()
{
	const bool bShown = IsModelShown();
	GetMesh()->SetVisibility(bShown && !bBlinkHidden);
	Body->SetVisibility(!bShown && !bBlinkHidden);
	Head->SetVisibility(!bShown && !bBlinkHidden);
	GetMesh()->SetRelativeScale3D(FVector(GetModelScale(StartModelHeight, ModelAssetHeight)));
}

void AWizardPawn::SetBlinkHidden(bool bGone)
{
	if (bBlinkHidden != bGone)
	{
		bBlinkHidden = bGone;
		ApplyModel();
	}
}

FVector2D AWizardPawn::GetPosition2D() const
{
	const FVector Location = GetActorLocation();
	return FVector2D(Location.X, Location.Y);
}

void AWizardPawn::BeginPlay()
{
	Super::BeginPlay();

	// The sound channels' volumes belong to the world that is running: put them in force for this one.
	if (UWizardSettingsSubsystem* Store = GetGameInstance() ? GetGameInstance()->GetSubsystem<UWizardSettingsSubsystem>() : nullptr)
	{
		Store->ApplySound();
	}

	Body->SetStaticMesh(WizardShapes::Load(TEXT("Cylinder")));
	Head->SetStaticMesh(WizardShapes::Load(TEXT("Sphere")));
	WizardShapes::Tint(Body, WizardColor);
	WizardShapes::Tint(Head, SkullColor);
	LoadModel();
	ApplyModel();

	// Stand on whatever ground is below the start point.
	const FVector Start = GetActorLocation();
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(WizardGround), false, this);
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, Start - FVector(0.0, 0.0, 5000.0), ECC_WorldStatic, Params))
	{
		GroundZ = Hit.Location.Z;
		// He starts just above the level's ground plate (2 units over the floor) and settles onto it.
		SetActorLocation(FVector(Start.X, Start.Y, GroundZ + CapsuleHalfHeight + FloorClearance));
	}
	else
	{
		GroundZ = Start.Z - CapsuleHalfHeight;
	}

	Crowd = GetWorld()->GetSubsystem<UCrowdSubsystem>();
	if (Crowd)
	{
		Crowd->RegisterWizard(this);
	}

	// Not in automated runs (they start playing at once), and not after a restart.
	bStartMenuPending = !FApp::IsUnattended() && WizardStartMenuShownFor.Get() != GetGameInstance();
}

void AWizardPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Input is made in code so the game needs no input assets. Every key comes from GetKeyBindings.
	MoveAction = NewObject<UInputAction>(this, TEXT("Move"));
	MoveAction->ValueType = EInputActionValueType::Axis2D;

	MappingContext = NewObject<UInputMappingContext>(this, TEXT("Wizard Input"));
	// The menus are clicked with the left mouse button whatever key Bolt is on; it is not one of the game's controls.
	MenuClickAction = NewObject<UInputAction>(this, TEXT("Menu Click"));
	MenuClickAction->ValueType = EInputActionValueType::Boolean;
	MenuClickAction->bTriggerWhenPaused = true;

	KeyActions.Reset();
	const auto MakeAction = [this](const TCHAR* Name, bool bWhenPaused = false)
	{
		UInputAction* Action = NewObject<UInputAction>(this, Name);
		Action->ValueType = EInputActionValueType::Boolean;
		Action->bTriggerWhenPaused = bWhenPaused;
		KeyActions.Add(Action);
		return Action;
	};
	UInputAction* BoltAction = MakeAction(TEXT("Bolt"));
	UInputAction* SpearAction = MakeAction(TEXT("Spear"));
	UInputAction* SummonAction = MakeAction(TEXT("Summon"));
	UInputAction* BlinkAction = MakeAction(TEXT("Blink"));
	UInputAction* CommandAction = MakeAction(TEXT("Command"));
	UInputAction* RecallAction = MakeAction(TEXT("Recall"));
	// The wheel gives a number, not a press.
	UInputAction* CommandCountAction = MakeAction(TEXT("Command Count"));
	CommandCountAction->ValueType = EInputActionValueType::Axis1D;
	// Menu keys also work while the game is paused.
	UInputAction* InteractAction = MakeAction(TEXT("Shrine"), true);
	UInputAction* PauseAction = MakeAction(TEXT("Pause"), true);
	UInputAction* RestartAction = MakeAction(TEXT("Restart"), true);
	UInputAction* MenuStartAction = MakeAction(TEXT("Menu Start"), true);
	// Test keys, in every build: a browser build has no console to reach.
	UInputAction* AngledAction = MakeAction(TEXT("Camera Angled"));
	UInputAction* TopDownAction = MakeAction(TEXT("Camera Top Down"));
	UInputAction* ThirdPersonAction = MakeAction(TEXT("Camera Third Person"));
	UInputAction* ToggleDebugAction = MakeAction(TEXT("Debug Overlay"));
	UInputAction* TestModelsAction = MakeAction(TEXT("Test Models"));
	UInputAction* TestEnemiesAction = MakeAction(TEXT("Test Enemies"));
	UInputAction* TestArmyAction = MakeAction(TEXT("Test Army"));
	UInputAction* TestSwitchAction = MakeAction(TEXT("Test Switch"));

	if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AWizardPawn::OnMove);
		Input->BindAction(MoveAction, ETriggerEvent::Completed, this, &AWizardPawn::OnMoveStopped);
		Input->BindAction(MoveAction, ETriggerEvent::Canceled, this, &AWizardPawn::OnMoveStopped);
		Input->BindAction(CommandAction, ETriggerEvent::Started, this, &AWizardPawn::OnCommand);
		Input->BindAction(RecallAction, ETriggerEvent::Started, this, &AWizardPawn::OnRecall);
		Input->BindAction(CommandCountAction, ETriggerEvent::Triggered, this, &AWizardPawn::OnCommandCount);
		Input->BindAction(BoltAction, ETriggerEvent::Started, this, &AWizardPawn::OnBoltStarted);
		Input->BindAction(BoltAction, ETriggerEvent::Completed, this, &AWizardPawn::OnBoltCompleted);
		Input->BindAction(BoltAction, ETriggerEvent::Canceled, this, &AWizardPawn::OnBoltCompleted);
		Input->BindAction(MenuClickAction, ETriggerEvent::Started, this, &AWizardPawn::OnMenuClick);
		Input->BindAction(MenuClickAction, ETriggerEvent::Completed, this, &AWizardPawn::OnMenuClickEnded);
		Input->BindAction(MenuClickAction, ETriggerEvent::Canceled, this, &AWizardPawn::OnMenuClickEnded);
		Input->BindAction(SpearAction, ETriggerEvent::Started, this, &AWizardPawn::OnSpear);
		Input->BindAction(SummonAction, ETriggerEvent::Started, this, &AWizardPawn::OnSummon);
		Input->BindAction(BlinkAction, ETriggerEvent::Started, this, &AWizardPawn::OnBlink);
		Input->BindAction(InteractAction, ETriggerEvent::Started, this, &AWizardPawn::OnInteract);
		Input->BindAction(PauseAction, ETriggerEvent::Started, this, &AWizardPawn::OnPause);
		Input->BindAction(RestartAction, ETriggerEvent::Started, this, &AWizardPawn::OnRestart);
		Input->BindAction(MenuStartAction, ETriggerEvent::Started, this, &AWizardPawn::OnMenuStart);
		Input->BindAction(AngledAction, ETriggerEvent::Started, this, &AWizardPawn::OnCameraAngled);
		Input->BindAction(TopDownAction, ETriggerEvent::Started, this, &AWizardPawn::OnCameraTopDown);
		Input->BindAction(ThirdPersonAction, ETriggerEvent::Started, this, &AWizardPawn::OnCameraThirdPerson);
		Input->BindAction(ToggleDebugAction, ETriggerEvent::Started, this, &AWizardPawn::OnToggleDebug);
		Input->BindAction(TestModelsAction, ETriggerEvent::Started, this, &AWizardPawn::OnTestModels);
		Input->BindAction(TestEnemiesAction, ETriggerEvent::Started, this, &AWizardPawn::OnTestEnemies);
		Input->BindAction(TestArmyAction, ETriggerEvent::Started, this, &AWizardPawn::OnTestArmy);
		Input->BindAction(TestSwitchAction, ETriggerEvent::Started, this, &AWizardPawn::OnTestSwitch);
	}

	MapKeys();
	if (const APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (const ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				InputSubsystem->AddMappingContext(MappingContext, 0);
			}
		}
	}
}

void AWizardPawn::MapKeys()
{
	if (!MappingContext || !MoveAction)
	{
		return;
	}
	MappingContext->UnmapAll();
	// A key gives +X. Right is +X, left is -X, forward is +Y, back is -Y.
	MappingContext->MapKey(MoveAction, GetKey(TEXT("Move Right")));
	MappingContext->MapKey(MoveAction, GetKey(TEXT("Move Left"))).Modifiers.Add(NewObject<UInputModifierNegate>(MappingContext));
	MappingContext->MapKey(MoveAction, GetKey(TEXT("Move Forward"))).Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(MappingContext));
	FEnhancedActionKeyMapping& Back = MappingContext->MapKey(MoveAction, GetKey(TEXT("Move Back")));
	Back.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(MappingContext));
	Back.Modifiers.Add(NewObject<UInputModifierNegate>(MappingContext));
	for (UInputAction* Action : KeyActions)
	{
		if (Action)
		{
			MappingContext->MapKey(Action, GetKey(Action->GetFName()));
		}
	}
	if (MenuClickAction)
	{
		MappingContext->MapKey(MenuClickAction, EKeys::LeftMouseButton);
	}
	// When keys change while the game runs, the player's input takes up the new ones. A key that is down at that
	// moment (the one just pressed to choose it) does nothing until it is let go.
	if (const APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (const ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				InputSubsystem->RequestRebuildControlMappings();
			}
		}
	}
}

EWizardRebind AWizardPawn::RebindKey(FName Action, const FKey& Key)
{
	FWizardKeyMap& Keys = WizardKeys();
	const FKey Old = Keys.Get(Action);
	FName Other;
	const EWizardRebind Result = Keys.Rebind(Action, Key, &Other);
	const FString Name = FWizardKeyMap::ActionLabel(Action);
	const FString KeyName = FWizardKeyMap::KeyLabel(Key);
	switch (Result)
	{
	case EWizardRebind::Set:
		KeyMessage = FString::Printf(TEXT("%s is now on %s."), *Name, *KeyName);
		break;
	case EWizardRebind::Swapped:
		KeyMessage = FString::Printf(TEXT("%s was on %s. %s moved to %s."), *KeyName, *FWizardKeyMap::ActionLabel(Other), *FWizardKeyMap::ActionLabel(Other), *FWizardKeyMap::KeyLabel(Old));
		break;
	case EWizardRebind::Same:
		KeyMessage = FString::Printf(TEXT("%s is already on %s."), *Name, *KeyName);
		break;
	default:
		KeyMessage = Other.IsNone() ? FString::Printf(TEXT("%s cannot be used."), *KeyName) : FString::Printf(TEXT("%s is kept for %s."), *KeyName, *FWizardKeyMap::ActionLabel(Other));
		break;
	}
	if (Result == EWizardRebind::Set || Result == EWizardRebind::Swapped)
	{
		MapKeys();
		if (const UWizardSettingsSubsystem* Store = GetGameInstance() ? GetGameInstance()->GetSubsystem<UWizardSettingsSubsystem>() : nullptr)
		{
			Store->SaveNow();
		}
	}
	if (Result == EWizardRebind::Refused)
	{
		if (UWizardAudioSubsystem* Audio = GetGameInstance() ? GetGameInstance()->GetSubsystem<UWizardAudioSubsystem>() : nullptr)
		{
			Audio->PlayCue(EWizardCue::Denied);
		}
	}
	return Result;
}

void AWizardPawn::ResetKeys()
{
	CapturingAction = NAME_None;
	if (WizardKeys().IsDefault())
	{
		return;
	}
	WizardKeys().Reset();
	KeyMessage = TEXT("Every key is back on its default.");
	MapKeys();
	if (const UWizardSettingsSubsystem* Store = GetGameInstance() ? GetGameInstance()->GetSubsystem<UWizardSettingsSubsystem>() : nullptr)
	{
		Store->SaveNow();
	}
}

namespace
{
	/** Every key and mouse button a control may be put on, and Escape, which cancels. */
	const TArray<FKey>& WizardCaptureKeys()
	{
		static TArray<FKey> Keys;
		if (Keys.IsEmpty())
		{
			TArray<FKey> All;
			EKeys::GetAllKeys(All);
			for (const FKey& Key : All)
			{
				if (FWizardKeyMap::IsAllowed(Key) || Key == EKeys::Escape)
				{
					Keys.Add(Key);
				}
			}
		}
		return Keys;
	}
}

void AWizardPawn::BeginKeyCapture(FName Action)
{
	CapturingAction = NAME_None;
	CaptureHeld.Reset();
	if (Action.IsNone() || !FWizardKeyMap::CanChange(Action))
	{
		return;
	}
	CapturingAction = Action;
	KeyMessage = FString::Printf(TEXT("Press the new key or mouse button for %s. Esc cancels."), *FWizardKeyMap::ActionLabel(Action));
	// What is down now (the button that clicked the key, at least) has to be let go before it can be the new key.
	if (const APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		for (const FKey& Key : WizardCaptureKeys())
		{
			if (PlayerController->IsInputKeyDown(Key))
			{
				CaptureHeld.Add(Key);
			}
		}
	}
}

void AWizardPawn::UpdateKeyCapture()
{
	if (CapturingAction.IsNone())
	{
		return;
	}
	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return;
	}
	for (const FKey& Key : WizardCaptureKeys())
	{
		if (!PlayerController->IsInputKeyDown(Key))
		{
			CaptureHeld.Remove(Key);
			continue;
		}
		if (CaptureHeld.Contains(Key))
		{
			continue;
		}
		const FName Action = CapturingAction;
		CapturingAction = NAME_None;
		CaptureHeld.Reset();
		if (Key == EKeys::Escape)
		{
			KeyMessage = TEXT("Cancelled: nothing changed.");
		}
		else
		{
			RebindKey(Action, Key);
		}
		// The press that chose the key is not also a click or a key of the menu.
		MenuClickFrame = GFrameCounter;
		return;
	}
}

void AWizardPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bStartMenuPending)
	{
		bStartMenuPending = false;
		ShowStartMenu(true);
	}
	UpdateAim(DeltaSeconds);
	UpdateCamera(DeltaSeconds);
	UpdateBolt(DeltaSeconds);
	UpdateFall();
	if (APlayerController* Player = Cast<APlayerController>(GetController()))
	{
		// Sounds are heard from where he stands, left and right as the screen has them: not from the camera high above.
		Player->SetAudioListenerOverride(nullptr, GetActorLocation(), FRotator(0.0f, CameraYaw, 0.0f));
	}
	if (UWizardAudioSubsystem* Audio = GetGameInstance() ? GetGameInstance()->GetSubsystem<UWizardAudioSubsystem>() : nullptr)
	{
		Audio->SetMusic(GetWantedMusic());
	}
}

void AWizardPawn::OnMove(const FInputActionValue& Value)
{
	if (IsStopped())
	{
		MoveDirection = FVector2D::ZeroVector;
		return;
	}
	// Movement follows the camera: forward is up the screen.
	const FVector2D Axis = Value.Get<FVector2D>();
	const FRotationMatrix Facing(FRotator(0.0f, CameraYaw, 0.0f));
	AddMovementInput(Facing.GetUnitAxis(EAxis::X), Axis.Y);
	AddMovementInput(Facing.GetUnitAxis(EAxis::Y), Axis.X);
	// Kept for Blink, which goes the way he is being moved.
	const FVector Held = Facing.GetUnitAxis(EAxis::X) * Axis.Y + Facing.GetUnitAxis(EAxis::Y) * Axis.X;
	MoveDirection = FVector2D(Held.X, Held.Y).GetSafeNormal();
}

void AWizardPawn::OnMoveStopped(const FInputActionValue& Value)
{
	MoveDirection = FVector2D::ZeroVector;
}

void AWizardPawn::OnCommand(const FInputActionValue& Value)
{
	if (Crowd && !IsStopped())
	{
		Crowd->ToggleCommandMode();
	}
}

void AWizardPawn::OnRecall(const FInputActionValue& Value)
{
	if (Crowd && !Menu.IsOpen() && !IsStopped() && !UGameplayStatics::IsGamePaused(this))
	{
		Crowd->Recall();
	}
}

void AWizardPawn::OnCommandCount(const FInputActionValue& Value)
{
	// One more or one fewer for each notch of the wheel.
	const float Turn = Value.Get<float>();
	if (Crowd && Crowd->IsCommandMode() && !FMath::IsNearlyZero(Turn))
	{
		Crowd->ScrollCommand(FMath::Abs(Turn) < 1.0f ? (Turn > 0.0f ? 1 : -1) : FMath::RoundToInt32(Turn));
	}
}

bool AWizardPawn::GetMouse(FVector2D& OutPoint, float& OutWidth, float& OutHeight) const
{
	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	float MouseX = 0.0f;
	float MouseY = 0.0f;
	if (!PlayerController || !PlayerController->GetMousePosition(MouseX, MouseY))
	{
		return false;
	}
	int32 ViewWidth = 0;
	int32 ViewHeight = 0;
	PlayerController->GetViewportSize(ViewWidth, ViewHeight);
	OutPoint = FVector2D(MouseX, MouseY);
	OutWidth = static_cast<float>(ViewWidth);
	OutHeight = static_cast<float>(ViewHeight);
	return true;
}

void AWizardPawn::ShowStartMenu(bool bShow)
{
	if (bShow)
	{
		WizardStartMenuShownFor = GetGameInstance();
		Menu.Show();
		SetPaused(true);
	}
	else if (Menu.March())
	{
		SetPaused(false);
	}
}

void AWizardPawn::OnMenuStart(const FInputActionValue& Value)
{
	if (SettingsScreen.IsOpen())
	{
		return;
	}
	ShowStartMenu(false);
}

void AWizardPawn::OnMenuClick(const FInputActionValue& Value)
{
	// The left mouse button, on whatever menu is up. Nothing when no menu is: Bolt's own key fires Bolt.
	const bool bPaused = UGameplayStatics::IsGamePaused(this);
	const bool bShrine = Crowd && !Crowd->IsCommandMode() && Crowd->IsShrineOpen();
	if (!CapturingAction.IsNone() || MenuClickFrame == GFrameCounter || !(Menu.IsOpen() || bPaused || bShrine || (Crowd && Crowd->IsRunWon())))
	{
		return;
	}
	MenuClickFrame = GFrameCounter;
	if (Crowd && Crowd->IsRunWon() && !Menu.IsOpen() && !UGameplayStatics::IsGamePaused(this))
	{
		// The final cut screen: a click on "March again" restarts the run. The locked button does nothing.
		FVector2D Point;
		float ViewWidth = 0.0f;
		float ViewHeight = 0.0f;
		if (GetMouse(Point, ViewWidth, ViewHeight) && AWizardHUD::FinalCutButtonAt(Point, ViewWidth, ViewHeight) == 0)
		{
			Crowd->RequestRestart();
		}
		return;
	}
	if (Menu.IsOpen() || UGameplayStatics::IsGamePaused(this))
	{
		// A menu is up: the click is for its buttons, and no Bolt is fired.
		FVector2D Point;
		float ViewWidth = 0.0f;
		float ViewHeight = 0.0f;
		if (!GetMouse(Point, ViewWidth, ViewHeight))
		{
			return;
		}
		if (SettingsScreen.IsOpen())
		{
			// The settings screen is over the menu: the click is for it alone.
			ClickSettings(Point, ViewWidth, ViewHeight);
		}
		else if (Menu.IsOpen())
		{
			const int32 Button = AWizardHUD::StartMenuButtonAt(Point, ViewWidth, ViewHeight);
			if (Button == 0)
			{
				ShowStartMenu(false);
			}
			else if (Button == 1)
			{
				ShowSettings(true);
			}
		}
		else
		{
			const int32 Button = AWizardHUD::PauseMenuButtonAt(Point, ViewWidth, ViewHeight);
			if (Button == 0)
			{
				SetPaused(false);
			}
			else if (Button == 1)
			{
				ShowSettings(true);
			}
			else if (Button == 2 && Crowd)
			{
				SetPaused(false);
				Crowd->RequestRestart();
			}
		}
		return;
	}
	if (Crowd && Crowd->IsShrineOpen())
	{
		// The shrine's menu is open: a click on a button buys, a click outside its box closes it, and no Bolt is fired either way.
		const APlayerController* PlayerController = Cast<APlayerController>(GetController());
		float MouseX = 0.0f;
		float MouseY = 0.0f;
		int32 ViewWidth = 0;
		int32 ViewHeight = 0;
		if (PlayerController && PlayerController->GetMousePosition(MouseX, MouseY))
		{
			PlayerController->GetViewportSize(ViewWidth, ViewHeight);
			int32 Row = 0;
			int32 Button = 0;
			if (!AWizardHUD::IsInShrineMenu(FVector2D(MouseX, MouseY), ViewWidth, ViewHeight))
			{
				Crowd->CloseShrine();
			}
			else if (AWizardHUD::ShrineMenuButtonAt(FVector2D(MouseX, MouseY), ViewWidth, ViewHeight, Row, Button))
			{
				Crowd->Buy(static_cast<ECrowdShopItem>(Row), AWizardHUD::ShrineMenuButtonAmount(Row, Button));
			}
		}
		return;
	}
}

void AWizardPawn::OnMenuClickEnded(const FInputActionValue& Value)
{
	EndSettingsDrag();
}

void AWizardPawn::OnBoltStarted(const FInputActionValue& Value)
{
	// A click a menu took this frame is not also a Bolt; and no Bolt while a menu is up.
	if (MenuClickFrame == GFrameCounter || Menu.IsOpen() || UGameplayStatics::IsGamePaused(this) || (Crowd && Crowd->IsRunWon()))
	{
		return;
	}
	if (Crowd && Crowd->IsCommandMode())
	{
		// Command mode: the left button sends the chosen number to the point under the mouse. No Bolt is fired.
		Crowd->ConfirmCommand(AimPoint2D);
		return;
	}
	if (Crowd && Crowd->IsShrineOpen())
	{
		return;
	}
	bBoltHeld = true;
}

void AWizardPawn::OnInteract(const FInputActionValue& Value)
{
	if (Crowd && !IsStopped() && !UGameplayStatics::IsGamePaused(this))
	{
		Crowd->ToggleShrine();
	}
}

void AWizardPawn::OnPause(const FInputActionValue& Value)
{
	// A key that was just chosen on the Controls tab, or is being chosen, is not also the pause key.
	if (!CapturingAction.IsNone() || MenuClickFrame == GFrameCounter)
	{
		return;
	}
	// With the settings up, the key is "Back": to the menu they were opened from.
	if (SettingsScreen.IsOpen())
	{
		ShowSettings(false);
		return;
	}
	// Not from the start menu: only March leaves it.
	if (Menu.IsOpen())
	{
		return;
	}
	TogglePause();
}

void AWizardPawn::OnRestart(const FInputActionValue& Value)
{
	// From the pause menu, and from the final cut screen.
	if (Crowd && !Menu.IsOpen() && !SettingsScreen.IsOpen() && CapturingAction.IsNone() && (UGameplayStatics::IsGamePaused(this) || Crowd->IsRunWon()))
	{
		SetPaused(false);
		Crowd->RequestRestart();
	}
}

void AWizardPawn::TogglePause()
{
	SetPaused(!UGameplayStatics::IsGamePaused(this));
}

void AWizardPawn::SetPaused(bool bPaused)
{
	if (bPaused)
	{
		// A button released during the pause is never reported, so nothing may stay held across it.
		bBoltHeld = false;
	}
	else
	{
		// The settings are only ever up over a menu.
		EndSettingsDrag();
		CapturingAction = NAME_None;
		SettingsScreen.Close();
	}
	UGameplayStatics::SetGamePaused(this, bPaused);
}

void AWizardPawn::ShowSettings(bool bShow)
{
	if (!bShow)
	{
		EndSettingsDrag();
		CapturingAction = NAME_None;
		SettingsScreen.Close();
		return;
	}
	if (Menu.IsOpen())
	{
		SettingsScreen.OpenFromStart();
		return;
	}
	if (!UGameplayStatics::IsGamePaused(this))
	{
		SetPaused(true);
	}
	SettingsScreen.OpenFromPause();
}

void AWizardPawn::ClickSettings(FVector2D Point, float ViewWidth, float ViewHeight)
{
	UWizardSettingsSubsystem* Store = GetGameInstance() ? GetGameInstance()->GetSubsystem<UWizardSettingsSubsystem>() : nullptr;
	const FWizardSettingsHit Hit = AWizardHUD::SettingsHitAt(Point, SettingsScreen.GetTab(), ViewWidth, ViewHeight);
	switch (Hit.Kind)
	{
	case EWizardSettingsHitKind::Tab:
		SettingsScreen.SetTab(static_cast<EWizardSettingsTab>(Hit.Row));
		break;
	case EWizardSettingsHitKind::Key:
		if (FWizardKeyMap::Changeable().IsValidIndex(Hit.Row))
		{
			BeginKeyCapture(FWizardKeyMap::Changeable()[Hit.Row]);
		}
		break;
	case EWizardSettingsHitKind::Mute:
		if (Store)
		{
			Store->SetMuted(Hit.Row, !Store->IsMuted(Hit.Row));
		}
		break;
	case EWizardSettingsHitKind::Volume:
		SettingsDragChannel = Hit.Row;
		UpdateSettingsDrag();
		break;
	case EWizardSettingsHitKind::Background:
		if (Store)
		{
			// The first choice is "Keep playing", the second "Mute".
			Store->SetBackgroundMute(Hit.Index == 1);
		}
		break;
	case EWizardSettingsHitKind::TestSound:
		if (Store)
		{
			Store->PlayTestSound(static_cast<int32>(EWizardSoundChannel::Effects));
		}
		break;
	case EWizardSettingsHitKind::Back:
		ShowSettings(false);
		break;
	case EWizardSettingsHitKind::Reset:
		// Only the tab that is open.
		if (SettingsScreen.GetTab() == EWizardSettingsTab::Controls)
		{
			ResetKeys();
		}
		else if (Store && SettingsScreen.GetTab() == EWizardSettingsTab::Sound)
		{
			Store->ResetSound();
		}
		else if (Store)
		{
			Store->ResetDisplay();
		}
		break;
	case EWizardSettingsHitKind::Choice:
		if (Store)
		{
			Store->Set(static_cast<EWizardDisplayRow>(Hit.Row), Hit.Index);
		}
		break;
	case EWizardSettingsHitKind::Slider:
		SettingsDragRow = static_cast<EWizardDisplayRow>(Hit.Row);
		SettingsDragValue = Store ? Store->Get().Get(SettingsDragRow) : 0;
		UpdateSettingsDrag();
		break;
	default:
		break;
	}
}

void AWizardPawn::UpdateSettingsDrag()
{
	if (SettingsDragRow == EWizardDisplayRow::Count && SettingsDragChannel == INDEX_NONE)
	{
		return;
	}
	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	FVector2D Point;
	float ViewWidth = 0.0f;
	float ViewHeight = 0.0f;
	if (!PlayerController || !PlayerController->IsInputKeyDown(EKeys::LeftMouseButton) || !GetMouse(Point, ViewWidth, ViewHeight))
	{
		// Let go (a release during a pause is not always reported), or no mouse to follow.
		EndSettingsDrag();
		return;
	}
	if (SettingsDragChannel != INDEX_NONE)
	{
		// A volume is heard as it moves.
		if (UWizardSettingsSubsystem* Store = GetGameInstance() ? GetGameInstance()->GetSubsystem<UWizardSettingsSubsystem>() : nullptr)
		{
			Store->SetVolume(SettingsDragChannel, AWizardHUD::SettingsVolumeValue(SettingsDragChannel, static_cast<float>(Point.X), ViewWidth, ViewHeight));
		}
		return;
	}
	SettingsDragValue = AWizardHUD::SettingsSliderValue(static_cast<int32>(SettingsDragRow), static_cast<float>(Point.X), ViewWidth, ViewHeight);
	// The size of the HUD and menus is taken when the slider is let go: taken at once, the slider would move under the mouse.
	if (SettingsDragRow != EWizardDisplayRow::HudSize)
	{
		if (UWizardSettingsSubsystem* Store = GetGameInstance() ? GetGameInstance()->GetSubsystem<UWizardSettingsSubsystem>() : nullptr)
		{
			Store->Set(SettingsDragRow, SettingsDragValue);
		}
	}
}

void AWizardPawn::EndSettingsDrag()
{
	if (SettingsDragChannel != INDEX_NONE)
	{
		// Letting go of the master or the effects volume plays one effect at the new volume.
		const int32 Channel = SettingsDragChannel;
		SettingsDragChannel = INDEX_NONE;
		UWizardSettingsSubsystem* Store = GetGameInstance() ? GetGameInstance()->GetSubsystem<UWizardSettingsSubsystem>() : nullptr;
		if (Store && (Channel == static_cast<int32>(EWizardSoundChannel::Master) || Channel == static_cast<int32>(EWizardSoundChannel::Effects)))
		{
			Store->PlayTestSound(static_cast<int32>(EWizardSoundChannel::Effects));
		}
		return;
	}
	if (SettingsDragRow == EWizardDisplayRow::Count)
	{
		return;
	}
	if (UWizardSettingsSubsystem* Store = GetGameInstance() ? GetGameInstance()->GetSubsystem<UWizardSettingsSubsystem>() : nullptr)
	{
		Store->Set(SettingsDragRow, SettingsDragValue);
	}
	SettingsDragRow = EWizardDisplayRow::Count;
}

void AWizardPawn::OnBoltCompleted(const FInputActionValue& Value)
{
	bBoltHeld = false;
}

void AWizardPawn::OnSpear(const FInputActionValue& Value)
{
	if (Crowd && Crowd->IsCommandMode())
	{
		// Command mode: the right button steps to the next kind of unit. No spear is thrown.
		Crowd->CycleCommandKind();
		return;
	}
	if (Crowd && !IsStopped())
	{
		Crowd->ThrowSpear(AimPoint2D);
	}
}

void AWizardPawn::OnSummon(const FInputActionValue& Value)
{
	if (Crowd && !IsStopped())
	{
		Crowd->Summon();
	}
}

void AWizardPawn::OnBlink(const FInputActionValue& Value)
{
	if (Crowd && !IsStopped())
	{
		// The way he is being moved. Standing still, the way he faces.
		Crowd->StartBlink(MoveDirection.IsNearlyZero() ? AimDirection : MoveDirection);
	}
}

void AWizardPawn::OnCameraAngled()
{
	SetCameraMode(EWizardCameraMode::Angled);
}

void AWizardPawn::OnCameraTopDown()
{
	SetCameraMode(EWizardCameraMode::TopDown);
}

void AWizardPawn::OnCameraThirdPerson()
{
	SetCameraMode(EWizardCameraMode::ThirdPerson);
}

void AWizardPawn::OnToggleDebug()
{
	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (AWizardHUD* Hud = PlayerController ? Cast<AWizardHUD>(PlayerController->GetHUD()) : nullptr)
	{
		Hud->SetDebugVisible(!Hud->IsDebugVisible());
	}
}

bool AWizardPawn::IsDebugShown() const
{
	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	const AWizardHUD* Hud = PlayerController ? Cast<AWizardHUD>(PlayerController->GetHUD()) : nullptr;
	return Hud && Hud->IsDebugVisible();
}

void AWizardPawn::OnTestModels()
{
	if (Crowd && IsDebugShown())
	{
		Crowd->SetArmyModelShown(!Crowd->IsArmyModelShown());
	}
}

void AWizardPawn::OnTestEnemies()
{
	if (Crowd && IsDebugShown())
	{
		Crowd->SpawnEnemiesAt(40, ECrowdKind::Grunt, GetPosition2D() + FVector2D(900.0, 0.0), 400.0f);
	}
}

void AWizardPawn::OnTestArmy()
{
	if (Crowd && IsDebugShown())
	{
		Crowd->AddSkeletons(30);
	}
}

void AWizardPawn::OnTestSwitch()
{
	if (Crowd && IsDebugShown())
	{
		Crowd->SetPerfTest((Crowd->GetPerfTest() + 1) % 3);
	}
}

void AWizardPawn::UpdateAim(float DeltaSeconds)
{
	const AWizardPlayerController* WizardController = Cast<AWizardPlayerController>(GetController());
	FVector AimPoint;
	if (!IsStopped() && WizardController && WizardController->GetAimPoint(AimPoint))
	{
		AimPoint2D = FVector2D(AimPoint.X, AimPoint.Y);
		const FVector ToAim = AimPoint - GetActorLocation();
		const FVector2D Flat = FVector2D(ToAim.X, ToAim.Y).GetSafeNormal();
		if (!Flat.IsNearlyZero())
		{
			AimDirection = Flat;
		}
	}

	// He turns to face the aim point, eased so it never snaps.
	const FRotator Target(0.0f, FMath::RadiansToDegrees(FMath::Atan2(AimDirection.Y, AimDirection.X)), 0.0f);
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), Target, DeltaSeconds, 15.0f));
}

void AWizardPawn::UpdateCamera(float DeltaSeconds)
{
	float TargetPitch = 0.0f;
	float TargetArmLength = 0.0f;
	GetCameraTarget(CameraMode, TargetPitch, TargetArmLength);

	BlendElapsed = FMath::Min(BlendElapsed + DeltaSeconds, CameraBlendSeconds);
	const float Alpha = FMath::InterpEaseInOut(0.0f, 1.0f, BlendElapsed / CameraBlendSeconds, 2.0f);
	CameraPitch = FMath::Lerp(BlendFromPitch, TargetPitch, Alpha);
	CameraArmLength = FMath::Lerp(BlendFromArmLength, TargetArmLength, Alpha);

	// Angled and Top Down keep a fixed yaw. Third Person swings behind the aim direction.
	const float TargetYaw = CameraMode == EWizardCameraMode::ThirdPerson
		? FMath::RadiansToDegrees(FMath::Atan2(AimDirection.Y, AimDirection.X))
		: 0.0f;
	const float YawSpeed = CameraMode == EWizardCameraMode::ThirdPerson ? 2.0f : 6.0f;
	CameraYaw = FMath::RInterpTo(FRotator(0.0f, CameraYaw, 0.0f), FRotator(0.0f, TargetYaw, 0.0f), DeltaSeconds, YawSpeed).Yaw;

	SpringArm->TargetArmLength = CameraArmLength;
	SpringArm->SetWorldRotation(FRotator(CameraPitch, CameraYaw, 0.0f));
}

void AWizardPawn::UpdateBolt(float DeltaSeconds)
{
	const bool bMay = !IsStopped() && !(Crowd && (Crowd->IsShrineOpen() || Crowd->IsCommandMode()));
	// The animation is used only when his model is shown; the plain shapes cast at once.
	const bool bAnimated = IsModelShown() && BoltAnimation && BoltFireTime > 0.0f;
	const FCrowdTuning* Tuning = Crowd ? &Crowd->GetTuning() : nullptr;
	const bool bLeaves = BoltCast.Step(DeltaSeconds, bBoltHeld, bMay && Crowd, Tuning ? Tuning->BoltInterval : 0.45f,
		bAnimated && Tuning ? Tuning->BoltWindup : 0.0f, bAnimated ? BoltFireTime : 0.0f, bAnimated ? BoltAnimLength : 0.0f);
	if (bLeaves)
	{
		FireBolt();
	}
	// The cast is on the upper body only: the legs keep walking, running or standing under it.
	if (UWizardAnimInstance* Anim = Cast<UWizardAnimInstance>(GetMesh()->GetAnimInstance()))
	{
		const float Weight = bAnimated ? BoltCast.GetWeight(BoltFireTime, BoltAnimLength) : 0.0f;
		Anim->SetUpperBody(Weight > 0.0f ? BoltAnimation.Get() : nullptr, FMath::Max(BoltCast.AnimTime, 0.0f), Weight);
	}
}

void AWizardPawn::UpdateFall()
{
	const bool bAnimated = FallAnimation && IsModelShown();
	if (bFalling && Crowd)
	{
		// The god goes under the crowd: brought to his knees first, then down, slow at first and then faster.
		FallClock += GetWorld()->GetDeltaSeconds();
		Visual->SetRelativeLocation(FVector(0.0, 0.0, -FallDepth * GetFallSink(Crowd->GetFallAlpha(), bAnimated)));
	}
	if (UWizardAnimInstance* Anim = Cast<UWizardAnimInstance>(GetMesh()->GetAnimInstance()))
	{
		const float Shown = bAnimated ? GetFallShown() : 0.0f;
		Anim->SetWholeBody(Shown > 0.0f ? FallAnimation.Get() : nullptr, FMath::Min(FallClock, FallAnimation ? FallAnimation->GetPlayLength() : 0.0f), Shown);
	}
}

void AWizardPawn::FireBolt()
{
	if (Crowd)
	{
		Crowd->FireBolt(GetActorLocation(), FVector(AimDirection.X, AimDirection.Y, 0.0));
	}
}

void AWizardPawn::BeginFall()
{
	if (!bFalling)
	{
		FallClock = 0.0f;
	}
	bFalling = true;
	bBoltHeld = false;
}

void AWizardPawn::SetSpeedScale(float Scale)
{
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed * FMath::Clamp(Scale, 0.0f, 1.0f);
}

void AWizardPawn::EndRun()
{
	bRunEnded = true;
	bBoltHeld = false;
	MoveDirection = FVector2D::ZeroVector;
}
