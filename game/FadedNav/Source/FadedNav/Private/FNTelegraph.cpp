#include "FNTelegraph.h"

#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "FNCharacter.h"
#include "FNHealthComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FName ColorParam(TEXT("Color"));
	constexpr float DiscHeight = 0.02f;
}

AFNTelegraph::AFNTelegraph()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	Outer = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Outer"));
	RootComponent = Outer;
	Fill = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Fill"));
	Fill->SetupAttachment(Outer);

	for (UStaticMeshComponent* Disc : { Outer.Get(), Fill.Get() })
	{
		Disc->SetStaticMesh(Cylinder.Object);
		Disc->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Disc->SetCastShadow(false);
	}
}

void AFNTelegraph::Setup(float InRadius, float InDelay, float InDamage, bool bInDeadly, const FLinearColor& InColor, AActor* InInstigator)
{
	Radius = InRadius;
	Delay = FMath::Max(0.05f, InDelay);
	Damage = InDamage;
	bDeadly = bInDeadly;
	Color = InColor;
	SourceActor = InInstigator;

	// Basic cylinder is 100 units wide and tall, pivot at centre.
	const float XY = Radius / 50.f;
	Outer->SetWorldScale3D(FVector(XY, XY, DiscHeight));
	Fill->SetRelativeScale3D(FVector(0.01f, 0.01f, 1.f));
	Fill->SetRelativeLocation(FVector(0.f, 0.f, 60.f)); // slightly above outer (relative units are scaled by DiscHeight)

	OuterMID = Outer->CreateDynamicMaterialInstance(0);
	FillMID = Fill->CreateDynamicMaterialInstance(0);
	if (OuterMID) { OuterMID->SetVectorParameterValue(ColorParam, Color * 0.25f); }
	if (FillMID) { FillMID->SetVectorParameterValue(ColorParam, Color); }
}

void AFNTelegraph::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Elapsed += DeltaSeconds;
	if (!bResolved)
	{
		const float Alpha = FMath::Clamp(Elapsed / Delay, 0.f, 1.f);
		Fill->SetRelativeScale3D(FVector(Alpha, Alpha, 1.f));
		if (Elapsed >= Delay)
		{
			Resolve();
		}
	}
	else if (Elapsed >= Delay + 0.15f)
	{
		Destroy();
	}
}

void AFNTelegraph::Resolve()
{
	bResolved = true;

	// Single short impulse on impact — no strobe (GDD §9).
	if (FillMID) { FillMID->SetVectorParameterValue(ColorParam, bDeadly ? FLinearColor(3.f, 3.f, 3.f) : Color * 2.f); }

	const FVector Center = GetActorLocation();
	for (TActorIterator<AFNCharacter> It(GetWorld()); It; ++It)
	{
		AFNCharacter* Player = *It;
		const FVector P = Player->GetActorLocation();
		const float Dist2D = FVector::Dist2D(P, Center);
		const bool bInside = Dist2D <= Radius + Player->GetSimpleCollisionRadius() * 0.5f && FMath::Abs(P.Z - Center.Z) < 400.f;
		if (bInside)
		{
			if (UFNHealthComponent* Health = Player->FindComponentByClass<UFNHealthComponent>())
			{
				Health->ApplyDamage(Damage, SourceActor.Get());
			}
		}
	}
}
