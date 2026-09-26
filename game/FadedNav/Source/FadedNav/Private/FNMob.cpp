#include "FNMob.h"

#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "FNAmmoPickup.h"
#include "FNCharacter.h"
#include "FNGameMode.h"
#include "FNHealthComponent.h"
#include "FNSpark.h"
#include "FNTelegraph.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FLinearColor DarkMass(0.05f, 0.05f, 0.06f);    // channel 2: darker than the ground
	const FLinearColor RotLight(0.5f, 0.91f, 0.75f);     // #7FE8C0, enemy weak points only
	const FLinearColor StrikeAmber(0.9f, 0.55f, 0.12f);  // normal (non-deadly) telegraph
	const FLinearColor CrackBrown(0.55f, 0.4f, 0.25f);   // Ryhlets crack ring
	constexpr float PlayerHalfHeight = 90.f;

	UStaticMesh* Shape(const TCHAR* Name)
	{
		return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"), Name, Name));
	}
}

AFNMob::AFNMob()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	GetCapsuleComponent()->InitCapsuleSize(45.f, 60.f);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AAIController::StaticClass();
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->MaxWalkSpeed = OtrostSpeed;

	auto MakePart = [this](const TCHAR* Name)
	{
		UStaticMeshComponent* C = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		C->SetupAttachment(RootComponent);
		C->SetStaticMesh(Sphere.Object);
		// Take hitscan (Visibility) like the boss; movement uses the capsule.
		C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		C->SetCollisionResponseToAllChannels(ECR_Ignore);
		C->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		return C;
	};
	Body = MakePart(TEXT("Body"));
	Eye = MakePart(TEXT("Eye"));
	Eye->ComponentTags.Add(TEXT("WeakPoint"));
	Eye->SetCastShadow(false);

	Health = CreateDefaultSubobject<UFNHealthComponent>(TEXT("Health"));
}

void AFNMob::BeginPlay()
{
	Super::BeginPlay();
	BodyMID = Body->CreateDynamicMaterialInstance(0);
	EyeMID = Eye->CreateDynamicMaterialInstance(0);
	if (BodyMID) { BodyMID->SetVectorParameterValue(TEXT("Color"), DarkMass); }
	SetEyeGlow(1.f);
	Health->OnDeath.AddDynamic(this, &AFNMob::HandleDeath);
	HomeLocation = GetActorLocation();
	InitType(Type);
}

void AFNMob::InitType(EFNMobType InType)
{
	Type = InType;

	float HP = OtrostHealth;
	switch (Type)
	{
	case EFNMobType::Otrost:
		// Low crawling lump of vein.
		GetCapsuleComponent()->SetCapsuleSize(45.f, 60.f);
		Body->SetStaticMesh(Shape(TEXT("Sphere")));
		Body->SetRelativeLocation(FVector(0.f, 0.f, -25.f));
		Body->SetRelativeScale3D(FVector(1.1f, 0.8f, 0.7f));
		Eye->SetRelativeLocation(FVector(45.f, 0.f, 5.f));
		Eye->SetRelativeScale3D(FVector(0.22f));
		GetCharacterMovement()->MaxWalkSpeed = OtrostSpeed;
		HP = OtrostHealth;
		break;

	case EFNMobType::Strelnik:
		// Thunder-arrow standing in the ground; glowing tip is the weak point.
		GetCapsuleComponent()->SetCapsuleSize(35.f, 110.f);
		Body->SetStaticMesh(Shape(TEXT("Cylinder")));
		Body->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
		Body->SetRelativeScale3D(FVector(0.3f, 0.3f, 2.2f));
		Eye->SetRelativeLocation(FVector(0.f, 0.f, 115.f));
		Eye->SetRelativeScale3D(FVector(0.3f));
		GetCharacterMovement()->MaxWalkSpeed = 0.f;
		AggroRadius = StrelnikRange;
		HP = StrelnikHealth;
		break;

	case EFNMobType::Ryhlets:
		// Broad digger mound.
		GetCapsuleComponent()->SetCapsuleSize(55.f, 70.f);
		Body->SetStaticMesh(Shape(TEXT("Sphere")));
		Body->SetRelativeLocation(FVector(0.f, 0.f, -20.f));
		Body->SetRelativeScale3D(FVector(1.3f, 1.3f, 1.0f));
		Eye->SetRelativeLocation(FVector(55.f, 0.f, 20.f));
		Eye->SetRelativeScale3D(FVector(0.25f));
		GetCharacterMovement()->MaxWalkSpeed = 0.f;
		HP = RyhletsHealth;
		break;
	}

	Health->MaxHealth = HP;
	Health->Health = HP;
}

