#include "FNCharacter.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "AnimationRuntime.h"
#include "Engine/SkeletalMesh.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "EngineUtils.h"
#include "FNGameMode.h"
#include "FNMob.h"
#include "FNPerunBoss.h"
#include "FNSkillTree.h"
#include "Kismet/GameplayStatics.h"
#include "FNVysiGreybox.h"
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
	Tree = CreateDefaultSubobject<UFNSkillTree>(TEXT("Tree"));

	// Spark form: a plasma ember with its own light (the only neon allowed: style_v0.1).
	SparkOrb = MakePart(TEXT("SparkOrb"), Sphere.Object, FVector(0.f, 0.f, 20.f), FVector(0.45f));
	SparkOrb->SetCastShadow(false);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMat(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (ShapeMat.Succeeded())
	{
		SparkOrb->SetMaterial(0, ShapeMat.Object); // the engine sphere ships with a grid material without a Color parameter
		Head->SetMaterial(0, ShapeMat.Object);
	}
	SparkLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("SparkLight"));
	SparkLight->SetupAttachment(RootComponent); // stays with the body: bright in the Spark, a dim ember in the Skeleton
	SparkLight->SetRelativeLocation(FVector(0.f, 0.f, 30.f));
	SparkLight->SetLightColor(SparkColor);
	SparkLight->SetAttenuationRadius(900.f);
	SparkLight->SetIntensity(12000.f);
	SparkLight->SetCastShadows(false);

	// Temporary visuals: Paragon Wraith (Epic, free for UE projects — see docs/tech/assets_licenses.csv).
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> WraithMesh(TEXT("/Game/ParagonWraith/Characters/Heroes/Wraith/Meshes/Wraith.Wraith"));
	static ConstructorHelpers::FClassFinder<UAnimInstance> WraithAnim(TEXT("/Game/ParagonWraith/Characters/Heroes/Wraith/Wraith_AnimBlueprint"));
	static ConstructorHelpers::FObjectFinder<UAnimMontage> WraithFire(TEXT("/Game/ParagonWraith/Characters/Heroes/Wraith/Animations/Fire_A_Slow_Montage.Fire_A_Slow_Montage"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> WraithDeath(TEXT("/Game/ParagonWraith/Characters/Heroes/Wraith/Animations/Death_Forward.Death_Forward"));
	if (WraithMesh.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(WraithMesh.Object);
		GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -90.f), FRotator(0.f, -90.f, 0.f));
		if (WraithAnim.Succeeded())
		{
			GetMesh()->SetAnimInstanceClass(WraithAnim.Class);
		}
		Body->SetVisibility(false);
		Head->SetVisibility(false);
		Gun->SetVisibility(false); // kept as the muzzle reference point
	}
	SkeletonMesh = CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("SkeletonMesh"));
	SkeletonMesh->SetupAttachment(GetCapsuleComponent());
	SkeletonMesh->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -90.f), FRotator(0.f, -90.f, 0.f));
	SkeletonMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkeletonMesh->SetVisibility(false);
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> SkelAsset(TEXT("/Game/Characters/Skeleton/SK_HeroSkeleton.SK_HeroSkeleton"));
	if (SkelAsset.Succeeded()) { SkeletonMesh->SetSkinnedAssetAndUpdate(SkelAsset.Object); }
	// The Wraith keeps animating while hidden: it drives the skeleton.
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	FireMontage = WraithFire.Object;
	DeathAnim = WraithDeath.Object;
}

