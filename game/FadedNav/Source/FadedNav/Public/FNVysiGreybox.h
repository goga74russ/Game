#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FNVysiGreybox.generated.h"

class ADirectionalLight;
class UPointLightComponent;

// Grey-box of the demo chapter "Thunder Heights" built from primitives.
// Layout source: docs/level/vysi_layout_v0.1.md (coordinates in metres, X = north/uphill, Z = up).
UCLASS()
class FADEDNAV_API AFNVysiGreybox : public AActor
{
	GENERATED_BODY()

public:
	AFNVysiGreybox();

	virtual void Tick(float DeltaSeconds) override;

	static FVector PlayerStart();   // cm
	static FVector ArenaCenter();   // cm

	void Build(ADirectionalLight* InSun);

private:
	AActor* Box(const FVector& TopCenterM, const FVector& SizeM, const FLinearColor& Color, bool bCollide = true);
	AActor* Ramp(const FVector& FromM, const FVector& ToM, float WidthM, const FLinearColor& Color);
	AActor* Cyl(const FVector& CenterM, float DiameterM, float HeightM, const FLinearColor& Color, bool bCollide = true);
	AActor* Ball(const FVector& CenterM, float DiameterM, const FLinearColor& Color);
	void Label(const FVector& PosM, const FString& Text, const FColor& Color);
	void LootColumn(const FVector& PosM);

	UPROPERTY() TObjectPtr<ADirectionalLight> Sun;
	UPROPERTY() TObjectPtr<UPointLightComponent> OakFlash;

	float LightningTimer = 12.f;
	float FlashRemaining = 0.f;
	float ShotClock = 0.f;
	int32 ShotTaken = -1;
};