AFNCharacter* AFNMob::FindTarget() const
{
	for (TActorIterator<AFNCharacter> It(GetWorld()); It; ++It)
	{
		if (!It->IsDead())
		{
			return *It; // solo demo: one player
		}
	}
	return nullptr;
}

void AFNMob::EnterState(EFNMobState NewState, float Duration)
{
	State = NewState;
	StateTime = 0.f;
	StateDuration = Duration;
}

void AFNMob::SetEyeGlow(float Strength)
{
	if (EyeMID) { EyeMID->SetVectorParameterValue(TEXT("Color"), RotLight * Strength); }
}

void AFNMob::SetBurrowed(bool bBurrowed)
{
	SetActorHiddenInGame(bBurrowed);
	SetActorEnableCollision(!bBurrowed);
	Health->bInvulnerable = bBurrowed;
	if (bBurrowed)
	{
		GetCharacterMovement()->DisableMovement();
	}
	else
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
}

void AFNMob::SpawnTelegraph(const FVector& Location, float Radius, float Delay, float Damage, const FLinearColor& Color)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AFNTelegraph* T = GetWorld()->SpawnActor<AFNTelegraph>(Location, FRotator::ZeroRotator, Params))
	{
		T->Setup(Radius, Delay, Damage, false, Color, this);
	}
}

void AFNMob::FireSpark(const AFNCharacter* Target)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector From = Eye->GetComponentLocation();
	if (AFNSpark* S = GetWorld()->SpawnActor<AFNSpark>(From, FRotator::ZeroRotator, Params))
	{
		S->Launch(Target->GetActorLocation() - From, SparkSpeed, StrelnikDamage, this);
	}
}

void AFNMob::DropAmmo()
{
	// Ammo comes only from combat (GDD §5); Otrost is the main source.
	const bool bDrop = Type != EFNMobType::Otrost || FMath::FRand() < OtrostAmmoChance;
	if (!bDrop)
	{
		return;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AFNAmmoPickup* A = GetWorld()->SpawnActor<AFNAmmoPickup>(GetActorLocation() + FVector(0.f, 0.f, 20.f), FRotator::ZeroRotator, Params))
	{
		A->Amount = 12;
	}
}

void AFNMob::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (State == EFNMobState::Dead)
	{
		return;
	}

	AFNCharacter* Target = FindTarget();
	StateTime += DeltaSeconds;

	switch (Type)
	{
	case EFNMobType::Otrost:   TickOtrost(Target, DeltaSeconds); break;
	case EFNMobType::Strelnik: TickStrelnik(Target, DeltaSeconds); break;
	case EFNMobType::Ryhlets:  TickRyhlets(Target, DeltaSeconds); break;
	}
}

void AFNMob::TickOtrost(AFNCharacter* Target, float DeltaSeconds)
{
	const bool bHasTarget = Target && FVector::Dist(Target->GetActorLocation(), HomeLocation) < LeashRadius;
	const FVector ToTarget = bHasTarget ? Target->GetActorLocation() - GetActorLocation() : FVector::ZeroVector;

	switch (State)
	{
	case EFNMobState::Idle:
		if (bHasTarget && ToTarget.Size() < AggroRadius)
		{
			EnterState(EFNMobState::Chase, 0.f);
		}
		else if (FVector::Dist2D(GetActorLocation(), HomeLocation) > 150.f)
		{
			AddMovementInput((HomeLocation - GetActorLocation()).GetSafeNormal2D(), 0.6f); // drift back home
		}
		break;

	case EFNMobState::Chase:
		if (!bHasTarget)
		{
			EnterState(EFNMobState::Idle, 0.f);
			break;
		}
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(0.f, ToTarget.Rotation().Yaw, 0.f), DeltaSeconds, 8.f));
		if (ToTarget.Size2D() > OtrostReach)
		{
			AddMovementInput(ToTarget.GetSafeNormal2D(), 1.f);
		}
		else
		{
			// Short lunge strike in front: small amber disc, rot-light flares during the wind-up.
			const FVector Front = GetActorLocation() + GetActorForwardVector() * OtrostReach * 0.5f
				- FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - 1.f);
			SpawnTelegraph(Front, OtrostReach * 0.75f, OtrostWindup, OtrostDamage, StrikeAmber);
			SetEyeGlow(3.f);
			GetCharacterMovement()->StopMovementImmediately();
			EnterState(EFNMobState::Windup, OtrostWindup);
		}
		break;

	case EFNMobState::Windup:
		if (StateTime >= StateDuration)
		{
			SetEyeGlow(1.f);
			EnterState(EFNMobState::Recover, 0.7f);
		}
		break;

	case EFNMobState::Recover:
		if (StateTime >= StateDuration)
		{
			EnterState(EFNMobState::Chase, 0.f);
		}
		break;

	default:
		break;
	}
}

