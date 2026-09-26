#include "FNPerunBoss.h"

#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "FNAmmoPickup.h"
#include "FNCharacter.h"
#include "FNHealthComponent.h"
#include "FNTelegraph.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FLinearColor BronzeTint(0.32f, 0.29f, 0.25f);
	const FLinearColor DeadlyRed(0.88f, 0.06f, 0.16f);   // #E0102A — reserved for the deadly signal
	const FLinearColor SlamAmber(0.9f, 0.55f, 0.12f);
	const FLinearColor BoltViolet(0.45f, 0.45f, 1.0f);   // Perun lightning is blue/violet, never pure white
	constexpr float FloorZ = 1.f;
}

AFNPerunBoss::AFNPerunBoss()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	GetCapsuleComponent()->InitCapsuleSize(90.f, 170.f);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AAIController::StaticClass();
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->MaxWalkSpeed = 330.f;

	auto MakePart = [this](const TCHAR* Name, UStaticMesh* PartMesh, const FVector& Loc, const FVector& Scale)
	{
		UStaticMeshComponent* C = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		C->SetupAttachment(RootComponent);
		C->SetStaticMesh(PartMesh);
		C->SetRelativeLocation(Loc);
		C->SetRelativeScale3D(Scale);
		// Meshes take hitscan (Visibility) so the head can be a weak point; they don't affect movement.
		C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		C->SetCollisionResponseToAllChannels(ECR_Ignore);
		C->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		return C;
	};
	Body = MakePart(TEXT("Body"), Cylinder.Object, FVector(0.f, 0.f, -10.f), FVector(1.6f, 1.6f, 3.2f));
	Head = MakePart(TEXT("Head"), Sphere.Object, FVector(0.f, 0.f, 200.f), FVector(0.9f));
	Head->ComponentTags.Add(TEXT("WeakPoint"));

	Health = CreateDefaultSubobject<UFNHealthComponent>(TEXT("Health"));
	Health->MaxHealth = 3000.f;
}

void AFNPerunBoss::BeginPlay()
{
	Super::BeginPlay();
	BodyMID = Body->CreateDynamicMaterialInstance(0);
	HeadMID = Head->CreateDynamicMaterialInstance(0);
	SetTint(BronzeTint);
	Health->OnDeath.AddDynamic(this, &AFNPerunBoss::HandleDeath);
	State = EFNBossState::Chase;
}

AFNCharacter* AFNPerunBoss::FindTarget() const
{
	AFNCharacter* Best = nullptr;
	float BestDist = TNumericLimits<float>::Max();
	for (TActorIterator<AFNCharacter> It(GetWorld()); It; ++It)
	{
		if (It->IsDead())
		{
			continue;
		}
		const float D = FVector::DistSquared(It->GetActorLocation(), GetActorLocation());
		if (D < BestDist)
		{
			BestDist = D;
			Best = *It;
		}
	}
	return Best;
}

void AFNPerunBoss::SetTint(const FLinearColor& Color)
{
	if (BodyMID) { BodyMID->SetVectorParameterValue(TEXT("Color"), Color); }
	if (HeadMID) { HeadMID->SetVectorParameterValue(TEXT("Color"), Color * 1.2f); }
}

void AFNPerunBoss::SetPose(bool bRaised)
{
	// Readable "pose" for the deadly attack: Perun rears up and grows taller.
	Body->SetRelativeScale3D(bRaised ? FVector(1.4f, 1.4f, 4.0f) : FVector(1.6f, 1.6f, 3.2f));
	Head->SetRelativeLocation(bRaised ? FVector(0.f, 0.f, 260.f) : FVector(0.f, 0.f, 200.f));
}

void AFNPerunBoss::SpawnTelegraph(const FVector& Location, float Radius, float Delay, float Damage, bool bDeadly, const FLinearColor& Color)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector At(Location.X, Location.Y, FloorZ);
	if (AFNTelegraph* T = GetWorld()->SpawnActor<AFNTelegraph>(At, FRotator::ZeroRotator, Params))
	{
		T->Setup(Radius, Delay, Damage, bDeadly, Color, this);
	}
}

void AFNPerunBoss::SpawnAmmo(int32 Count)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	for (int32 i = 0; i < Count; ++i)
	{
		const FVector2D Offset = FVector2D(FMath::VRand()).GetSafeNormal() * FMath::FRandRange(600.f, 1200.f);
		const FVector At(GetActorLocation().X + Offset.X, GetActorLocation().Y + Offset.Y, 60.f);
		GetWorld()->SpawnActor<AFNAmmoPickup>(At, FRotator::ZeroRotator, Params);
	}
}

