#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FNTelegraph.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

// Ground area attack: shows a disc that fills over Delay, then damages players inside.
UCLASS()
class FADEDNAV_API AFNTelegraph : public AActor
{
	GENERATED_BODY()

public:
	AFNTelegraph();

	void Setup(float InRadius, float InDelay, float InDamage, bool bInDeadly, const FLinearColor& InColor, AActor* InInstigator);

	virtual void Tick(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Outer;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Fill;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> OuterMID;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> FillMID;

	TWeakObjectPtr<AActor> SourceActor;

	float Radius = 300.f;
	float Delay = 1.f;
	float Damage = 20.f;
	bool bDeadly = false;
	FLinearColor Color = FLinearColor::Yellow;

	float Elapsed = 0.f;
	bool bResolved = false;

	void Resolve();
};