void AFNMob::TickStrelnik(AFNCharacter* Target, float DeltaSeconds)
{
	if (!Target)
	{
		return;
	}
	const FVector From = Eye->GetComponentLocation();
	const FVector ToTarget = Target->GetActorLocation() - From;
	const bool bInRange = ToTarget.Size() < StrelnikRange;

	bool bLineOfSight = false;
	if (bInRange)
	{
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(FNStrelnikLOS), false, this);
		bLineOfSight = !GetWorld()->LineTraceSingleByChannel(Hit, From, Target->GetActorLocation(), ECC_WorldStatic, Params);
	}

	switch (State)
	{
	case EFNMobState::Idle:
	case EFNMobState::Chase:
		if (bInRange && bLineOfSight)
		{
			EnterState(EFNMobState::Windup, StrelnikWindup);
		}
		break;

	case EFNMobState::Windup:
		// Tip brightens over the wind-up: the read is "it's charging".
		SetEyeGlow(1.f + 3.f * FMath::Clamp(StateTime / StateDuration, 0.f, 1.f));
		if (StateTime >= StateDuration)
		{
			SetEyeGlow(1.f);
			if (bInRange && bLineOfSight)
			{
				FireSpark(Target);
			}
			EnterState(EFNMobState::Recover, StrelnikCooldown);
		}
		break;

	case EFNMobState::Recover:
		if (StateTime >= StateDuration)
		{
			EnterState(EFNMobState::Idle, 0.f);
		}
		break;

	default:
		break;
	}
}

void AFNMob::TickRyhlets(AFNCharacter* Target, float /*DeltaSeconds*/)
{
	const bool bHasTarget = Target && FVector::Dist(Target->GetActorLocation(), HomeLocation) < LeashRadius;

	switch (State)
	{
	case EFNMobState::Idle:
		if (bHasTarget && FVector::Dist(Target->GetActorLocation(), GetActorLocation()) < AggroRadius)
		{
			SetBurrowed(true);
			EnterState(EFNMobState::Burrowed, RyhletsBurrowTime);
		}
		break;

	case EFNMobState::Burrowed:
		if (StateTime >= StateDuration)
		{
			if (!bHasTarget)
			{
				// Lost the player: resurface at home.
				SetActorLocation(HomeLocation, false, nullptr, ETeleportType::TeleportPhysics);
				SetBurrowed(false);
				EnterState(EFNMobState::Idle, 0.f);
				break;
			}
			// Crack ring under the player; surfaces there when it fills.
			BurrowTarget = Target->GetActorLocation() - FVector(0.f, 0.f, PlayerHalfHeight - 1.f);
			SpawnTelegraph(BurrowTarget, RyhletsRadius, RyhletsTelegraph, RyhletsDamage, CrackBrown);
			EnterState(EFNMobState::Windup, RyhletsTelegraph);
		}
		break;

	case EFNMobState::Windup:
		if (StateTime >= StateDuration)
		{
			const float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			SetActorLocation(BurrowTarget + FVector(0.f, 0.f, HalfHeight + 5.f), false, nullptr, ETeleportType::TeleportPhysics);
			SetBurrowed(false);
			SetEyeGlow(2.f);
			EnterState(EFNMobState::Recover, RyhletsExposed); // exposed: the window to punish it
		}
		break;

	case EFNMobState::Recover:
		if (StateTime >= StateDuration)
		{
			SetEyeGlow(1.f);
			SetBurrowed(true);
			EnterState(EFNMobState::Burrowed, RyhletsBurrowTime);
		}
		break;

	default:
		break;
	}
}

void AFNMob::HandleDeath(AActor* /*Killer*/)
{
	EnterState(EFNMobState::Dead, 0.f);
	SetActorEnableCollision(false);
	GetCharacterMovement()->DisableMovement();
	if (BodyMID) { BodyMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.12f, 0.12f, 0.12f)); }
	SetEyeGlow(0.f);
	Body->SetRelativeScale3D(Body->GetRelativeScale3D() * FVector(1.f, 1.f, 0.4f)); // slumps
	DropAmmo();

	if (AFNGameMode* GM = GetWorld()->GetAuthGameMode<AFNGameMode>())
	{
		GM->NotifyMobKilled(this);
	}
	SetLifeSpan(4.f);
}
