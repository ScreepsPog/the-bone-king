#include "Game/WizardAnimInstance.h"

#include "Animation/AnimCurveTypes.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimationPoseData.h"
#include "Animation/AttributesRuntime.h"
#include "BonePose.h"

void UWizardAnimInstance::SetUpperBody(UAnimSequenceBase* Animation, float Time, float Weight)
{
	UpperAnimation = Animation;
	UpperTime = Time;
	UpperWeight = Animation ? FMath::Clamp(Weight, 0.0f, 1.0f) : 0.0f;
}

FQuat UWizardAnimInstance::MakeUpperRootRotation(const FQuat& ParentInFigure, const FQuat& AnimationInFigure)
{
	// In the figure's space a bone's turn is its parent's turn, then its own: so its own is the parent's undone, then the one wanted.
	return (ParentInFigure.Inverse() * AnimationInFigure).GetNormalized();
}

namespace
{
	/** A bone's turn in the figure's own space: its own turn under every parent's, up to the root. */
	FQuat WizardRotationInFigure(const FCompactPose& Pose, FCompactPoseBoneIndex Bone)
	{
		FQuat Turn = Pose[Bone].GetRotation();
		for (FCompactPoseBoneIndex Parent = Pose.GetParentBoneIndex(Bone); Parent != INDEX_NONE; Parent = Pose.GetParentBoneIndex(Parent))
		{
			Turn = Pose[Parent].GetRotation() * Turn;
		}
		return Turn.GetNormalized();
	}
}

FAnimInstanceProxy* UWizardAnimInstance::CreateAnimInstanceProxy()
{
	return new FWizardAnimProxy(this);
}

void UWizardAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy)
{
	delete InProxy;
}

void FWizardAnimProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
	FAnimInstanceProxy::PreUpdate(InAnimInstance, DeltaSeconds);
	const UWizardAnimInstance* Instance = Cast<UWizardAnimInstance>(InAnimInstance);
	Upper = Instance ? Instance->UpperAnimation.Get() : nullptr;
	UpperTime = Instance ? Instance->UpperTime : 0.0f;
	UpperWeight = Instance ? Instance->UpperWeight : 0.0f;
	UpperRoot = Instance ? Instance->UpperBodyRoot : NAME_None;
	Whole = Instance ? Instance->WholeAnimation.Get() : nullptr;
	WholeTime = Instance ? Instance->WholeTime : 0.0f;
	WholeWeight = Instance ? Instance->WholeWeight : 0.0f;
}

void UWizardAnimInstance::SetWholeBody(UAnimSequenceBase* Animation, float Time, float Weight)
{
	WholeAnimation = Animation;
	WholeTime = Time;
	WholeWeight = Animation ? FMath::Clamp(Weight, 0.0f, 1.0f) : 0.0f;
}

namespace
{
	/** The pose of an animation (a montage through its first track) at a time. False when it has none. */
	bool WizardReadPose(const UAnimSequenceBase* Animation, float Time, FAnimationPoseData& OutData)
	{
		const FAnimExtractContext Context(static_cast<double>(Time), false);
		if (const UAnimMontage* Montage = Cast<UAnimMontage>(Animation))
		{
			if (Montage->SlotAnimTracks.Num() == 0)
			{
				return false;
			}
			Montage->SlotAnimTracks[0].AnimTrack.GetAnimationPose(OutData, Context);
			return true;
		}
		Animation->GetAnimationPose(OutData, Context);
		return true;
	}
}

void FWizardAnimProxy::BlendWhole(FPoseContext& Output) const
{
	if (!Whole || WholeWeight <= 0.0f)
	{
		return;
	}
	const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
	FCompactPose WholePose;
	WholePose.SetBoneContainer(&Bones);
	WholePose.ResetToRefPose();
	FBlendedCurve WholeCurve;
	WholeCurve.InitFrom(Output.Curve);
	UE::Anim::FStackAttributeContainer WholeAttributes;
	FAnimationPoseData WholeData(WholePose, WholeCurve, WholeAttributes);
	if (!WizardReadPose(Whole, WholeTime, WholeData))
	{
		return;
	}
	for (const FCompactPoseBoneIndex Bone : Output.Pose.ForEachBoneIndex())
	{
		const FTransform Under = Output.Pose[Bone];
		Output.Pose[Bone].Blend(Under, WholePose[Bone], WholeWeight);
		Output.Pose[Bone].NormalizeRotation();
	}
}

bool FWizardAnimProxy::Evaluate(FPoseContext& Output)
{
	// The blueprint's graph first: the whole body, walking or standing.
	EvaluateAnimationNode(Output);
	if (!Upper || UpperWeight <= 0.0f)
	{
		BlendWhole(Output);
		return true;
	}
	const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
	const int32 RootMeshIndex = Bones.GetPoseBoneIndexForBoneName(UpperRoot);
	if (RootMeshIndex == INDEX_NONE)
	{
		BlendWhole(Output);
		return true;
	}
	const FCompactPoseBoneIndex Root = Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(RootMeshIndex));
	if (Root == INDEX_NONE)
	{
		BlendWhole(Output);
		return true;
	}

	// The animation's pose at its time. A montage is read through its first track.
	FCompactPose UpperPose;
	UpperPose.SetBoneContainer(&Bones);
	UpperPose.ResetToRefPose();
	FBlendedCurve UpperCurve;
	UpperCurve.InitFrom(Output.Curve);
	UE::Anim::FStackAttributeContainer UpperAttributes;
	FAnimationPoseData UpperData(UpperPose, UpperCurve, UpperAttributes);
	const FAnimExtractContext Context(static_cast<double>(UpperTime), false);
	if (const UAnimMontage* Montage = Cast<UAnimMontage>(Upper))
	{
		if (Montage->SlotAnimTracks.Num() == 0)
		{
			BlendWhole(Output);
			return true;
		}
		Montage->SlotAnimTracks[0].AnimTrack.GetAnimationPose(UpperData, Context);
	}
	else
	{
		Upper->GetAnimationPose(UpperData, Context);
	}

	// The upper body's first bone keeps the way it points in the animation, measured in the figure's own space, not
	// against the hips: the hips walk, lean and turn their own way, and the cast must still go straight ahead.
	const FCompactPoseBoneIndex RootParent = Output.Pose.GetParentBoneIndex(Root);
	const FQuat RootWanted = RootParent != INDEX_NONE
		? UWizardAnimInstance::MakeUpperRootRotation(WizardRotationInFigure(Output.Pose, RootParent), WizardRotationInFigure(UpperPose, Root))
		: UpperPose[Root].GetRotation();

	// From the root of the upper body up the chain, the animation takes over; the legs are left as they were.
	for (const FCompactPoseBoneIndex Bone : Output.Pose.ForEachBoneIndex())
	{
		if (Bone == Root || Bones.BoneIsChildOf(Bone, Root))
		{
			const FTransform Under = Output.Pose[Bone];
			FTransform Over = UpperPose[Bone];
			if (Bone == Root)
			{
				Over.SetRotation(RootWanted);
			}
			Output.Pose[Bone].Blend(Under, Over, UpperWeight);
			Output.Pose[Bone].NormalizeRotation();
		}
	}
	BlendWhole(Output);
	return true;
}
