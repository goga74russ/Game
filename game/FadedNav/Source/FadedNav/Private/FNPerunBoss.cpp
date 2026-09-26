#include "FNPerunBoss.h"

#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "EngineUtils.h"
#include "FNAmmoPickup.h"
#include "FNCharacter.h"
#include "FNHealthComponent.h"
#include "FNMob.h"
#include "FNTelegraph.h"
#include "FNVysiGreybox.h"
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
	const FLinearColor ParryGlow(0.75f, 0.95f, 1.0f);    // "glowing point" on the wind-up
	constexpr float FloorZ = 1.f;
	constexpr float M = 100.f;

	const TCHAR* Perun = TEXT("Перун");
}

AFNPerunBoss::AFNPerunBoss()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMat(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

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
	ParryPoint = MakePart(TEXT("ParryPoint"), Sphere.Object, FVector(40.f, 0.f, 90.f), FVector(0.35f));
	ParryPoint->ComponentTags.Add(TEXT("ParryPoint"));
	ParryPoint->SetCastShadow(false);
	if (ShapeMat.Succeeded())
	{
		Head->SetMaterial(0, ShapeMat.Object);
		ParryPoint->SetMaterial(0, ShapeMat.Object);
	}

	Health = CreateDefaultSubobject<UFNHealthComponent>(TEXT("Health"));
	Health->MaxHealth = 3000.f;

	// Temporary visuals: Paragon Greystone as the mentor (Epic, free for UE projects).
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> GreyMesh(TEXT("/Game/ParagonGreystone/Characters/Heroes/Greystone/Meshes/Greystone.Greystone"));
	static ConstructorHelpers::FClassFinder<UAnimInstance> GreyAnim(TEXT("/Game/ParagonGreystone/Characters/Heroes/Greystone/Greystone_AnimBlueprint"));
	static ConstructorHelpers::FObjectFinder<UAnimMontage> MontA(TEXT("/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations/Attack_PrimaryA_Montage.Attack_PrimaryA_Montage"));
	static ConstructorHelpers::FObjectFinder<UAnimMontage> MontB(TEXT("/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations/Attack_PrimaryB_Montage.Attack_PrimaryB_Montage"));
	static ConstructorHelpers::FObjectFinder<UAnimMontage> MontC(TEXT("/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations/Attack_PrimaryC_Montage.Attack_PrimaryC_Montage"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> DeathSeq(TEXT("/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations/Death.Death"));

	if (GreyMesh.Succeeded())
	{
		bHasSkeletalVisual = true;
		GetMesh()->SetSkeletalMesh(GreyMesh.Object);
		GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -170.f), FRotator(0.f, -90.f, 0.f));
		GetMesh()->SetRelativeScale3D(FVector(1.6f));
		if (GreyAnim.Succeeded())
		{
			GetMesh()->SetAnimInstanceClass(GreyAnim.Class);
		}
		// Grey-box volumes stay as invisible hit volumes; the head collider follows the head bone.
		Body->SetVisibility(false);
		Head->SetVisibility(false);
		Head->SetupAttachment(GetMesh(), TEXT("head"));
		Head->SetUsingAbsoluteScale(true);
		Head->SetRelativeLocation(FVector::ZeroVector);
		Head->SetRelativeScale3D(FVector(0.7f));
		// The parry point sits on the chest.
		ParryPoint->SetupAttachment(GetMesh(), TEXT("spine_03"));
		ParryPoint->SetUsingAbsoluteScale(true);
		ParryPoint->SetRelativeLocation(FVector::ZeroVector);
		ParryPoint->SetRelativeScale3D(FVector(0.45f));
	}
	SlamMontage = MontB.Object;
	BoltsMontage = MontA.Object;
	JudgmentMontage = MontC.Object;
	DeathAnim = DeathSeq.Object;
}

