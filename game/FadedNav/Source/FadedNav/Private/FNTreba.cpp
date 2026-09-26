#include "FNTreba.h"

#include "Components/PointLightComponent.h"
#include "EngineUtils.h"
#include "FNCharacter.h"

AFNTreba::AFNTreba()
{
	PrimaryActorTick.bCanEverTick = true;
	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	RootComponent = Glow;
	Glow->SetLightColor(FLinearColor(0.75f, 0.82f, 1.f));
	Glow->SetAttenuationRadius(1200.f);
	Glow->SetIntensity(20000.f);
	Glow->SetCastShadows(false);
}

void AFNTreba::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Age += DeltaSeconds;
	Glow->SetIntensity(20000.f + 8000.f * FMath::Sin(Age * 1.5f)); // slow, even pulse

	for (TActorIterator<AFNCharacter> It(GetWorld()); It; ++It)
	{
		const float Dist = FVector::Dist2D(It->GetActorLocation(), GetActorLocation());
		if (!bPlayerInside && Dist < 350.f && !It->IsDead())
		{
			bPlayerInside = true;
			It->RestAtTreba(GetActorLocation() + FVector(0.f, 0.f, 50.f));
		}
		else if (bPlayerInside && Dist > 800.f)
		{
			bPlayerInside = false; // rest again next visit
		}
		break;
	}
}
