#include "FNPickup.h"

#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "FNCharacter.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AFNPickup::AFNPickup()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	Item = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Item"));
	RootComponent = Item;
	Item->SetStaticMesh(Cube.Object);
	Item->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Column = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Column"));
	Column->SetupAttachment(Item);
	Column->SetStaticMesh(Cylinder.Object);
	Column->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Column->SetCastShadow(false);
	Column->SetUsingAbsoluteScale(true);
	Column->SetRelativeScale3D(FVector(0.2f, 0.2f, 14.f));
	Column->SetRelativeLocation(FVector(0.f, 0.f, 700.f));
}

void AFNPickup::BeginPlay()
{
	Super::BeginPlay();
	BaseLocation = GetActorLocation();

	// Rough silhouettes: long rifle bar, short fat scattergun, armour plate.
	switch (Type)
	{
	case EFNPickupType::Rifle:   Item->SetWorldScale3D(FVector(1.2f, 0.15f, 0.15f)); break;
	case EFNPickupType::Scatter: Item->SetWorldScale3D(FVector(0.7f, 0.25f, 0.25f)); break;
	case EFNPickupType::Armor:   Item->SetWorldScale3D(FVector(0.5f, 0.1f, 0.6f)); break;
	case EFNPickupType::Rune:    Item->SetWorldScale3D(FVector(0.3f)); Item->SetRelativeRotation(FRotator(45.f, 0.f, 45.f)); break;
	}

	const FLinearColor Wax(1.f, 0.72f, 0.25f);
	if (UMaterialInstanceDynamic* M = Item->CreateDynamicMaterialInstance(0)) { M->SetVectorParameterValue(TEXT("Color"), Wax); }
	if (UMaterialInstanceDynamic* M = Column->CreateDynamicMaterialInstance(0)) { M->SetVectorParameterValue(TEXT("Color"), Wax * 0.6f); }
}

void AFNPickup::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Age += DeltaSeconds;
	SetActorLocation(BaseLocation + FVector(0.f, 0.f, 12.f * FMath::Sin(Age * 2.5f)));
	SetActorRotation(Type == EFNPickupType::Rune ? FRotator(45.f, Age * 90.f, 45.f) : FRotator(0.f, Age * 45.f, 0.f));

	for (TActorIterator<AFNCharacter> It(GetWorld()); It; ++It)
	{
		AFNCharacter* P = *It;
		if (P->IsDead() || FVector::Dist(P->GetActorLocation(), GetActorLocation()) > 180.f)
		{
			continue;
		}
		switch (Type)
		{
		case EFNPickupType::Rifle:   P->GiveWeapon(EFNWeapon::Rifle); break;
		case EFNPickupType::Scatter: P->GiveWeapon(EFNWeapon::Scatter); break;
		case EFNPickupType::Armor:   P->GiveArmor(25.f); break;
		case EFNPickupType::Rune:    P->FindRune(RuneNode); break;
		}
		Destroy();
		return;
	}
}