void AFNPerunBoss::BeginPlay()
{
	Super::BeginPlay();
	BodyMID = Body->CreateDynamicMaterialInstance(0);
	HeadMID = Head->CreateDynamicMaterialInstance(0);
	if (UMaterialInstanceDynamic* PM = ParryPoint->CreateDynamicMaterialInstance(0)) { PM->SetVectorParameterValue(TEXT("Color"), ParryGlow); }
	SetTint(BronzeTint);
	SetParryWindow(false);
	Health->OnDeath.AddDynamic(this, &AFNPerunBoss::HandleDeath);
	HomeLocation = GetActorLocation();
	LastHealth = Health->MaxHealth;
	State = EFNBossState::Idle;
}

bool AFNPerunBoss::IsFightActive() const
{
	switch (State)
	{
	case EFNBossState::Intro: case EFNBossState::Chase: case EFNBossState::Windup:
	case EFNBossState::Recover: case EFNBossState::Stagger: case EFNBossState::Transition:
		return true;
	default:
		return false;
	}
}

bool AFNPerunBoss::HasSubtitle() const
{
	return GetWorld() && GetWorld()->GetTimeSeconds() < SubUntil && !SubText.IsEmpty();
}

void AFNPerunBoss::Say(const FString& Speaker, const FString& Text, float Duration)
{
	SubSpeaker = Speaker;
	SubText = Text;
	SubUntil = GetWorld()->GetTimeSeconds() + Duration;
}

void AFNPerunBoss::EnterState(EFNBossState NewState, float Duration)
{
	State = NewState;
	StateTime = 0.f;
	StateDuration = Duration;
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

void AFNPerunBoss::SetOverlay(const FLinearColor* Color)
{
	// Placeholder silhouette impulse: flash the grey-box volume around the model (proper overlay material in stage 1).
	if (!bHasSkeletalVisual)
	{
		return;
	}
	Body->SetVisibility(Color != nullptr);
	if (Color)
	{
		SetTint(*Color);
	}
}

void AFNPerunBoss::SetParryWindow(bool bOpen)
{
	bParryWindow = bOpen;
	ParryPoint->SetVisibility(bOpen);
	ParryPoint->SetCollisionEnabled(bOpen ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
}

void AFNPerunBoss::PlayMontage(UAnimMontage* Montage, float Duration)
{
	if (!Montage)
	{
		return;
	}
	if (UAnimInstance* Anim = GetMesh()->GetAnimInstance())
	{
		const float Rate = Duration > 0.f ? FMath::Clamp(Montage->GetPlayLength() / Duration, 0.4f, 2.f) : 1.f;
		Anim->Montage_Play(Montage, Rate);
	}
}

void AFNPerunBoss::SetPose(bool bRaised)
{
	Body->SetRelativeScale3D(bRaised ? FVector(1.4f, 1.4f, 4.0f) : FVector(1.6f, 1.6f, 3.2f));
	if (!bHasSkeletalVisual)
	{
		Head->SetRelativeLocation(bRaised ? FVector(0.f, 0.f, 260.f) : FVector(0.f, 0.f, 200.f));
	}
}

AFNTelegraph* AFNPerunBoss::SpawnTelegraph(const FVector& Location, float Radius, float Delay, float Damage, bool bDeadly, const FLinearColor& Color)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector At(Location.X, Location.Y, (bDeadly || Radius == SlamRadius) ? GetActorLocation().Z - 165.f : Location.Z - 85.f);
	AFNTelegraph* T = GetWorld()->SpawnActor<AFNTelegraph>(At, FRotator::ZeroRotator, Params);
	if (T)
	{
		T->Setup(Radius, Delay, Damage, bDeadly, Color, this);
		ActiveTelegraphs.Add(T);
	}
	return T;
}

void AFNPerunBoss::CancelAttack()
{
	for (FTimerHandle& H : BoltTimers)
	{
		GetWorldTimerManager().ClearTimer(H);
	}
	for (TWeakObjectPtr<AFNTelegraph>& T : ActiveTelegraphs)
	{
		if (T.IsValid()) { T->Destroy(); }
	}
	ActiveTelegraphs.Reset();
	SetPose(false);
	SetOverlay(nullptr);
	SetTint(BronzeTint);
	SetParryWindow(false);
	ImpulseRemaining = 0.f;
	if (UAnimInstance* Anim = GetMesh()->GetAnimInstance())
	{
		Anim->Montage_Stop(0.2f);
	}
}

void AFNPerunBoss::SpawnAmmo(int32 Count)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	for (int32 i = 0; i < Count; ++i)
	{
		const FVector2D Offset = FVector2D(FMath::VRand()).GetSafeNormal() * FMath::FRandRange(600.f, 1200.f);
		const FVector At(HomeLocation.X + Offset.X, HomeLocation.Y + Offset.Y, HomeLocation.Z - 110.f);
		GetWorld()->SpawnActor<AFNAmmoPickup>(At, FRotator::ZeroRotator, Params);
	}
}