void AFNCharacter::BeginPlay()
{
	Super::BeginPlay();
	Ammo = MagazineSize;
	Stamina = MaxStamina;
	DefaultWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
	Health->OnDeath.AddDynamic(this, &AFNCharacter::HandleDeath);
	LastSafeLocation = GetActorLocation();
	Checkpoint = GetActorLocation();
	if (UMaterialInstanceDynamic* MID = SparkOrb->CreateDynamicMaterialInstance(0)) { MID->SetVectorParameterValue(TEXT("Color"), SparkColor); }
	InitRetarget();
	SparkLight->SetLightColor(SparkColor);

	// The chapter starts as a Spark; the flat boss arena (?Arena) starts as full Flesh with the rifle.
	bool bChapter = false;
	for (TActorIterator<AFNVysiGreybox> It(GetWorld()); It; ++It) { bChapter = true; break; }
	if (bChapter)
	{
		SetStage(EFNStage::Spark, false);
		ShowMessage(TEXT("Ты — Искра. Пять побед — и обретёшь остов."));
		if (FParse::Param(FCommandLine::Get(), TEXT("StartSkeleton"))) { SetStage(EFNStage::Skeleton, false); } // test key
	}
	else
	{
		SetStage(EFNStage::Flesh, false);
		bHasRifle = true;
		Weapon = EFNWeapon::Rifle;
	}

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
	Mapping->MapKey(RestartAction, EKeys::Enter);
	Weapon1Action = MakeAction(EInputActionValueType::Boolean);
	Weapon2Action = MakeAction(EInputActionValueType::Boolean);
	Weapon3Action = MakeAction(EInputActionValueType::Boolean);
	// Weapons: mouse wheel and Q (keys 1-4 are ability slots).
	Mapping->MapKey(Weapon1Action, EKeys::MouseScrollUp);
	Mapping->MapKey(Weapon2Action, EKeys::MouseScrollDown);
	Mapping->MapKey(Weapon3Action, EKeys::Q);
	const FKey AbilityKeys[4] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four };
	for (int32 i = 0; i < 4; ++i)
	{
		AbilityActions[i] = MakeAction(EInputActionValueType::Boolean);
		Mapping->MapKey(AbilityActions[i], AbilityKeys[i]);
	}
	TreeAction = MakeAction(EInputActionValueType::Boolean);
	TreeAction->bTriggerWhenPaused = true;
	Mapping->MapKey(TreeAction, EKeys::Tab);
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
	Input->BindAction(RestartAction, ETriggerEvent::Started, this, &AFNCharacter::OnRestart);
	Input->BindAction(Weapon1Action, ETriggerEvent::Started, this, &AFNCharacter::OnWeapon1);
	Input->BindAction(Weapon2Action, ETriggerEvent::Started, this, &AFNCharacter::OnWeapon2);
	Input->BindAction(Weapon3Action, ETriggerEvent::Started, this, &AFNCharacter::OnWeapon3);
	for (int32 i = 0; i < 4; ++i)
	{
		Input->BindAction(AbilityActions[i], ETriggerEvent::Started, this, &AFNCharacter::OnAbility, i);
	}
	Input->BindAction(TreeAction, ETriggerEvent::Started, this, &AFNCharacter::ToggleTree);
}

void AFNCharacter::OnMove(const FInputActionValue& Value)
{
	if (bDead || bRolling || !Controller || bTreeOpen)
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
	if (bTreeOpen)
	{
		return;
	}
	const FVector2D Axis = Value.Get<FVector2D>();
	const float Sensitivity = bAiming ? 0.5f : 1.f;
	AddControllerYawInput(Axis.X * Sensitivity);
	AddControllerPitchInput(-Axis.Y * Sensitivity);
}

void AFNCharacter::OnRoll()
{
	const float Cost = FMath::Max(5.f, RollCost * (1.f + TreeMods.Dodge));
	if (bDead || bRolling || Stamina < Cost || bTreeOpen)
	{
		return;
	}
	Stamina -= Cost;
	StaminaDelay = 0.8f;

	RollDirection = LastMoveInput.IsNearlyZero() ? GetActorForwardVector() : LastMoveInput;
	RollDirection.Z = 0.f;
	RollDirection.Normalize();

	if (Stage == EFNStage::Spark)
	{
		// Blink: teleport forward up to 6 m, stopping at obstacles.
		const FVector From = GetActorLocation();
		const FVector To = From + RollDirection * 600.f;
		FHitResult Hit;
		FCollisionQueryParams Q(SCENE_QUERY_STAT(FNBlink), false, this);
		const bool bBlocked = GetWorld()->SweepSingleByChannel(Hit, From, To, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(35.f), Q);
		SetActorLocation(bBlocked ? Hit.Location : To, false, nullptr, ETeleportType::TeleportPhysics);
		DrawDebugLine(GetWorld(), From, GetActorLocation(), FColor(90, 230, 255), false, 0.2f, 0, 4.f);
		IFramesRemaining = 0.2f;
		Health->bInvulnerable = true;
		return;
	}

	bRolling = true;
	RollRemaining = CurRollDuration;
	IFramesRemaining = CurRollIFrames + TreeMods.IFrames;
	Health->bInvulnerable = true;
	bWantsFire = false;
}

void AFNCharacter::OnReload()
{
	if (bDead || bReloading || Reserve <= 0 || Weapon == EFNWeapon::Plasma)
	{
		return;
	}
	if ((Weapon == EFNWeapon::Rifle && Ammo >= MagazineSize) || (Weapon == EFNWeapon::Scatter && ScatterAmmo >= 6))
	{
		return;
	}
	bReloading = true;
	ReloadRemaining = (Weapon == EFNWeapon::Scatter ? 1.2f : ReloadTime) / (1.f + TreeMods.Reload);
}

void AFNCharacter::FinishReload()
{
	bReloading = false;
	int32& Mag = Weapon == EFNWeapon::Scatter ? ScatterAmmo : Ammo;
	const int32 Size = Weapon == EFNWeapon::Scatter ? 6 : MagazineSize;
	const int32 Taken = FMath::Min(Size - Mag, Reserve);
	Mag += Taken;
	Reserve -= Taken;
}

