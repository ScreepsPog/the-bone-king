#pragma once

#include "Game/WizardSounds.h"
#include "CoreMinimal.h"
#include "Game/WizardMenuState.h"
#include "Game/WizardKeys.h"
#include "Game/WizardSettings.h"
#include "Sim/BoltCast.h"
#include "GameFramework/Character.h"
#include "InputCoreTypes.h"
#include "WizardPawn.generated.h"

class UCameraComponent;
class UCrowdSubsystem;
class FBoolProperty;
class UInputAction;
class UInputMappingContext;
class USceneComponent;
class USpringArmComponent;
class UStaticMeshComponent;
struct FInputActionValue;

UENUM(BlueprintType)
enum class EWizardCameraMode : uint8
{
	Angled,
	TopDown UMETA(DisplayName = "Top Down"),
	ThirdPerson UMETA(DisplayName = "Third Person")
};

/**
 * The skeleton wizard: the Wizard King model on the UE5 mannequin skeleton (simple shapes if the model is missing or
 * switched off), WASD movement, mouse aim, Bolt, and three camera modes to compare.
 *
 * A Character (since 2026-11-13), so the mannequin's animation blueprints read his movement themselves. The class
 * keeps its first name. He turns to face the aim, and the camera does not turn with him.
 */
UCLASS()
class AWizardPawn : public ACharacter
{
	GENERATED_BODY()

public:
	AWizardPawn();

	/** Pitch in degrees and spring arm length for a camera mode. */
	static void GetCameraTarget(EWizardCameraMode Mode, float& OutPitch, float& OutArmLength);

	/** Accepts Angled, TopDown, ThirdPerson in any letter case. */
	static bool ParseCameraMode(const FString& Text, EWizardCameraMode& OutMode);

	/** "Angled", "Top Down" or "Third Person". */
	static FString GetCameraModeName(EWizardCameraMode Mode);

	/** Every control with the key it is on now. A control has one key and a key works one control (director, 2026-11-13). */
	static TArray<FWizardKeyBinding> GetKeyBindings();

	/** The key of a control, by the name in GetKeyBindings. An invalid key for an unknown name. */
	static FKey GetKey(FName Action);

	/** Eases to the new mode over CameraBlendSeconds. */
	void SetCameraMode(EWizardCameraMode Mode);

	UFUNCTION(BlueprintPure, Category = "Wizard")
	EWizardCameraMode GetCameraMode() const { return CameraMode; }

	UFUNCTION(BlueprintPure, Category = "Wizard")
	float GetCameraYaw() const { return CameraYaw; }

	FVector2D GetPosition2D() const;

	/** Height of the ground the wizard stands on. */
	UFUNCTION(BlueprintPure, Category = "Wizard")
	float GetGroundZ() const { return GroundZ; }

	/** Unit direction on the ground plane from the wizard toward the mouse. */
	FVector2D GetAimDirection() const { return AimDirection; }

	/** The direction on the ground he is being moved in by the keys held right now, or zero with none held. */
	FVector2D GetMoveDirection() const { return MoveDirection; }

	/** The point on the ground under the mouse. */
	FVector2D GetAimPoint2D() const { return AimPoint2D; }

	/** The scale that shows a model AssetHeight tall at Height (held to ModelHeightMin..ModelHeightMax). 1 if the asset has no height. */
	static float GetModelScale(float Height, float AssetHeight);

	/** Shows the model, or the simple shapes. The shapes stay if the model could not be loaded. */
	void SetModelShown(bool bShown);

	UFUNCTION(BlueprintPure, Category = "Wizard")
	bool IsModelShown() const;

	/** The model asset's name, or "shapes" while the simple shapes are shown. */
	UFUNCTION(BlueprintPure, Category = "Wizard")
	FString GetModelName() const;

	/** True while the animation blueprint is playing the walk or run. */
	UFUNCTION(BlueprintPure, Category = "Wizard")
	bool IsModelMoving() const;

	/** How tall the model stands, in units. */
	void SetModelHeight(float Height);

	UFUNCTION(BlueprintPure, Category = "Wizard")
	float GetModelHeight() const;

	/** Pauses or resumes the game. The pause menu is drawn by the HUD. */
	void TogglePause();

	/** Shows the start menu, with the game paused behind it; or closes it, which starts the run. */
	void ShowStartMenu(bool bShow);

	bool IsMenuOpen() const { return Menu.IsOpen(); }

	/**
	 * Opens the settings screen over the start menu or the pause menu (pausing the game if neither is up), or closes
	 * it, back to the menu it came from. The game stays paused.
	 */
	void ShowSettings(bool bShow);
	void SetSettingsTab(EWizardSettingsTab Tab) { SettingsScreen.SetTab(Tab); }
	const FWizardSettingsScreen& GetSettingsScreen() const { return SettingsScreen; }

