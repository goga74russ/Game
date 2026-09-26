#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FNSpark.generated.h"

class UStaticMeshComponent;

// Slow visible projectile of the Strelnik: flies straight to where the player was, so strafing dodges it.
UCLASS()
class FADEDNAV_API AFNSpark : public AActor
{
	GENERATED_BODY()

public:
	AFNSpark();

	void Launch(const FVector& Direction, float Speed, float InDamage, AActor* InInstigator);

	virtual void Tick(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Core;

	TWeakObjectPtr<AActor> SourceActor;
	FVector Velocity = FVector::ZeroVector;
	float Damage = 10.f;
	float Life = 2.5f;
};
