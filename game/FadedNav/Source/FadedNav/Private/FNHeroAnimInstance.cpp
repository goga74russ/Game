#include "FNHeroAnimInstance.h"

#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimationPoseData.h"
#include "AnimationRuntime.h"
#include "GameFramework/Pawn.h"

// Only PreUpdate reads the actor / instance. Evaluation uses this snapshot, so
// parallel pose evaluation never reads mutable gameplay state.
struct FFNHeroAnimProxy : public FAnimInstanceProxy
{
	using FAnimInstanceProxy::FAnimInstanceProxy;
	UAnimSequence* Clips[5] = {};
	UAnimSequence* Action = nullptr;
	UAnimSequence* Shot = nullptr;
	FVector Velocity = FVector::ZeroVector;
	float Phase = 0.f, IdleTime = 0.f, ActionTime = 0.f, ActionWeight = 0.f;
	float ShotTime = 0.f, ShotWeight = 0.f;

	virtual void PreUpdate(UAnimInstance* Instance, float Dt) override
	{
		FAnimInstanceProxy::PreUpdate(Instance, Dt);
		const UFNHeroAnimInstance* Hero = CastChecked<UFNHeroAnimInstance>(Instance);
		for (int32 I = 0; I < 5; ++I) { Clips[I] = Hero->Locomotion[I]; }
		Velocity = Hero->LocalVelocity;
		Action = Hero->ActionSequence;
		ActionWeight = 0.f;
		if (Action && Hero->HasFullBodyAction())
		{
			const float Duration = FMath::Max(0.01f, Hero->ActionDuration);
			ActionTime = FMath::Lerp(Hero->ActionStart, Hero->ActionEnd, Hero->ActionElapsed / Duration);
			const float Fade = FMath::Min(0.06f, Duration * 0.15f);
			ActionWeight = FMath::Clamp(FMath::Min(Hero->ActionElapsed, Duration - Hero->ActionElapsed) / Fade, 0.f, 1.f);
		}
		Shot = Hero->ShotSequence;
		ShotTime = Hero->ShotElapsed * 2.5f;
		const float ShotDuration = Shot ? Shot->GetPlayLength() / 2.5f : 0.f;
		ShotWeight = ShotDuration > Hero->ShotElapsed
			? FMath::Clamp(FMath::Min(Hero->ShotElapsed, ShotDuration - Hero->ShotElapsed) / 0.04f, 0.f, 1.f) : 0.f;
	}

	virtual void Update(float Dt) override
	{
		IdleTime += Dt;
		// All four run clips share normalized phase; diagonals do not mix two
		// unrelated footfalls. Slower movement slows the gait as well.
		Phase = FMath::Fmod(Phase + Dt * FMath::Clamp(Velocity.Size2D() / 500.f, 0.15f, 1.8f) / 0.75f, 1.f);
	}

	static void Sample(UAnimSequence* Clip, float Time, FPoseContext& Pose)
	{
		Pose.ResetToRefPose();
		if (!Clip) { return; }
		FAnimationPoseData Data(Pose);
		Clip->GetAnimationPose(Data, FAnimExtractContext(FMath::Clamp(Time, 0.f, Clip->GetPlayLength()), true));
	}