void AFNPerunBoss::SpawnAdds(int32 Count)
{
	// Summoned Otrosts at the arena edge: the in-fight ammo source (GDD §5).
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	for (int32 i = 0; i < Count; ++i)
	{
		const float A = FMath::FRandRange(0.f, 2.f * PI);
		const FVector Top = HomeLocation + FVector(FMath::Cos(A) * 1700.f, FMath::Sin(A) * 1700.f, 3000.f);
		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, Top, Top - FVector(0.f, 0.f, 8000.f), ECC_WorldStatic, FCollisionQueryParams(SCENE_QUERY_STAT(FNAdd), false)))
		{
			if (AFNMob* Mob = GetWorld()->SpawnActor<AFNMob>(Hit.ImpactPoint + FVector(0.f, 0.f, 130.f), FRotator::ZeroRotator, Params))
			{
				Mob->InitType(EFNMobType::Otrost);
			}
		}
	}
}

float AFNPerunBoss::CooldownFor() const
{
	switch (Phase)
	{
	case 1: return FMath::FRandRange(1.2f, 2.0f);
	case 2: return FMath::FRandRange(0.9f, 1.6f);
	default: return FMath::FRandRange(0.6f, 1.2f);
	}
}

void AFNPerunBoss::StartAttack(AFNCharacter* Target)
{
	++AttackCount;
	const float Dist = FVector::Dist2D(Target->GetActorLocation(), GetActorLocation());
	const int32 JudgmentEvery = Phase >= 3 ? 3 : 4;

	if (Phase >= 2 && AttackCount % JudgmentEvery == 0)
	{
		CurrentAttack = EFNBossAttack::Judgment;
	}
	else
	{
		CurrentAttack = Dist < SlamRadius * 1.3f ? EFNBossAttack::Slam : EFNBossAttack::Bolts;
	}

	GetCharacterMovement()->StopMovementImmediately();
	ActiveTelegraphs.Reset();

	switch (CurrentAttack)
	{
	case EFNBossAttack::Slam:
		EnterState(EFNBossState::Windup, SlamDelay);
		PlayMontage(SlamMontage, SlamDelay + 0.3f);
		SpawnTelegraph(GetActorLocation(), SlamRadius, SlamDelay, SlamDamage, false, SlamAmber);
		SetParryWindow(true);
		break;

	case EFNBossAttack::Bolts:
	{
		const int32 Strikes = Phase >= 3 ? 4 : 3;
		EnterState(EFNBossState::Windup, BoltDelay + 0.45f * (Strikes - 1));
		PlayMontage(BoltsMontage, 0.f);
		SpawnTelegraph(Target->GetActorLocation(), BoltRadius, BoltDelay, BoltDamage, false, BoltViolet);
		BoltTimers.SetNum(Strikes - 1);
		for (int32 i = 0; i < Strikes - 1; ++i)
		{
			GetWorldTimerManager().SetTimer(BoltTimers[i], FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				if (State == EFNBossState::Windup)
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
		EnterState(EFNBossState::Windup, JudgmentDelay);
		SpawnTelegraph(GetActorLocation(), JudgmentRadius, JudgmentDelay, JudgmentDamage, true, DeadlyRed);
		SetPose(true);
		ImpulseRemaining = 0.15f; // one short silhouette impulse, not a strobe
		SetTint(FLinearColor(4.f, 4.f, 4.f));
		SetOverlay(&FLinearColor::White);
		PlayMontage(JudgmentMontage, JudgmentDelay + 0.3f);
		SetParryWindow(true);
		break;
	}
}

void AFNPerunBoss::TryParry()
{
	if (!bParryWindow || State != EFNBossState::Windup)
	{
		return;
	}
	CancelAttack();
	Health->ApplyDamage(60.f, nullptr);
	EnterState(EFNBossState::Stagger, StaggerTime);
	if (AFNCharacter* T = FindTarget())
	{
		T->ShowMessage(TEXT("Парирование!"));
	}
}

void AFNPerunBoss::OnPlayerFell(AFNCharacter* Player)
{
	if (!IsFightActive())
	{
		return;
	}
	// Outcome A: he stops the fight. The line depends on how far the pupil got (bosses.md §2, foundation §6.1).
	CancelAttack();
	FallenPlayer = Player;
	Health->bInvulnerable = true;
	const bool bNear = Phase >= 3 && bDamagedInFinalPhase;
	Say(Perun, bNear
		? TEXT("Стой. Хватит. …Не сказывали, чтобы Перун бил ученика до конца. Значит, и я не буду.")
		: TEXT("Встань. Сказал — сделай: ты сказал, что дойдёшь. Иди. Дальше — не я."), 7.f);
	EnterState(EFNBossState::Stopped, 7.5f);
}

void AFNPerunBoss::HandleDeath(AActor* /*Killer*/)
{
	// Outcome B: the rare win. He cracks along the grain; no power for the player, only a trace and a line.
	CancelAttack();
	EnterState(EFNBossState::Beaten, 7.f);
	Say(Perun, TEXT("Годно. Про такое тоже не сказывали."), 6.f);
	if (bHasSkeletalVisual && DeathAnim)
	{
		GetMesh()->PlayAnimation(DeathAnim, false);
	}
	else
	{
		Body->SetRelativeRotation(FRotator(0.f, 0.f, 12.f));
	}
	GetCharacterMovement()->StopMovementImmediately();
	if (AFNCharacter* T = FindTarget())
	{
		T->GiveTrace();
	}
}

void AFNPerunBoss::BeginEpilogue()
{
	// Both outcomes: the mentor walks the pupil to the portal on the ridge.
	bool bChapter = false;
	for (TActorIterator<AFNVysiGreybox> It(GetWorld()); It; ++It) { bChapter = true; break; }

	Health->bInvulnerable = true;
	Health->Health = FMath::Max(Health->Health, 1.f);
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	Body->SetRelativeRotation(FRotator::ZeroRotator);

	// On the ridge path next to the oak (the ridge is ~6 m wide: stay on its axis), snapped to the ground.
	FVector Spot = bChapter ? FVector(872.f, 2.f, 150.f) * M : HomeLocation;
	if (bChapter)
	{
		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, Spot, Spot - FVector(0.f, 0.f, 10000.f), ECC_WorldStatic, FCollisionQueryParams(SCENE_QUERY_STAT(FNEpilogue), false, this)))
		{
			Spot = Hit.ImpactPoint + FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 5.f);
		}
	}
	SetActorLocation(Spot, false, nullptr, ETeleportType::ResetPhysics);
	GetCharacterMovement()->StopMovementImmediately();
	SetActorRotation(FRotator(0.f, 180.f, 0.f));

	if (FallenPlayer.IsValid())
	{
		FallenPlayer->ReviveAt(bChapter ? FVector(748.f, 0.f, 92.f) * M : HomeLocation + FVector(-1500.f, 0.f, 0.f));
	}
	Say(Perun, TEXT("Пойдём. Провожу до края."), 5.f);
	EpilogueStep = 0;
	EpilogueClock = 0.f;
	EnterState(EFNBossState::Epilogue, 0.f);
}