bool AFNCharacter::AddReserveAmmo(int32 Amount)
{
	const int32 Cap = FMath::RoundToInt(MaxReserve * (1.f + TreeMods.Reserve));
	if (bDead || Reserve >= Cap)
	{
		return false;
	}
	Reserve = FMath::Min(Cap, Reserve + Amount);
	return true;
}

float AFNCharacter::GetTimeSinceHit() const
{
	return GetWorld() ? static_cast<float>(GetWorld()->GetTimeSeconds() - LastHitTime) : 100.f;
}

void AFNCharacter::FireShot()
{
	switch (Weapon)
	{
	case EFNWeapon::Plasma:
		// Weak energy bolt, no ammo (Spark/Skeleton before the first weapon).
		FireCooldown = 0.28f / TreeMods.FireRate;
		FireTrace(10.f, 1.5f, 3000.f, FColor(90, 230, 255));
		return;

	case EFNWeapon::Scatter:
		if (ScatterAmmo <= 0) { OnReload(); return; }
		--ScatterAmmo;
		FireCooldown = 0.75f / TreeMods.FireRate;
		for (int32 i = 0; i < 8; ++i)
		{
			FireTrace(9.f, 6.f, 2500.f, FColor(255, 190, 90));
		}
		break;

	case EFNWeapon::Rifle:
	default:
		if (Ammo <= 0) { OnReload(); return; }
		--Ammo;
		FireCooldown = FireInterval / TreeMods.FireRate;
		FireTrace(ShotDamage, bAiming ? AimSpreadDeg : HipSpreadDeg, 20000.f, FColor(255, 190, 90));
		break;
	}

	if (Stage == EFNStage::Flesh)
	{
		if (UAnimInstance* Anim = GetMesh()->GetAnimInstance())
		{
			if (FireMontage && !Anim->Montage_IsPlaying(FireMontage))
			{
				Anim->Montage_Play(FireMontage, 2.5f);
			}
		}
	}
}

void AFNCharacter::FireTrace(float Damage, float SpreadDeg, float Range, const FColor& Tracer)
{
	FVector ViewLoc;
	FRotator ViewRot;
	Controller->GetPlayerViewPoint(ViewLoc, ViewRot);

	const FVector Dir = FMath::VRandCone(ViewRot.Vector(), FMath::DegreesToRadians(SpreadDeg));
	const FVector End = ViewLoc + Dir * Range;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(FNShot), false, this);
	FHitResult Hit;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, ViewLoc, End, ECC_Visibility, Params);

	const FVector Muzzle = Stage == EFNStage::Spark ? SparkOrb->GetComponentLocation() : Gun->GetComponentLocation() + GetActorForwardVector() * 50.f;
	const FVector Impact = bHit ? Hit.ImpactPoint : End;
	DrawDebugLine(GetWorld(), Muzzle, Impact, Tracer, false, 0.05f, 0, 1.2f);

	if (bHit && Hit.GetComponent() && Hit.GetComponent()->ComponentHasTag(TEXT("ParryPoint")))
	{
		if (AFNPerunBoss* Boss = Cast<AFNPerunBoss>(Hit.GetActor()))
		{
			Boss->TryParry();
		}
	}
	if (bHit && Hit.GetActor())
	{
		if (UFNHealthComponent* TargetHealth = Hit.GetActor()->FindComponentByClass<UFNHealthComponent>())
		{
			const bool bWeak = Hit.GetComponent() && Hit.GetComponent()->ComponentHasTag(TEXT("WeakPoint"));
			if (TargetHealth->ApplyDamage(Damage * TreeMods.Ranged * (bWeak ? WeakPointMultiplier * (1.f + TreeMods.Weak) : 1.f), this) > 0.f)
			{
				LastHitTime = GetWorld()->GetTimeSeconds();
				bLastHitWeak = bWeak;
				// "Раскат": a weak-point shot makes the creature flinch.
				if (bWeak && TreeMods.bWeakFlinch)
				{
					if (AFNMob* Mob = Cast<AFNMob>(Hit.GetActor())) { Mob->Flinch(); }
				}
			}
		}
		DrawDebugPoint(GetWorld(), Impact, 8.f, FColor(255, 230, 160), false, 0.1f);
	}
}

void AFNCharacter::CycleWeapon(int32 Dir)
{
	if (bTreeOpen) { return; } // the wheel zooms the tree
	for (int32 Step = 1; Step <= 3; ++Step)
	{
		const EFNWeapon Next = static_cast<EFNWeapon>((static_cast<int32>(Weapon) + Dir * Step + 3) % 3);
		if (HasWeapon(Next))
		{
			SelectWeapon(Next);
			return;
		}
	}
}