	/** "closed", "controls", "sound" or "display". */
	UFUNCTION(BlueprintPure, Category = "Wizard")
	FString GetSettingsName() const { return SettingsScreen.GetName(); }

	/**
	 * Puts a control on another key, as the Controls tab does: two controls swap when the key is taken. The input is
	 * rebuilt and the choice saved. Says what happened in GetKeyMessage.
	 */
	EWizardRebind RebindKey(FName Action, const FKey& Key);

	/** Every key back to its default. */
	void ResetKeys();

	/** Waits for the next key or mouse button and puts the control on it; Escape cancels. NAME_None stops waiting. */
	void BeginKeyCapture(FName Action);
	FName GetCapturingAction() const { return CapturingAction; }

	/** What the last key change did, in words, for the line under the list. */
	UFUNCTION(BlueprintPure, Category = "Wizard")
	FString GetKeyMessage() const { return KeyMessage; }

	/** A slider of the settings screen is held, or a key is waited for: both are followed from the HUD, which is drawn while paused. */
	void UpdateSettingsDrag();
	void UpdateKeyCapture();
	bool IsSettingsDragging(EWizardDisplayRow Row) const { return SettingsDragRow == Row && Row != EWizardDisplayRow::Count; }
	int32 GetSettingsDragValue() const { return SettingsDragValue; }

	/** "start", "controls" or "none". */
	UFUNCTION(BlueprintPure, Category = "Wizard")
	FString GetMenuName() const { return Menu.GetName(); }

	/** Seconds into the casting animation, or below zero when it is not playing. */
	UFUNCTION(BlueprintPure, Category = "Wizard")
	float GetBoltAnimTime() const { return BoltCast.AnimTime; }

	/** The "Fire" moment of the casting animation, in seconds. 0: there is no animation, and a Bolt leaves at once. */
	UFUNCTION(BlueprintPure, Category = "Wizard")
	float GetBoltFireTime() const { return BoltFireTime; }

	/** Called when the god starts to fall: input stops and the body sinks as the fall progresses. */
	void BeginFall();

	/** Called when the game is finished at the kingdom gate: input stops. */
	void EndRun();

	/** How fast he walks, as a share of his full speed: below 1 while the commander's spear has him slowed. */
	void SetSpeedScale(float Scale);

	/** His full walking speed. */
	static constexpr float WalkSpeed = 600.0f;

	/** Hides him (the blink) or shows him again. */
	void SetBlinkHidden(bool bGone);
	bool IsBlinkHidden() const { return bBlinkHidden; }

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	static constexpr float CameraBlendSeconds = 0.4f;
	static constexpr float FallDepth = 200.0f;

	/**
	 * Overwhelmed, he is brought to his knees (the director's animation) and only then goes under. The animation
	 * comes in over FallBlendSeconds, slow at both ends, from whatever he was doing; he starts to sink once
	 * FallSinkFrom of the fall has passed.
	 */
	static constexpr float FallBlendSeconds = 0.45f;
	static constexpr float FallSinkFrom = 0.6f;
	/** How strongly the fall animation shows, Seconds after the fall began: 0 to 1, eased at both ends. */
	static float GetFallBlend(float Seconds) { return FMath::SmoothStep(0.0f, FallBlendSeconds, Seconds); }
	/** How far down he is (0 to 1 of FallDepth) at this share of the fall: at once without the animation, late with it. */
	static float GetFallSink(float Alpha, bool bAnimated)
	{
		const float Share = bAnimated ? FMath::Clamp((Alpha - FallSinkFrom) / (1.0f - FallSinkFrom), 0.0f, 1.0f) : FMath::Clamp(Alpha, 0.0f, 1.0f);
		return Share * Share;
	}
	bool HasFallAnimation() const { return FallAnimation != nullptr; }
	/** How strongly the fall animation shows now (0 when he is not falling). */
	float GetFallShown() const { return bFalling ? GetFallBlend(FallClock) : 0.0f; }

