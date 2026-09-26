#include "FNAmmoPickup.h"

#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "FNCharacter.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AFNAmmoPickup::AFNAmmoPickup()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	Orb = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Orb"));
	RootComponent = Orb;
	Orb->SetStaticMesh(Sphere.Object);
	Orb->SetWorldScale3D(FVector(0.35f));
	Orb->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Vertical column = loot channel in the readability hierarchy (style_v0.1 §2).
	Column = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Column"));
	Column->SetupAttachment(Orb);
	Column->SetStaticMesh(Cylinder.Object);
	Column->SetRelativeScale3D(FVector(0.15f, 0.15f, 12.f));
	Column->SetRelativeLocation(FVector(0.f, 0.f, 600.f));
	Column->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Column->SetCastShadow(false);
}

void AFNAmmoPickup::BeginPlay()
{
	Super::BeginPlay();
	BaseLocation = GetActorLocation();

	const FLinearColor Wax(1.0f, 0.72f, 0.25f); // "wax gold"
	for (UStaticMeshComponent* C : { Orb.Get(), Column.Get() })
	{
		if (UMaterialInstanceDynamic* MID = C->CreateDynamicMaterialInstance(0))
		{
			MID->SetVectorParameterValue(TEXT("Color"), C == Column ? Wax * 0.6f : Wax);
		}
	}
}

void AFNAmmoPickup::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Age += DeltaSeconds;
	SetActorLocation(BaseLocation + FVector(0.f, 0.f, 10.f * FMath::Sin(Age * 3.f)));

	for (TActorIterator<AFNCharacter> It(GetWorld()); It; ++It)
	{
		if (FVector::Dist(It->GetActorLocation(), GetActorLocation()) < 140.f && It->AddReserveAmmo(Amount))
		{
			Destroy();
			return;
		}
	}
}
