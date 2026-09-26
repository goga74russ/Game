#include "FNCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "FNHealthComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AFNCharacter::AFNCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));

	GetCapsuleComponent()->InitCapsuleSize(40.f, 90.f);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);

	// Shooter control: character faces where the camera looks.
	bUseControllerRotationYaw = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->MaxWalkSpeed = DefaultWalkSpeed;

	Boom = CreateDefaultSubobject<USpringArmComponent>(TEXT("Boom"));
	Boom->SetupAttachment(RootComponent);
	Boom->TargetArmLength = 320.f;
	Boom->SocketOffset = FVector(0.f, 65.f, 70.f); // over the right shoulder
	Boom->bUsePawnControlRotation = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Boom, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;

	auto MakePart = [this](const TCHAR* Name, UStaticMesh* PartMesh, const FVector& Loc, const FVector& Scale)
	{
		UStaticMeshComponent* C = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		C->SetupAttachment(RootComponent);
		C->SetStaticMesh(PartMesh);
		C->SetRelativeLocation(Loc);
		C->SetRelativeScale3D(Scale);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return C;
	};
	Body = MakePart(TEXT("Body"), Cylinder.Object, FVector(0.f, 0.f, -10.f), FVector(0.7f, 0.7f, 1.6f));
	Head = MakePart(TEXT("Head"), Sphere.Object, FVector(0.f, 0.f, 75.f), FVector(0.4f));
	Gun = MakePart(TEXT("Gun"), Cube.Object, FVector(45.f, 30.f, 20.f), FVector(0.9f, 0.12f, 0.12f));

	Health = CreateDefaultSubobject<UFNHealthComponent>(TEXT("Health"));
	Health->MaxHealth = 100.f;
}

void AFNCharacter::BeginPlay()
{
	Super::BeginPlay();
	Ammo = MagazineSize;
	Stamina = MaxStamina;
	DefaultWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
	Health->OnDeath.AddDynamic(this, &AFNCharacter::HandleDeath);

	const FLinearColor Flesh(0.55f, 0.5f, 0.45f);
	for (UStaticMeshComponent* C : { Body.Get(), Head.Get() })
	{
		if (UMaterialInstanceDynamic* MID = C->CreateDynamicMaterialInstance(0)) { MID->SetVectorParameterValue(TEXT("Color"), Flesh); }
	}
	if (UMaterialInstanceDynamic* MID = Gun->CreateDynamicMaterialInstance(0)) { MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.2f, 0.13f, 0.08f)); }
}

void AFNCharacter::BuildInput()
{
	Mapping = NewObject<UInputMappingContext>(this);

	auto MakeAction = [this](EInputActionValueType Type)
	{
		UInputAction* A = NewObject<UInputAction>(this);
		A->ValueType = Type;
		return A;
	};
	MoveAction = MakeAction(EInputActionValueType::Axis2D);
	LookAction = MakeAction(EInputActionValueType::Axis2D);
	FireAction = MakeAction(EInputActionValueType::Boolean);
	AimAction = MakeAction(EInputActionValueType::Boolean);
	RollAction = MakeAction(EInputActionValueType::Boolean);
	ReloadAction = MakeAction(EInputActionValueType::Boolean);
	MeleeAction = MakeAction(EInputActionValueType::Boolean);
	RestartAction = MakeAction(EInputActionValueType::Boolean);

	auto MapMove = [this](const FKey& Key, bool bSwizzle, bool bNegate)
	{
		FEnhancedActionKeyMapping& M = Mapping->MapKey(MoveAction, Key);
		if (bSwizzle)
		{
			UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(Mapping);
			Swizzle->Order = EInputAxisSwizzle::YXZ;
			M.Modifiers.Add(Swizzle);
		}
		if (bNegate)
		{
			M.Modifiers.Add(NewObject<UInputModifierNegate>(Mapping));
		}
	};
	MapMove(EKeys::W, true, false);
	MapMove(EKeys::S, true, true);
	MapMove(EKeys::A, false, true);
	MapMove(EKeys::D, false, false);

	Mapping->MapKey(LookAction, EKeys::Mouse2D);
	Mapping->MapKey(FireAction, EKeys::LeftMouseButton);
	Mapping->MapKey(AimAction, EKeys::RightMouseButton);
	Mapping->MapKey(RollAction, EKeys::SpaceBar);
	Mapping->MapKey(ReloadAction, EKeys::R);
	Mapping->MapKey(MeleeAction, EKeys::F);
	Mapping->MapKey(RestartAction, EKeys::Enter);
}

void AFNCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (!Mapping)
	{
		BuildInput();
	}

	if (const APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(Mapping, 0);
		}
	}

	UEnhancedInputComponent* Input = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AFNCharacter::OnMove);
	Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &AFNCharacter::OnLook);
	Input->BindAction(FireAction, ETriggerEvent::Started, this, &AFNCharacter::OnFireStarted);
	Input->BindAction(FireAction, ETriggerEvent::Completed, this, &AFNCharacter::OnFireStopped);
	Input->BindAction(AimAction, ETriggerEvent::Started, this, &AFNCharacter::OnAimStarted);
	Input->BindAction(AimAction, ETriggerEvent::Completed, this, &AFNCharacter::OnAimStopped);
	Input->BindAction(RollAction, ETriggerEvent::Started, this, &AFNCharacter::OnRoll);
	Input->BindAction(ReloadAction, ETriggerEvent::Started, this, &AFNCharacter::OnReload);
	Input->BindAction(MeleeAction, ETriggerEvent::Started, this, &AFNCharacter::OnMelee);
	Input->BindAction(RestartAction, ETriggerEvent::Started, this, &AFNCharacter::OnRestart);
}

void AFNCharacter::OnMove(const FInputActionValue& Value)
{
	if (bDead || bRolling || !Controller)
	{
		return;
	}
	const FVector2D Axis = Value.Get<FVector2D>();
	const FRotator Yaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
	const FVector Forward = FRotationMatrix(Yaw).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y);
	LastMoveInput = (Forward * Axis.Y + Right * Axis.X).GetSafeNormal();
	AddMovementInput(Forward, Axis.Y);
	AddMovementInput(Right, Axis.X);
}

void AFNCharacter::OnLook(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	const float Sensitivity = bAiming ? 0.5f : 1.f;
	AddControllerYawInput(Axis.X * Sensitivity);
	AddControllerPitchInput(-Axis.Y * Sensitivity);
}

void AFNCharacter::OnRoll()
{
	if (bDead || bRolling || Stamina < RollCost)
	{
		return;
	}
	Stamina -= RollCost;
	StaminaDelay = 0.8f;

	RollDirection = LastMoveInput.IsNearlyZero() ? GetActorForwardVector() : LastMoveInput;
	RollDirection.Z = 0.f;
	RollDirection.Normalize();

	bRolling = true;
	RollRemaining = RollDuration;
	IFramesRemaining = RollIFrames;
	Health->bInvulnerable = true;
	bWantsFire = false;
}

void AFNCharacter::OnReload()
{
	if (bDead || bReloading || Ammo >= MagazineSize || Reserve <= 0)
	{
		return;
	}
	bReloading = true;
	ReloadRemaining = ReloadTime;
}

void AFNCharacter::FinishReload()
{
	bReloading = false;
	const int32 Needed = MagazineSize - Ammo;
	const int32 Taken = FMath::Min(Needed, Reserve);
	Ammo += Taken;
	Reserve -= Taken;
}

bool AFNCharacter::AddReserveAmmo(int32 Amount)
{
	if (bDead || Reserve >= MaxReserve)
	{
		return false;
	}
	Reserve = FMath::Min(MaxReserve, Reserve + Amount);
	return true;
}

float AFNCharacter::GetTimeSinceHit() const
{
	return GetWorld() ? static_cast<float>(GetWorld()->GetTimeSeconds() - LastHitTime) : 100.f;
}