void AFNCharacter::OnAbility(int32 Slot)
{
	// Skill gems are not in the demo yet: the slots show the future layout.
	ShowMessage(IsAbilitySlotOpen(Slot)
		? FString::Printf(TEXT("Слот %d пуст — камень-навык ещё не найден"), Slot + 1)
		: FString(TEXT("Слот 4 — ульта. Откроется в Нави")));
}

void AFNCharacter::SelectWeapon(EFNWeapon W)
{
	if (bDead || Stage == EFNStage::Spark || bReloading)
	{
		return;
	}
	if ((W == EFNWeapon::Rifle && !bHasRifle) || (W == EFNWeapon::Scatter && !bHasScatter))
	{
		return;
	}
	Weapon = W;
}

void AFNCharacter::GiveWeapon(EFNWeapon NewWeapon)
{
	if (NewWeapon == EFNWeapon::Rifle) { bHasRifle = true; ShowMessage(TEXT("Найдено ружьё  [2]")); }
	if (NewWeapon == EFNWeapon::Scatter) { bHasScatter = true; ShowMessage(TEXT("Найден дробовик  [3]")); }
	if (Stage != EFNStage::Spark)
	{
		Weapon = NewWeapon;
	}
}

void AFNCharacter::GiveArmor(float Bonus)
{
	ArmorBonus += Bonus;
	ApplyStats();
	Health->Health = FMath::Min(Health->MaxHealth, Health->Health + Bonus);
	ShowMessage(FString::Printf(TEXT("Доспех: +%.0f к здоровью"), Bonus));
}

void AFNCharacter::RestAtTreba(const FVector& At)
{
	Checkpoint = At;
	Health->Health = Health->MaxHealth;
	Stamina = MaxStamina;
	Reserve = FMath::Max(Reserve, 72);
	ShowMessage(TEXT("Треба. Отдых — сюда ты и вернёшься."));
}

void AFNCharacter::ShowMessage(const FString& Text)
{
	Message = Text;
	MessageTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
}

float AFNCharacter::GetMessageAge() const
{
	return GetWorld() ? static_cast<float>(GetWorld()->GetTimeSeconds() - MessageTime) : 100.f;
}

void AFNCharacter::SetStage(EFNStage NewStage, bool bAnnounce)
{
	Stage = NewStage;
	const bool bHasSkin = GetMesh()->GetSkeletalMeshAsset() != nullptr;

	float BaseHealth = 100.f;
	float Speed = 500.f;
	switch (Stage)
	{
	case EFNStage::Spark:
		BaseHealth = 20.f; Speed = 650.f;
		Weapon = EFNWeapon::Plasma;
		break;
	case EFNStage::Skeleton:
		BaseHealth = 50.f; Speed = 540.f;
		CurRollSpeed = 1700.f; CurRollDuration = 0.28f; CurRollIFrames = 0.12f;
		if (bHasRifle) { Weapon = EFNWeapon::Rifle; }
		break;
	case EFNStage::Flesh:
		BaseHealth = 100.f; Speed = 500.f;
		CurRollSpeed = RollSpeed; CurRollDuration = RollDuration; CurRollIFrames = RollIFrames;
		if (bHasRifle && Weapon == EFNWeapon::Plasma) { Weapon = EFNWeapon::Rifle; }
		break;
	}

	// Glow fades with the evolution: bright plasma -> dim ember in the bones -> none (GDD §4).
	SparkLight->SetIntensity(Stage == EFNStage::Spark ? 12000.f : (Stage == EFNStage::Skeleton ? 4000.f : (bHasTrace ? 2500.f : 0.f)));
	SparkLight->SetLightColor(Stage == EFNStage::Flesh && bHasTrace ? FLinearColor(0.8f, 0.85f, 0.95f) : SparkColor);
	SparkLight->SetVisibility(Stage != EFNStage::Flesh || bHasTrace);

	// Visuals: ember -> pale bone frame -> full body.
	SparkOrb->SetVisibility(Stage == EFNStage::Spark, true);
	GetMesh()->SetVisibility(Stage == EFNStage::Flesh && bHasSkin);
	const bool bBones = Stage == EFNStage::Skeleton && bHasSkin && SkeletonMesh->GetSkinnedAsset() && RetargetBones.Num() > 0;
	SkeletonMesh->SetVisibility(bBones);
	if (bBones && FParse::Param(FCommandLine::Get(), TEXT("ShowWraith"))) { GetMesh()->SetVisibility(true); } // debug: compare poses
	const bool bFrame = (Stage == EFNStage::Skeleton && !bBones) || (Stage == EFNStage::Flesh && !bHasSkin);
	Body->SetVisibility(bFrame);
	Head->SetVisibility(bFrame);
	if (Stage == EFNStage::Skeleton)
	{
		Body->SetRelativeScale3D(FVector(0.3f, 0.3f, 1.6f));
		for (UStaticMeshComponent* C : { Body.Get(), Head.Get() })
		{
			if (UMaterialInstanceDynamic* MID = C->CreateDynamicMaterialInstance(0)) { MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.85f, 0.82f, 0.72f)); }
		}
	}

	StageBaseHealth = BaseHealth;
	StageBaseSpeed = Speed;
	ApplyStats();
	Health->Health = Health->MaxHealth;

	if (bAnnounce)
	{
		Health->bInvulnerable = true;
		IFramesRemaining = 1.5f;
		ShowMessage(Stage == EFNStage::Skeleton ? TEXT("Искра собирает кости.  (рывок)") : TEXT("Плоть вернулась.  (перекат, удар, полное здоровье)"));
	}
}

