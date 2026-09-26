#include "FNSpark.h"

#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "FNCharacter.h"
#include "FNHealthComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AFNSpark::AFNSpark()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	Core = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Core"));
	RootComponent = Core;
	Core->SetStaticMesh(Sphere.Object);
	Core->SetWorldScale3D(FVector(0.35f));
	Core->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Core->SetCastShadow(false);
}

void AFNSpark::Launch(const FVector& Direction, float Speed, float InDamage, AActor* InInstigator)
{
	Velocity = Direction.GetSafeNormal() * Speed;
	Damage = InDamage;
	SourceActor = InInstigator;

	// Enemy thunder spark: cold blue-violet like Perun's lightning, never red/white (GDD §9, §12).
	if (UMaterialInstanceDynamic* MID = Core->CreateDynamicMaterialInstance(0))
	{
		MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.5f, 0.55f, 1.2f));
	}
}

void AFNSpark::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Life -= DeltaSeconds;
	const FVector From = GetActorLocation();
	const FVector To = From + Velocity * DeltaSeconds;

	// Stop on terrain and props.
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(FNSpark), false, this);
	if (SourceActor.IsValid())
	{
		Params.AddIgnoredActor(SourceActor.Get());
	}
	if (Life <= 0.f || GetWorld()->LineTraceSingleByChannel(Hit, From, To, ECC_WorldStatic, Params))
	{
		Destroy();
		return;
	}
	SetActorLocation(To);

	for (TActorIterator<AFNCharacter> It(GetWorld()); It; ++It)
	{
		AFNCharacter* Player = *It;
		if (Player->IsDead())
		{
			continue;
		}
		// Capsule-ish hit test: close in XY and within the body height.
		const FVector P = Player->GetActorLocation();
		if (FVector::Dist2D(P, To) < 70.f && FMath::Abs(P.Z - To.Z) < 110.f)
		{
			if (UFNHealthComponent* H = Player->FindComponentByClass<UFNHealthComponent>())
			{
				H->ApplyDamage(Damage, SourceActor.Get());
			}
			Destroy();
			return;
		}
	}
}
