#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FNAmmoPickup.generated.h"

class UStaticMeshComponent;

// Placeholder for "ammo drops from the boss's summoned mobs" (GDD §5).
UCLASS()
class FADEDNAV_API AFNAmmoPickup : public AActor
{
	GENERATED_BODY()

public:
	AFNAmmoPickup();

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Ammo")
	int32 Amount = 24;

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Orb;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Column;

	virtual void BeginPlay() override;

	float Age = 0.f;
	FVector BaseLocation;
};