void AFNCharacter::FireShot()
{
	if (Ammo <= 0)
	{
		OnReload();
		return;
	}
	--Ammo;
	FireCooldown = FireInterval;

	FVector ViewLoc;
	FRotator ViewRot;
	Controller->GetPlayerViewPoint(ViewLoc, ViewRot);

	const float Spread = FMath::DegreesToRadians(bAiming ? AimSpreadDeg : HipSpreadDeg);
	const FVector Dir = FMath::VRandCone(ViewRot.Vector(), Spread);
	const FVector End = ViewLoc + Dir * 20000.f;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(FNShot), false, this);
	FHitResult Hit;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, ViewLoc, End, ECC_Visibility, Params);

	const FVector Muzzle = Gun->GetComponentLocation() + GetActorForwardVector() * 50.f;
	const FVector Impact = bHit ? Hit.ImpactPoint : End;
	DrawDebugLine(GetWorld(), Muzzle, Impact, FColor(255, 190, 90), false, 0.05f, 0, 1.2f);

	if (bHit && Hit.GetActor())
	{
		if (UFNHealthComponent* TargetHealth = Hit.GetActor()->FindComponentByClass<UFNHealthComponent>())
		{
			const bool bWeak = Hit.GetComponent() && Hit.GetComponent()->ComponentHasTag(TEXT("WeakPoint"));
			if (TargetHealth->ApplyDamage(ShotDamage * (bWeak ? WeakPointMultiplier : 1.f), this) > 0.f)
			{
				LastHitTime = GetWorld()->GetTimeSeconds();
				bLastHitWeak = bWeak;
			}
		}
		DrawDebugPoint(GetWorld(), Impact, 8.f, FColor(255, 230, 160), false, 0.1f);
	}
}

void AFNCharacter::OnMelee()
{
	if (bDead || bRolling || Stamina < MeleeCost)
	{
		return;
	}
	Stamina -= MeleeCost;
	StaminaDelay = 0.8f;

	const FVector Center = GetActorLocation() + GetActorForwardVector() * 150.f;
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(FNMelee), false, this);
	GetWorld()->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(180.f), Params);

	TSet<AActor*> Damaged;
	for (const FOverlapResult& O : Overlaps)
	{
		AActor* A = O.GetActor();
		if (A && !Damaged.Contains(A))
		{
			Damaged.Add(A);
			if (UFNHealthComponent* H = A->FindComponentByClass<UFNHealthComponent>())
			{
				H->ApplyDamage(MeleeDamage, this);
				LastHitTime = GetWorld()->GetTimeSeconds();
				bLastHitWeak = false;
			}
		}
	}
	DrawDebugSphere(GetWorld(), Center, 180.f, 12, FColor(200, 200, 200), false, 0.15f);
}

void AFNCharacter::OnRestart()
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->RestartLevel();
	}
}

void AFNCharacter::HandleDeath(AActor* /*Killer*/)
{
	bDead = true;
	bWantsFire = false;
	bRolling = false;
	GetCharacterMovement()->DisableMovement();
	Body->SetRelativeRotation(FRotator(80.f, 0.f, 0.f)); // placeholder "fallen" pose
}

void AFNCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bDead)
	{
		return;
	}

	// Roll: fixed-velocity dash, i-frames at the start.
	if (bRolling)
	{
		RollRemaining -= DeltaSeconds;
		IFramesRemaining -= DeltaSeconds;
		const FVector V = GetCharacterMovement()->Velocity;
		GetCharacterMovement()->Velocity = FVector(RollDirection.X * RollSpeed, RollDirection.Y * RollSpeed, V.Z);
		if (IFramesRemaining <= 0.f)
		{
			Health->bInvulnerable = false;
		}
		if (RollRemaining <= 0.f)
		{
			bRolling = false;
			Health->bInvulnerable = false;
		}
	}

	// Stamina
	if (StaminaDelay > 0.f)
	{
		StaminaDelay -= DeltaSeconds;
	}
	else
	{
		Stamina = FMath::Min(MaxStamina, Stamina + StaminaRegen * DeltaSeconds);
	}

	// Reload
	if (bReloading)
	{
		ReloadRemaining -= DeltaSeconds;
		if (ReloadRemaining <= 0.f)
		{
			FinishReload();
		}
	}

	// Fire
	FireCooldown -= DeltaSeconds;
	if (bWantsFire && !bRolling && !bReloading && FireCooldown <= 0.f && Controller)
	{
		FireShot();
	}

	// Aim: tighter camera, slower walk.
	const float TargetFOV = bAiming ? 60.f : 90.f;
	const float TargetArm = bAiming ? 190.f : 320.f;
	Camera->SetFieldOfView(FMath::FInterpTo(Camera->FieldOfView, TargetFOV, DeltaSeconds, 12.f));
	Boom->TargetArmLength = FMath::FInterpTo(Boom->TargetArmLength, TargetArm, DeltaSeconds, 12.f);
	GetCharacterMovement()->MaxWalkSpeed = bAiming ? DefaultWalkSpeed * 0.6f : DefaultWalkSpeed;
}