void AFNPerunBoss::StartAttack(AFNCharacter* Target)
{
	++AttackCount;
	const float Dist = FVector::Dist2D(Target->GetActorLocation(), GetActorLocation());

	if (AttackCount % 4 == 0)
	{
		CurrentAttack = EFNBossAttack::Judgment;
	}
	else
	{
		CurrentAttack = Dist < SlamRadius * 1.3f ? EFNBossAttack::Slam : EFNBossAttack::Bolts;
	}

	State = EFNBossState::Windup;
	StateTime = 0.f;
	GetCharacterMovement()->StopMovementImmediately();

	switch (CurrentAttack)
	{
	case EFNBossAttack::Slam:
		StateDuration = SlamDelay;
		SpawnTelegraph(GetActorLocation(), SlamRadius, SlamDelay, SlamDamage, false, SlamAmber);
		break;

	case EFNBossAttack::Bolts:
	{
		StateDuration = BoltDelay + 0.9f;
		SpawnTelegraph(Target->GetActorLocation(), BoltRadius, BoltDelay, BoltDamage, false, BoltViolet);
		BoltTimers.SetNum(2);
		for (int32 i = 0; i < 2; ++i)
		{
			GetWorldTimerManager().SetTimer(BoltTimers[i], FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				if (State != EFNBossState::Dead)
				{
					if (AFNCharacter* T = FindTarget())
					{
						SpawnTelegraph(T->GetActorLocation(), BoltRadius, BoltDelay, BoltDamage, false, BoltViolet);
					}
				}
			}), 0.45f * (i + 1), false);
		}
		break;
	}

	case EFNBossAttack::Judgment:
		StateDuration = JudgmentDelay;
		SpawnTelegraph(GetActorLocation(), JudgmentRadius, JudgmentDelay, JudgmentDamage, true, DeadlyRed);
		SetPose(true);
		ImpulseRemaining = 0.15f; // one short silhouette impulse, not a strobe
		SetTint(FLinearColor(4.f, 4.f, 4.f));
		break;
	}
}

void AFNPerunBoss::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (State == EFNBossState::Dead)
	{
		return;
	}

	if (ImpulseRemaining > 0.f)
	{
		ImpulseRemaining -= DeltaSeconds;
		if (ImpulseRemaining <= 0.f)
		{
			SetTint(DeadlyRed * 0.8f); // hold red through the wind-up
		}
	}

	AFNCharacter* Target = FindTarget();
	if (!Target)
	{
		State = EFNBossState::Idle;
		return;
	}

	StateTime += DeltaSeconds;
	const FVector ToTarget = Target->GetActorLocation() - GetActorLocation();

	switch (State)
	{
	case EFNBossState::Idle:
		State = EFNBossState::Chase;
		break;

	case EFNBossState::Chase:
	{
		const FRotator Face(0.f, ToTarget.Rotation().Yaw, 0.f);
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), Face, DeltaSeconds, 5.f));
		if (ToTarget.Size2D() > SlamRadius * 0.7f)
		{
			AddMovementInput(ToTarget.GetSafeNormal2D(), 1.f);
		}
		Cooldown -= DeltaSeconds;
		if (Cooldown <= 0.f)
		{
			StartAttack(Target);
		}
		break;
	}

	case EFNBossState::Windup:
		if (StateTime >= StateDuration)
		{
			if (CurrentAttack == EFNBossAttack::Judgment)
			{
				SetPose(false);
				SetTint(BronzeTint);
				SpawnAmmo(2);
			}
			else if (CurrentAttack == EFNBossAttack::Bolts)
			{
				SpawnAmmo(1);
			}
			State = EFNBossState::Recover;
			StateTime = 0.f;
		}
		break;

	case EFNBossState::Recover:
		if (StateTime >= RecoverTime)
		{
			State = EFNBossState::Chase;
			Cooldown = FMath::FRandRange(MinCooldown, MaxCooldown);
		}
		break;

	default:
		break;
	}
}

void AFNPerunBoss::HandleDeath(AActor* /*Killer*/)
{
	State = EFNBossState::Dead;
	for (FTimerHandle& H : BoltTimers)
	{
		GetWorldTimerManager().ClearTimer(H);
	}
	SetPose(false);
	SetTint(FLinearColor(0.15f, 0.15f, 0.17f));
	Body->SetRelativeRotation(FRotator(0.f, 0.f, 12.f)); // kneels
	GetCharacterMovement()->DisableMovement();
}