void AFNPerunBoss::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	StateTime += DeltaSeconds;

	if (ImpulseRemaining > 0.f)
	{
		ImpulseRemaining -= DeltaSeconds;
		if (ImpulseRemaining <= 0.f)
		{
			SetTint(DeadlyRed * 0.8f);
			SetOverlay(nullptr);
		}
	}

	AFNCharacter* Target = FindTarget();

	switch (State)
	{
	case EFNBossState::Idle:
		if (Target && FVector::Dist2D(Target->GetActorLocation(), HomeLocation) < ActivationRadius)
		{
			// He turns to face the pupil: the exam begins.
			SetActorRotation(FRotator(0.f, (Target->GetActorLocation() - GetActorLocation()).Rotation().Yaw, 0.f));
			Say(Perun, TEXT("Сказал — сделай. Замахнулся — ударь. Покажи, чему научился."), 4.f);
			EnterState(EFNBossState::Intro, 3.5f);
		}
		return;

	case EFNBossState::Stopped:
	case EFNBossState::Beaten:
		if (StateTime >= StateDuration)
		{
			BeginEpilogue();
		}
		return;

	case EFNBossState::Epilogue:
		if (Target)
		{
			const FVector To = Target->GetActorLocation() - GetActorLocation();
			SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(0.f, To.Rotation().Yaw, 0.f), DeltaSeconds, 3.f));
			if (EpilogueStep == 0 && To.Size2D() < 2500.f)
			{
				EpilogueStep = 1;
				EpilogueClock = 0.f;
				Say(TEXT("Ты"), TEXT("Скоро будет дождь?"), 3.f);
			}
		}
		EpilogueClock += DeltaSeconds;
		if (EpilogueStep == 1 && EpilogueClock > 3.2f)
		{
			EpilogueStep = 2;
			EpilogueClock = 0.f;
			Say(Perun, TEXT("…Не знаю. Про это не сказывали."), 5.f);
		}
		else if (EpilogueStep == 2 && EpilogueClock > 5.5f)
		{
			EpilogueStep = 3;
			bEndCard = true;
		}
		return;

	default:
		break;
	}

	if (!Target)
	{
		return;
	}

	// Track damage taken in the final phase ("almost won" = reached phase 3 and hurt him there).
	if (Phase >= 3 && Health->Health < LastHealth - 0.5f)
	{
		bDamagedInFinalPhase = true;
	}
	LastHealth = Health->Health;

	// Phase gates.
	const float Ratio = Health->GetRatio();
	const int32 Desired = Ratio <= 0.33f ? 3 : (Ratio <= 0.66f ? 2 : 1);
	if (Desired > Phase && State != EFNBossState::Transition)
	{
		CancelAttack();
		Phase = Desired;
		Health->bInvulnerable = true;
		Say(Perun, Phase == 2 ? TEXT("Не стой. Гром не ждёт.") : TEXT("Сказал — сделай!"), 3.f);
		SpawnAdds(Phase == 2 ? 2 : 3);
		EnterState(EFNBossState::Transition, 1.8f);
		return;
	}

	const FVector ToTarget = Target->GetActorLocation() - GetActorLocation();

	switch (State)
	{
	case EFNBossState::Intro:
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(0.f, ToTarget.Rotation().Yaw, 0.f), DeltaSeconds, 4.f));
		if (StateTime >= StateDuration)
		{
			EnterState(EFNBossState::Chase, 0.f);
			Cooldown = 1.f;
		}
		break;

	case EFNBossState::Transition:
		if (StateTime >= StateDuration)
		{
			Health->bInvulnerable = false;
			EnterState(EFNBossState::Chase, 0.f);
			Cooldown = 0.8f;
		}
		break;

	case EFNBossState::Chase:
	{
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(0.f, ToTarget.Rotation().Yaw, 0.f), DeltaSeconds, 5.f));
		if (ToTarget.Size2D() > SlamRadius * 0.7f && FVector::Dist2D(GetActorLocation(), HomeLocation) < ActivationRadius)
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
			SetParryWindow(false);
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
			EnterState(EFNBossState::Recover, RecoverTime);
		}
		break;

	case EFNBossState::Recover:
	case EFNBossState::Stagger:
		if (StateTime >= StateDuration)
		{
			EnterState(EFNBossState::Chase, 0.f);
			Cooldown = CooldownFor();
		}
		break;

	default:
		break;
	}
}