	virtual bool Evaluate(FPoseContext& Output) override
	{
		const float Speed = Velocity.Size2D();
		const float Move = FMath::Clamp(Speed / 80.f, 0.f, 1.f);
		const float Sum = FMath::Max(1.f, FMath::Abs(Velocity.X) + FMath::Abs(Velocity.Y));
		const float Weights[5] = { 1.f - Move, Move * FMath::Max(0.f, Velocity.X) / Sum,
			Move * FMath::Max(0.f, -Velocity.X) / Sum, Move * FMath::Max(0.f, -Velocity.Y) / Sum,
			Move * FMath::Max(0.f, Velocity.Y) / Sum };
		Output.ResetToRefPose();
		float Total = 0.f;
		for (int32 I = 0; I < 5; ++I)
		{
			if (!Clips[I] || Weights[I] < KINDA_SMALL_NUMBER) { continue; }
			FPoseContext Next(Output);
			const float Time = I == 0 ? FMath::Fmod(IdleTime, FMath::Max(0.01f, Clips[I]->GetPlayLength())) : Phase * Clips[I]->GetPlayLength();
			Sample(Clips[I], Time, Next);
			FAnimationPoseData Result(Output), Input(Next);
			FAnimationRuntime::BlendTwoPosesTogetherInPlace(Result, Input, Total / (Total + Weights[I]));
			Total += Weights[I];
		}
		if (Shot && ShotWeight > 0.f && ActionWeight < 0.01f)
		{
			FPoseContext Firing(Output), Mixed(Output);
			Sample(Shot, ShotTime, Firing);
			// Fire clips are full poses. Blend only the spine and its descendants;
			// the legs keep walking, including when the pointer is behind the hero.
			const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
			const int32 SpineMesh = Bones.GetPoseBoneIndexForBoneName(TEXT("spine_01"));
			const FCompactPoseBoneIndex Spine = Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(SpineMesh));
			TArray<float> BoneWeights;
			BoneWeights.Init(0.f, Output.Pose.GetNumBones());
			for (FCompactPoseBoneIndex B : Output.Pose.ForEachBoneIndex())
			{
				for (FCompactPoseBoneIndex P = B; P.IsValid(); P = Bones.GetParentBoneIndex(P))
				{
					if (P == Spine) { BoneWeights[B.GetInt()] = ShotWeight; break; }
				}
			}
			FAnimationPoseData Base(Output), Fire(Firing), Result(Mixed);
			FAnimationRuntime::BlendTwoPosesTogetherPerBone(Base, Fire, BoneWeights, Result);
			Output = Mixed;
		}
		if (Action && ActionWeight > 0.f)
		{
			FPoseContext FullBody(Output);
			Sample(Action, ActionTime, FullBody);
			// Foreign clips have no Wraith rifle track. Keep the integrated
			// rifle at its reference grip relative to the animated right hand.
			for (int32 Side = 0; Side < 2; ++Side)
			{
				const FBoneContainer& Bones = FullBody.Pose.GetBoneContainer();
				const FCompactPoseBoneIndex Weapon = Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(Bones.GetPoseBoneIndexForBoneName(Side == 0 ? TEXT("weapon_r") : TEXT("weapon_l"))));
				const FCompactPoseBoneIndex Hand = Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(Bones.GetPoseBoneIndexForBoneName(Side == 0 ? TEXT("hand_r") : TEXT("hand_l"))));
				if (Weapon.IsValid() && Hand.IsValid())
				{
					auto RefComponent = [&Bones](FCompactPoseBoneIndex Index)
					{
						FTransform Result = Bones.GetRefPoseTransform(Index);
						for (Index = Bones.GetParentBoneIndex(Index); Index.IsValid(); Index = Bones.GetParentBoneIndex(Index)) { Result *= Bones.GetRefPoseTransform(Index); }
						return Result;
					};
					FCSPose<FCompactPose> ComponentPose;
					ComponentPose.InitPose(FullBody.Pose);
					const FTransform Grip = RefComponent(Weapon).GetRelativeTransform(RefComponent(Hand));
					const FTransform Desired = Grip * ComponentPose.GetComponentSpaceTransform(Hand);
					const FCompactPoseBoneIndex Parent = Bones.GetParentBoneIndex(Weapon);
					FullBody.Pose[Weapon] = Parent.IsValid() ? Desired.GetRelativeTransform(ComponentPose.GetComponentSpaceTransform(Parent)) : Desired;
				}
			}
			if (Action->GetFName() == TEXT("A_HeroUnarmed"))
			{
				// Greystone's shield charge stores travel on the pelvis, not
				// the root. Root-motion extraction alone cannot remove it.
				const FBoneContainer& Bones = FullBody.Pose.GetBoneContainer();
				const FCompactPoseBoneIndex Pelvis = Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(Bones.GetPoseBoneIndexForBoneName(TEXT("pelvis"))));
				if (Pelvis.IsValid())
				{
					FVector Position = FullBody.Pose[Pelvis].GetTranslation();
					const FVector Rest = Bones.GetRefPoseTransform(Pelvis).GetTranslation();
					Position.X = Rest.X;
					Position.Y = Rest.Y;
					FullBody.Pose[Pelvis].SetTranslation(Position);
				}
			}
			FAnimationPoseData Result(Output), Input(FullBody);
			FAnimationRuntime::BlendTwoPosesTogetherInPlace(Result, Input, 1.f - ActionWeight);
		}
		Output.Pose.NormalizeRotations();
		return true;
	}
};

void UFNHeroAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	const FString Base = TEXT("/Game/ParagonWraith/Characters/Heroes/Wraith/Animations/");
	const TCHAR* Names[5] = { TEXT("Idle_Noise_A_NonAdditive"), TEXT("Jog_Fwd"), TEXT("Jog_Bwd"), TEXT("Jog_Left"), TEXT("Jog_Right") };
	for (int32 I = 0; I < 5; ++I) { Locomotion[I] = LoadObject<UAnimSequence>(nullptr, *(Base + Names[I])); }
	ShotSequence = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/HeroActions/A_HeroShot"));
}

void UFNHeroAnimInstance::NativeUpdateAnimation(float Dt)
{
	Super::NativeUpdateAnimation(Dt);
	if (const APawn* Pawn = TryGetPawnOwner())
	{
		LocalVelocity = Pawn->GetActorTransform().InverseTransformVectorNoScale(Pawn->GetVelocity());
		LocalVelocity.Z = 0.f;
	}
	ActionElapsed += Dt;
	ShotElapsed += Dt;
}

bool UFNHeroAnimInstance::PlayFullBody(UAnimSequence* Sequence, float Duration, float Start, float End)
{
	if (!Sequence || Duration <= 0.f) { return false; }
	ActionSequence = Sequence;
	ActionDuration = Duration;
	ActionElapsed = 0.f;
	ActionStart = FMath::Clamp(Start, 0.f, Sequence->GetPlayLength());
	ActionEnd = End < 0.f ? Sequence->GetPlayLength() : FMath::Clamp(End, ActionStart, Sequence->GetPlayLength());
	ShotElapsed = 10.f;
	return true;
}

void UFNHeroAnimInstance::PlayShot() { if (!HasFullBodyAction()) { ShotElapsed = 0.f; } }
void UFNHeroAnimInstance::CancelActions() { ActionDuration = 0.f; ShotElapsed = 10.f; }
FAnimInstanceProxy* UFNHeroAnimInstance::CreateAnimInstanceProxy() { return new FFNHeroAnimProxy(this); }
void UFNHeroAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) { delete Proxy; }
