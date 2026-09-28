#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "FNHeroAnimInstance.generated.h"

// Small native graph for the isometric prototype: directional locomotion,
// upper-body firing and full-body actions. No gameplay is driven by animation.
UCLASS(Transient)
class FADEDNAV_API UFNHeroAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	bool PlayFullBody(UAnimSequence* Sequence, float Duration, float Start = 0.f, float End = -1.f);
	void PlayShot();
	void CancelActions();
	bool HasFullBodyAction() const { return ActionElapsed < ActionDuration; }

	UPROPERTY(Transient) TObjectPtr<UAnimSequence> Locomotion[5]; // idle, forward, backward, left, right
	UPROPERTY(Transient) TObjectPtr<UAnimSequence> ShotSequence;
	UPROPERTY(Transient) TObjectPtr<UAnimSequence> ActionSequence;
	float ActionElapsed = 0.f, ActionDuration = 0.f, ActionStart = 0.f, ActionEnd = 0.f;
	float ShotElapsed = 10.f;
	FVector LocalVelocity = FVector::ZeroVector;

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override;
};
