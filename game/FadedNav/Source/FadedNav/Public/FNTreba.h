#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FNTreba.generated.h"

class UPointLightComponent;

// The chapter's single treba (checkpoint): heals, refills, becomes the respawn point (GDD §5, slice_v1).
// Readability channel 4: moon silver, slow even pulse.
UCLASS()
class FADEDNAV_API AFNTreba : public AActor
{
	GENERATED_BODY()

public:
	AFNTreba();
	virtual void Tick(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> Glow;

private:
	bool bPlayerInside = false;
	float Age = 0.f;
};