int32 AFNCharacter::GetKills() const
{
	const AFNGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AFNGameMode>() : nullptr;
	return GM ? GM->GetKills() : 0;
}

int32 AFNCharacter::GetSkillPoints() const
{
	const AFNGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AFNGameMode>() : nullptr;
	const int32 Earned = GM ? GM->GetKills() / 2 : 0; // 1 point per 2 kills [D]
	return Earned - Tree->GetSpent();
}

void AFNCharacter::TryAllocate(int32 Node)
{
	if (Tree->Allocate(Node, GetSkillPoints()))
	{
		ApplyStats();
		ShowMessage(FString::Printf(TEXT("Изучено: %s"), *UFNSkillTree::Nodes()[Node].Name));
	}
}

void AFNCharacter::TryRefund(int32 Node)
{
	if (Tree->Refund(Node) > 0)
	{
		ApplyStats();
	}
}

void AFNCharacter::FindRune(int32 Node)
{
	Tree->FindRune(Node);
	ShowMessage(FString::Printf(TEXT("Руна-ключ: %s  (Tab)"), *UFNSkillTree::Nodes()[Node].Name));
}

void AFNCharacter::ApplyStats()
{
	const FFNTreeStats S = Tree->ComputeStats();
	TreeMods.Ranged = S.Ranged();
	TreeMods.FireRate = S.FireRate();
	TreeMods.Weak = S.WeakInc;
	TreeMods.Reload = S.ReloadInc;
	TreeMods.Reserve = S.ReserveInc;
	TreeMods.Melee = S.MeleeInc;
	TreeMods.MeleeHeal = S.MeleeHeal;
	TreeMods.Stamina = S.StaminaRegenInc;
	TreeMods.Dodge = S.DodgeCostInc;
	TreeMods.IFrames = S.RollIFramesFlat;
	TreeMods.bWeakFlinch = S.bWeakFlinch;

	const float OldMax = FMath::Max(1.f, Health->MaxHealth);
	const float Ratio = Health->Health / OldMax;
	Health->MaxHealth = FMath::Max(1.f, (StageBaseHealth + ArmorBonus + S.MaxHPFlat) * (1.f + S.MaxHPInc));
	Health->Health = FMath::Clamp(Ratio * Health->MaxHealth, 0.f, Health->MaxHealth);
	Health->IncomingMultiplier = S.DamageTaken();

	DefaultWalkSpeed = StageBaseSpeed * (1.f + S.MoveInc);
	GetCharacterMovement()->MaxWalkSpeed = DefaultWalkSpeed;
}

void AFNCharacter::ToggleTree()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || bDead)
	{
		return;
	}
	bTreeOpen = !bTreeOpen;
	bWantsFire = false;
	// Time nearly stops (not a hard pause, so HUD hit boxes and input keep working).
	UGameplayStatics::SetGlobalTimeDilation(this, bTreeOpen ? 0.02f : 1.f);
	PC->bShowMouseCursor = bTreeOpen;
	PC->bEnableClickEvents = bTreeOpen;
	PC->bEnableMouseOverEvents = bTreeOpen;
	if (bTreeOpen)
	{
		FInputModeGameAndUI Mode;
		Mode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(Mode);
	}
	else
	{
		PC->SetInputMode(FInputModeGameOnly());
	}
}

void AFNCharacter::ReviveAt(const FVector& At)
{
	Checkpoint = At;
	Revive();
	MessageTime = -100.0; // no "rekindles" line after the exam
}

void AFNCharacter::GiveTrace()
{
	bHasTrace = true;
	SetStage(Stage, false);
	ShowMessage(TEXT("След наставника: серебряный отсвет (без силы)"));
}