	/** The music for where the game is now: the start menu's, the end's, or the game's. */
	EWizardMusic GetWantedMusic() const;
	static constexpr float ModelHeightDefault = 220.0f;
	static constexpr float ModelHeightMin = 120.0f;
	static constexpr float ModelHeightMax = 400.0f;
	/** Below this speed the model stands. */
	static constexpr float ModelMoveSpeed = 10.0f;

protected:
	virtual void BeginPlay() override;

private:
	void OnMove(const FInputActionValue& Value);
	void OnMoveStopped(const FInputActionValue& Value);
	void OnCommand(const FInputActionValue& Value);
	void OnRecall(const FInputActionValue& Value);
	void OnCommandCount(const FInputActionValue& Value);
	void OnBoltStarted(const FInputActionValue& Value);
	void OnMenuClick(const FInputActionValue& Value);
	void OnMenuClickEnded(const FInputActionValue& Value);
	/** Puts every key on its control in the mapping context, from the key map, and has the input rebuilt. */
	void MapKeys();
	void OnBoltCompleted(const FInputActionValue& Value);
	void OnSpear(const FInputActionValue& Value);
	void OnSummon(const FInputActionValue& Value);
	void OnBlink(const FInputActionValue& Value);
	void OnInteract(const FInputActionValue& Value);
	void OnPause(const FInputActionValue& Value);
	void OnRestart(const FInputActionValue& Value);
	void OnMenuStart(const FInputActionValue& Value);
	void SetPaused(bool bPaused);
	void ClickSettings(FVector2D Point, float ViewWidth, float ViewHeight);
	void EndSettingsDrag();
	/** The mouse on the screen, and the screen's size. False when there is no mouse to read. */
	bool GetMouse(FVector2D& OutPoint, float& OutWidth, float& OutHeight) const;
	void OnCameraAngled();
	void OnCameraTopDown();
	void OnCameraThirdPerson();
	void OnToggleDebug();
	// Test keys, only while the debug overlay is shown.
	bool IsDebugShown() const;
	void OnTestModels();
	void OnTestEnemies();
	void OnTestArmy();
	void OnTestSwitch();
	void UpdateAim(float DeltaSeconds);
	void UpdateCamera(float DeltaSeconds);
	void UpdateBolt(float DeltaSeconds);
	void UpdateFall();
	void FireBolt();
	void LoadModel();
	void ApplyModel();

	/** Holds the model and the shapes so they can sink without moving the camera. */
	UPROPERTY(VisibleAnywhere, Category = "Wizard")
	TObjectPtr<USceneComponent> Visual;

	UPROPERTY(VisibleAnywhere, Category = "Wizard")
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(VisibleAnywhere, Category = "Wizard")
	TObjectPtr<UStaticMeshComponent> Head;

	UPROPERTY(VisibleAnywhere, Category = "Wizard")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, Category = "Wizard")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(Transient)
	TObjectPtr<UCrowdSubsystem> Crowd;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> MappingContext;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MoveAction;

	/** One for each control but the four of movement, which share MoveAction. Kept so they are not collected. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UInputAction>> KeyActions;

	// The left mouse button on the menus, whatever key Bolt is on.
	UPROPERTY()
	TObjectPtr<UInputAction> MenuClickAction;

	EWizardCameraMode CameraMode = EWizardCameraMode::Angled;
	float CameraPitch = -55.0f;
	float CameraArmLength = 1600.0f;
	float CameraYaw = 0.0f;
	float BlendFromPitch = -55.0f;
	float BlendFromArmLength = 1600.0f;
	float BlendElapsed = CameraBlendSeconds;

	FVector2D AimDirection = FVector2D(1.0, 0.0);
	FVector2D AimPoint2D = FVector2D(600.0, 0.0);
	FVector2D MoveDirection = FVector2D::ZeroVector;
	float GroundZ = 0.0f;
	// When the Bolt leaves, and where the casting animation is.
	FBoltCast BoltCast;

	// The casting animation (shown on the upper body only), its "Fire" moment and its length, in seconds.
	UPROPERTY()
	TObjectPtr<class UAnimMontage> BoltAnimation;

	UPROPERTY()
	TObjectPtr<class UAnimSequenceBase> FallAnimation;
	float FallClock = 0.0f;

	float BoltFireTime = 0.0f;
	float BoltAnimLength = 0.0f;
	// The settings screen, and the slider of it that is held (Count when none is) with the value under the mouse.
	FWizardSettingsScreen SettingsScreen;
	EWizardDisplayRow SettingsDragRow = EWizardDisplayRow::Count;
	int32 SettingsDragValue = 0;
	// The sound channel whose volume slider is held, or INDEX_NONE.
	int32 SettingsDragChannel = INDEX_NONE;
	// The control waiting for its new key, the keys that were down when the wait began, and what the last change did.
	FName CapturingAction;
	TSet<FKey> CaptureHeld;
	FString KeyMessage;
	// The frame a click was taken by a menu: Bolt does not also fire on it.
	uint64 MenuClickFrame = 0;
	bool bBlinkHidden = false;
	bool bBoltHeld = false;
	bool bFalling = false;
	bool bRunEnded = false;

	FWizardMenuState Menu;
	// The start menu goes up on the first frame, once the game can be paused.
	bool bStartMenuPending = false;

	// No moving, aiming or casting: he is falling, or the game is finished.
	bool IsStopped() const { return bFalling || bRunEnded; }

	bool bModelLoaded = false;
	float ModelAssetHeight = 0.0f;
	// The animation blueprint's own "should move" variable, found once by name, for IsModelMoving.
	FBoolProperty* AnimShouldMove = nullptr;
};
