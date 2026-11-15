#pragma once

#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "CoreMinimal.h"
#include "WizardAnimInstance.generated.h"

class UAnimSequenceBase;

/** Lays an animation over the upper body of whatever pose the animation blueprint made. */
struct FWizardAnimProxy : public FAnimInstanceProxy
{
	FWizardAnimProxy() = default;
	explicit FWizardAnimProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance) {}

	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
	virtual bool Evaluate(FPoseContext& Output) override;
	void BlendWhole(FPoseContext& Output) const;

	// Kept alive by the instance; read on the animation thread only between PreUpdate and the next one.
	const UAnimSequenceBase* Upper = nullptr;
	float UpperTime = 0.0f;
	float UpperWeight = 0.0f;
	FName UpperRoot;
	const UAnimSequenceBase* Whole = nullptr;
	float WholeTime = 0.0f;
	float WholeWeight = 0.0f;
};

/**
 * The parent class of the mannequin's animation blueprint. The blueprint's graph (idle, walk, run) makes the whole
 * pose; then an animation set with SetUpperBody replaces the pose from UpperBodyRoot up (spine, arms, head), so the
 * legs keep walking under a cast. Done here because the blueprint's graph cannot be edited from a script.
 */
UCLASS()
class UWizardAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	/** Shows this animation (a sequence or a montage) at this time on the upper body, this strongly (0 to 1). Null or 0: nothing. */
	void SetUpperBody(UAnimSequenceBase* Animation, float Time, float Weight);

	UFUNCTION(BlueprintPure, Category = "Wizard")
	float GetUpperBodyWeight() const { return UpperWeight; }

	/**
	 * Shows this animation at this time over the whole body, this strongly (0 to 1), on top of everything else: the
	 * pose the blueprint made is blended toward it, so raising the weight over a moment eases into it from whatever
	 * he was doing. Null or 0: nothing.
	 */
	void SetWholeBody(UAnimSequenceBase* Animation, float Time, float Weight);

	UFUNCTION(BlueprintPure, Category = "Wizard")
	float GetWholeBodyWeight() const { return WholeWeight; }

	/**
	 * The turn to give the upper body's first bone so that it points, in the figure's own space, the way it does in
	 * the animation, whatever the hips under it are doing: the hips' turn undone, then the animation's turn.
	 * Without this the cast was laid on hips that walk and lean their own way, and the throwing arm went wide.
	 */
	static FQuat MakeUpperRootRotation(const FQuat& ParentInFigure, const FQuat& AnimationInFigure);

	/** The bone the upper body starts at: it and everything under it take the animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wizard")
	FName UpperBodyRoot = TEXT("spine_01");

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override;

private:
	friend struct FWizardAnimProxy;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequenceBase> UpperAnimation;

	float UpperTime = 0.0f;
	float UpperWeight = 0.0f;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequenceBase> WholeAnimation;

	float WholeTime = 0.0f;
	float WholeWeight = 0.0f;
};