void AFNCharacter::Revive()
{
	bDead = false;
	bDiedInArena = false;
	RespawnTimer = -1.f;
	SetActorLocation(Checkpoint, false, nullptr, ETeleportType::ResetPhysics);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	Body->SetRelativeRotation(FRotator::ZeroRotator);
	SetStage(Stage, false);
	Stamina = MaxStamina;
	LastSafeLocation = Checkpoint;
	ShowMessage(TEXT("Искра разгорается вновь."));
}

void AFNCharacter::OnMelee()
{
	if (bDead || bRolling || bTreeOpen || MeleeCooldown > 0.f)
	{
		return;
	}

	// Per stage: Spark = flash around the ember, Skeleton = a bare-hand blow, Flesh = melee weapon [D].
	float Damage = 45.f, Radius = 180.f, Reach = 170.f, Cost = MeleeCost, Cooldown = 0.7f;
	FColor Tint(200, 200, 200);
	switch (Stage)
	{
	case EFNStage::Spark:    Damage = 15.f; Radius = 260.f; Reach = 0.f;   Cost = 10.f; Cooldown = 0.8f; Tint = FColor(90, 230, 255); break;
	case EFNStage::Skeleton: Damage = 25.f; Radius = 120.f; Reach = 140.f; Cost = 12.f; Cooldown = 0.45f; Tint = FColor(230, 225, 205); break;
	case EFNStage::Flesh:    Damage = MeleeDamage + 5.f; break;
	}
	if (Stamina < Cost)
	{
		return;
	}
	Stamina -= Cost;
	StaminaDelay = 0.8f;
	MeleeCooldown = Cooldown;

	const FVector Center = GetActorLocation() + GetActorForwardVector() * Reach;
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(FNMelee), false, this);
	GetWorld()->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(Radius), Params);

	TSet<AActor*> Damaged;
	for (const FOverlapResult& O : Overlaps)
	{
		AActor* A = O.GetActor();
		if (A && A != this && !Damaged.Contains(A))
		{
			Damaged.Add(A);
			if (UFNHealthComponent* H = A->FindComponentByClass<UFNHealthComponent>())
			{
				if (H->ApplyDamage(Damage * (1.f + TreeMods.Melee), this) > 0.f && TreeMods.MeleeHeal > 0.f)
				{
					Health->Health = FMath::Min(Health->MaxHealth, Health->Health + TreeMods.MeleeHeal);
				}
				LastHitTime = GetWorld()->GetTimeSeconds();
				bLastHitWeak = false;
			}
		}
	}
	DrawDebugSphere(GetWorld(), Center, Radius, 16, Tint, false, 0.15f);
	if (Stage == EFNStage::Spark)
	{
		MeleeFlash = 0.12f; // the ember flares
		SparkLight->SetIntensity(60000.f);
	}
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
	// On the exam arena (or the flat test arena) a fall is an exam outcome; elsewhere the Spark returns to the treba.
	bool bChapter = false;
	for (TActorIterator<AFNVysiGreybox> It(GetWorld()); It; ++It) { bChapter = true; break; }
	bDiedInArena = !bChapter || FVector::Dist2D(GetActorLocation(), AFNVysiGreybox::ArenaCenter()) < 2600.f;
	RespawnTimer = bDiedInArena ? -1.f : 3.f;
	if (bDiedInArena)
	{
		for (TActorIterator<AFNPerunBoss> It(GetWorld()); It; ++It)
		{
			It->OnPlayerFell(this);
		}
	}
	GetCharacterMovement()->DisableMovement();
	if (DeathAnim && GetMesh()->GetSkeletalMeshAsset())
	{
		GetMesh()->PlayAnimation(DeathAnim, false);
	}
	else
	{
		Body->SetRelativeRotation(FRotator(80.f, 0.f, 0.f)); // grey-box "fallen" pose
	}
}

void AFNCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateRetarget();
	// Test key "-PoseShot": stand, then run sideways past the camera, screenshot both, quit.
	if (FParse::Param(FCommandLine::Get(), TEXT("PoseShot")))
	{
		const float T = GetWorld()->GetTimeSeconds();
		static int32 Taken = 0;
		if (T > 12.f && T < 16.f) { AddMovementInput(GetActorRightVector(), 1.f); }
		if (T > 10.f && Taken == 0) { Taken = 1; FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/20_stand.png"), false, false); }
		if (T > 14.f && Taken == 1) { Taken = 2; FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/21_run.png"), false, false); }
		if (T > 17.f && Taken == 2) { Taken = 3; Cast<APlayerController>(GetController())->ConsoleCommand(TEXT("quit")); }
	}

	if (bDead)
	{
		if (RespawnTimer > 0.f)
		{
			RespawnTimer -= DeltaSeconds;
			if (RespawnTimer <= 0.f)
			{
				Revive();
			}
		}
		return;
	}

	// Evolution by kills (Spark -> Skeleton -> Flesh).
	if (const AFNGameMode* GM = GetWorld()->GetAuthGameMode<AFNGameMode>())
	{
		if (Stage == EFNStage::Spark && GM->GetKills() >= KillsToSkeleton) { SetStage(EFNStage::Skeleton, true); }
		else if (Stage == EFNStage::Skeleton && GM->GetKills() >= KillsToFlesh) { SetStage(EFNStage::Flesh, true); }
	}
	if (IFramesRemaining > 0.f && !bRolling)
	{
		IFramesRemaining -= DeltaSeconds;
		if (IFramesRemaining <= 0.f) { Health->bInvulnerable = false; }
	}

	// Grey-box safety net: remember safe ground, return there after falling off the path.
	SafeTimer -= DeltaSeconds;
	if (GetCharacterMovement()->IsMovingOnGround() && SafeTimer <= 0.f)
	{
		LastSafeLocation = GetActorLocation();
		SafeTimer = 1.f;
	}
	if (GetActorLocation().Z < LastSafeLocation.Z - 3000.f)
	{
		SetActorLocation(LastSafeLocation, false, nullptr, ETeleportType::ResetPhysics);
		GetCharacterMovement()->StopMovementImmediately();
	}

	// Roll: fixed-velocity dash, i-frames at the start.
	if (bRolling)
	{
		RollRemaining -= DeltaSeconds;
		IFramesRemaining -= DeltaSeconds;
		const FVector V = GetCharacterMovement()->Velocity;
		GetCharacterMovement()->Velocity = FVector(RollDirection.X * CurRollSpeed, RollDirection.Y * CurRollSpeed, V.Z);
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
		Stamina = FMath::Min(MaxStamina, Stamina + StaminaRegen * (1.f + TreeMods.Stamina) * DeltaSeconds);
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
	if (bWantsFire && !bRolling && !bReloading && FireCooldown <= 0.f && Controller && !bTreeOpen)
	{
		FireShot();
	}

	// Melee timers and the Spark's flare.
	MeleeCooldown = FMath::Max(0.f, MeleeCooldown - DeltaSeconds);
	if (MeleeFlash > 0.f)
	{
		MeleeFlash -= DeltaSeconds;
		if (MeleeFlash <= 0.f && Stage == EFNStage::Spark) { SparkLight->SetIntensity(12000.f); }
	}

	// Startle at thunder: short camera jolt.
	if (FlinchRemaining > 0.f)
	{
		FlinchRemaining -= DeltaSeconds;
		const float A = FMath::Max(0.f, FlinchRemaining) * 30.f;
		Camera->SetRelativeLocation(FVector(0.f, FMath::FRandRange(-A, A), FMath::FRandRange(-A, A) - A));
		if (FlinchRemaining <= 0.f) { Camera->SetRelativeLocation(FVector::ZeroVector); }
	}

	// Aim: tighter camera, slower walk.
	const float TargetFOV = bAiming ? 60.f : 90.f;
	const float TargetArm = bAiming ? 190.f : 320.f;
	Camera->SetFieldOfView(FMath::FInterpTo(Camera->FieldOfView, TargetFOV, DeltaSeconds, 12.f));
	Boom->TargetArmLength = FMath::FInterpTo(Boom->TargetArmLength, TargetArm, DeltaSeconds, 12.f);
	GetCharacterMovement()->MaxWalkSpeed = bAiming ? DefaultWalkSpeed * 0.6f : DefaultWalkSpeed;
}

void AFNCharacter::InitRetarget()
{
	RetargetBones.Reset();
	const USkeletalMesh* SrcMesh = GetMesh()->GetSkeletalMeshAsset();
	const USkeletalMesh* DstMesh = Cast<USkeletalMesh>(SkeletonMesh->GetSkinnedAsset());
	if (!SrcMesh || !DstMesh)
	{
		return;
	}
	// Parent-before-child order: world-space sets depend on the parent already being posed.
	static const TCHAR* Map[][2] = {
		{ TEXT("pelvis"), TEXT("Hips") }, { TEXT("spine_01"), TEXT("Spine") }, { TEXT("spine_02"), TEXT("Spine1") }, { TEXT("spine_03"), TEXT("Spine2") },
		{ TEXT("neck_01"), TEXT("Neck") }, { TEXT("head"), TEXT("Head") },
		{ TEXT("clavicle_l"), TEXT("LeftShoulder") }, { TEXT("upperarm_l"), TEXT("LeftArm") }, { TEXT("lowerarm_l"), TEXT("LeftForeArm") }, { TEXT("hand_l"), TEXT("LeftHand") },
		{ TEXT("clavicle_r"), TEXT("RightShoulder") }, { TEXT("upperarm_r"), TEXT("RightArm") }, { TEXT("lowerarm_r"), TEXT("RightForeArm") }, { TEXT("hand_r"), TEXT("RightHand") },
		{ TEXT("thigh_l"), TEXT("LeftUpLeg") }, { TEXT("calf_l"), TEXT("LeftLeg") }, { TEXT("foot_l"), TEXT("LeftFoot") }, { TEXT("ball_l"), TEXT("LeftToeBase") },
		{ TEXT("thigh_r"), TEXT("RightUpLeg") }, { TEXT("calf_r"), TEXT("RightLeg") }, { TEXT("foot_r"), TEXT("RightFoot") }, { TEXT("ball_r"), TEXT("RightToeBase") },
	};
	const FReferenceSkeleton& SrcRef = SrcMesh->GetRefSkeleton();
	const FReferenceSkeleton& DstRef = DstMesh->GetRefSkeleton();
	auto RefCS = [](const FReferenceSkeleton& R, int32 I) { return FAnimationRuntime::GetComponentSpaceTransformRefPose(R, I); };

	for (const auto& M : Map)
	{
		FRetargetBone B;
		B.Src = M[0];
		B.Dst = FName(M[1]); // the importer strips the "mixamorig:" namespace
		B.SrcIdx = SrcRef.FindBoneIndex(B.Src);
		B.DstIdx = DstRef.FindBoneIndex(B.Dst);
		if (B.SrcIdx == INDEX_NONE || B.DstIdx == INDEX_NONE)
		{
			UE_LOG(LogTemp, Warning, TEXT("Retarget: missing %s -> %s"), *B.Src.ToString(), *B.Dst.ToString());
			continue;
		}
		RetargetBones.Add(B);
	}
	// Rest-pose alignment (A-pose vs T-pose), in the Wraith component's space (both meshes ride the capsule rigidly).
	const FTransform DstToSrc = SkeletonMesh->GetRelativeTransform().GetRelativeTransform(GetMesh()->GetRelativeTransform());
	for (int32 i = 0; i < RetargetBones.Num(); ++i)
	{
		FRetargetBone& B = RetargetBones[i];
		for (int32 j = i + 1; j < RetargetBones.Num(); ++j)
		{
			const FRetargetBone& C = RetargetBones[j];
			if (SrcRef.GetParentIndex(C.SrcIdx) == B.SrcIdx && B.Src != TEXT("pelvis") && B.Src != TEXT("spine_03"))
			{
				const FVector SrcDir = (RefCS(SrcRef, C.SrcIdx).GetLocation() - RefCS(SrcRef, B.SrcIdx).GetLocation()).GetSafeNormal();
				const FVector DstDir = DstToSrc.TransformVector(RefCS(DstRef, C.DstIdx).GetLocation() - RefCS(DstRef, B.DstIdx).GetLocation()).GetSafeNormal();
				B.Align = FQuat::FindBetweenNormals(DstDir, SrcDir);
				break;
			}
		}
	}
}

void AFNCharacter::UpdateRetarget()
{
	if (!SkeletonMesh->IsVisible() || RetargetBones.Num() == 0)
	{
		return;
	}
	const FReferenceSkeleton& SrcRef = GetMesh()->GetSkeletalMeshAsset()->GetRefSkeleton();
	const FReferenceSkeleton& DstRef = SkeletonMesh->GetSkinnedAsset()->GetRefSkeleton();
	const TArray<FTransform>& SrcPose = GetMesh()->GetComponentSpaceTransforms();
	const FTransform SrcC = GetMesh()->GetComponentTransform();
	const FTransform DstToSrc = SkeletonMesh->GetRelativeTransform().GetRelativeTransform(GetMesh()->GetRelativeTransform());
	for (const FRetargetBone& B : RetargetBones)
	{
		if (!SrcPose.IsValidIndex(B.SrcIdx)) { return; }
		// Everything in Wraith component space, then out to world.
		const FTransform SrcRest = FAnimationRuntime::GetComponentSpaceTransformRefPose(SrcRef, B.SrcIdx);
		const FTransform DstRest = FAnimationRuntime::GetComponentSpaceTransformRefPose(DstRef, B.DstIdx) * DstToSrc;
		const FTransform& SrcNow = SrcPose[B.SrcIdx];
		const FQuat Delta = SrcNow.GetRotation() * SrcRest.GetRotation().Inverse();
		const FQuat Rot = SrcC.GetRotation() * (Delta * B.Align * DstRest.GetRotation());
		SkeletonMesh->SetBoneRotationByName(B.Dst, Rot.Rotator(), EBoneSpaces::WorldSpace);
		if (B.Src == TEXT("pelvis"))
		{
			SkeletonMesh->SetBoneLocationByName(B.Dst, SrcC.TransformPosition(DstRest.GetLocation() + (SrcNow.GetLocation() - SrcRest.GetLocation())), EBoneSpaces::WorldSpace);
		}
	}
}
