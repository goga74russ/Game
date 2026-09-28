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
#include "FNRite.h"
#include "FNSkills.h"
#include "TimerManager.h"
#include "Misc/App.h"
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

	// The character follows the cursor; the camera retains its world-space rotation.
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->MaxWalkSpeed = DefaultWalkSpeed;

	Boom = CreateDefaultSubobject<USpringArmComponent>(TEXT("Boom"));
	Boom->SetupAttachment(RootComponent);
	Boom->SetUsingAbsoluteRotation(true);
	Boom->SetRelativeRotation(FRotator(IsometricPitch, IsometricYaw, 0.f));
	Boom->TargetArmLength = IsometricDistance;
	Boom->TargetOffset = FVector(0.f, 0.f, 40.f);
	Boom->bUsePawnControlRotation = false;
	Boom->bDoCollisionTest = false; // fixed composition: props must not pull the camera into the hero
	Boom->bEnableCameraLag = true;
	Boom->CameraLagSpeed = 12.f;
	Boom->CameraLagMaxDistance = 100.f;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Boom, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;
	Camera->SetFieldOfView(IsometricFOV);

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
	DesiredCameraDistance = FMath::Clamp(IsometricDistance, MinCameraDistance, MaxCameraDistance);
	Boom->TargetArmLength = DesiredCameraDistance;
	Boom->SetWorldRotation(FRotator(IsometricPitch, IsometricYaw, 0.f));
	Camera->SetFieldOfView(IsometricFOV);
	Ammo = MagazineSize;
	Stamina = MaxStamina;
	DefaultWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
	Health->OnDeath.AddDynamic(this, &AFNCharacter::HandleDeath);
	LastSafeLocation = GetActorLocation();
	Checkpoint = GetActorLocation();
	if (UMaterialInstanceDynamic* MID = SparkOrb->CreateDynamicMaterialInstance(0)) { MID->SetVectorParameterValue(TEXT("Color"), SparkColor); }
	InitRetarget();
	Health->OnAvoided = [this]() { OnAttackAvoided(); };
	Health->OnShieldBroken = [this]()
	{
		// "Оберег грозы" breaks: a discharge around the hero.
		TArray<FOverlapResult> Hits;
		GetWorld()->OverlapMultiByObjectType(Hits, GetActorLocation(), FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(FNSkills::Def(EFNSkillId::StormWard).RadiusCm), FCollisionQueryParams(SCENE_QUERY_STAT(FNWard), false, this));
		TSet<AActor*> Done;
		for (const FOverlapResult& O : Hits)
		{
			AActor* A = O.GetActor();
			if (!A || A == this || Done.Contains(A)) { continue; }
			Done.Add(A);
			if (UFNHealthComponent* H = A->FindComponentByClass<UFNHealthComponent>()) { YarFromHit(H, H->ApplyDamage(FNSkills::Def(EFNSkillId::StormWard).Damage, this), A); }
		}
		DrawDebugSphere(GetWorld(), GetActorLocation(), FNSkills::Def(EFNSkillId::StormWard).RadiusCm, 20, FColor(150, 170, 255), false, 0.3f);
		ShieldTime = 0.f;
	};
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
	ZoomAction = MakeAction(EInputActionValueType::Axis1D);
	FireAction = MakeAction(EInputActionValueType::Boolean);
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

	Mapping->MapKey(ZoomAction, EKeys::MouseWheelAxis);
	Mapping->MapKey(FireAction, EKeys::LeftMouseButton);
	Mapping->MapKey(MeleeAction, EKeys::RightMouseButton);
	Mapping->MapKey(RollAction, EKeys::SpaceBar);
	Mapping->MapKey(ReloadAction, EKeys::R);
	Mapping->MapKey(RestartAction, EKeys::Enter);
	Weapon1Action = MakeAction(EInputActionValueType::Boolean);
	// The wheel zooms the camera; Q cycles the existing weapons.
	Mapping->MapKey(Weapon1Action, EKeys::Q);
	const FKey AbilityKeys[4] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four };
	for (int32 i = 0; i < 4; ++i)
	{
		AbilityActions[i] = MakeAction(EInputActionValueType::Boolean);
		Mapping->MapKey(AbilityActions[i], AbilityKeys[i]);
	}
	InteractAction = MakeAction(EInputActionValueType::Boolean);
	Mapping->MapKey(InteractAction, EKeys::E);
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
	Input->BindAction(MoveAction, ETriggerEvent::Completed, this, &AFNCharacter::OnMoveStopped);
	Input->BindAction(MoveAction, ETriggerEvent::Canceled, this, &AFNCharacter::OnMoveStopped);
	Input->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &AFNCharacter::OnZoom);
	Input->BindAction(FireAction, ETriggerEvent::Started, this, &AFNCharacter::OnFireStarted);
	Input->BindAction(FireAction, ETriggerEvent::Completed, this, &AFNCharacter::OnFireStopped);
	Input->BindAction(FireAction, ETriggerEvent::Canceled, this, &AFNCharacter::OnFireStopped);
	Input->BindAction(MeleeAction, ETriggerEvent::Started, this, &AFNCharacter::OnMelee);
	Input->BindAction(RollAction, ETriggerEvent::Started, this, &AFNCharacter::OnRoll);
	Input->BindAction(ReloadAction, ETriggerEvent::Started, this, &AFNCharacter::OnReload);
	Input->BindAction(RestartAction, ETriggerEvent::Started, this, &AFNCharacter::OnRestart);
	Input->BindAction(Weapon1Action, ETriggerEvent::Started, this, &AFNCharacter::OnWeapon1);
	for (int32 i = 0; i < 4; ++i)
	{
		Input->BindAction(AbilityActions[i], ETriggerEvent::Started, this, &AFNCharacter::OnAbility, i);
	}
	Input->BindAction(TreeAction, ETriggerEvent::Started, this, &AFNCharacter::ToggleTree);
	Input->BindAction(InteractAction, ETriggerEvent::Started, this, &AFNCharacter::OnInteract);
	ConfigureCursorInput();
}

void AFNCharacter::OnMove(const FInputActionValue& Value)
{
	if (bDead || bRolling || !Controller || bTreeOpen)
	{
		return;
	}
	const FVector2D Axis = Value.Get<FVector2D>();
	const FRotator Yaw(0.f, Boom->GetComponentRotation().Yaw, 0.f);
	const FVector Forward = FRotationMatrix(Yaw).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y);
	LastMoveInput = (Forward * Axis.Y + Right * Axis.X).GetSafeNormal();
	AddMovementInput(Forward, Axis.Y);
	AddMovementInput(Right, Axis.X);
}

void AFNCharacter::OnZoom(const FInputActionValue& Value)
{
	if (bTreeOpen) { return; } // the tree handles its own zoom
	DesiredCameraDistance = FMath::Clamp(DesiredCameraDistance - Value.Get<float>() * CameraZoomStep, MinCameraDistance, MaxCameraDistance);
}

void AFNCharacter::ConfigureCursorInput()
{
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		PC->bShowMouseCursor = true;
		PC->DefaultMouseCursor = bTreeOpen ? EMouseCursor::Default : EMouseCursor::None;
		PC->CurrentMouseCursor = PC->DefaultMouseCursor;
		PC->bEnableClickEvents = bTreeOpen;
		PC->bEnableMouseOverEvents = bTreeOpen;
		FInputModeGameAndUI Mode;
		Mode.SetHideCursorDuringCapture(false);
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
		PC->SetInputMode(Mode);
	}
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
	DodgeStartTime = GetWorld()->GetTimeSeconds();

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
	if (!HasWeapon(Weapon)) { return; } // Skeleton before the first gun: melee only
	switch (Weapon)
	{
	case EFNWeapon::Plasma:
		// Weak energy bolt, no ammo; costs 3 Yar (skills_demo §2).
		if (Yar < 3.f) { ShowMessage(TEXT("Нет Яри на выстрел — бей вспышкой")); FireCooldown = 0.5f; return; }
		Yar -= 3.f;
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
		FireTrace(ShotDamage, AimSpreadDeg, 20000.f, FColor(255, 190, 90));
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
	FHitResult Hit;
	FVector End;
	const bool bHit = TraceCursorShot(Range, SpreadDeg, Hit, End);
	const FVector Muzzle = MuzzleLocation();
	const FVector Impact = bHit ? Hit.ImpactPoint : End;
	DrawDebugLine(GetWorld(), Muzzle, Impact, Tracer, false, 0.05f, 0, 1.2f);

	if (bHit && Hit.GetComponent() && Hit.GetComponent()->ComponentHasTag(TEXT("ParryPoint")))
	{
		if (AFNRiteObject* Rite = Cast<AFNRiteObject>(Hit.GetActor())) { Rite->OnShot(this); } // near-miss feedback
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
			if (const float Dealt = TargetHealth->ApplyDamage(Damage * TreeMods.Ranged * (bWeak ? WeakPointMultiplier * (1.f + TreeMods.Weak) : 1.f), this); Dealt > 0.f)
			{
				YarFromHit(TargetHealth, Dealt, Hit.GetActor());
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
		if (HasWeapon(Next) && Next != Weapon)
		{
			SelectWeapon(Next);
			return;
		}
	}
}

void AFNCharacter::OnAbility(int32 Slot)
{
	if (bDead || bTreeOpen) { return; }
	if (!IsAbilitySlotOpen(Slot)) { ShowMessage(TEXT("Слот 4 — ульта. Откроется в Нави")); return; }
	const int32 Id = Panel[Slot];
	if (Id < 0) { ShowMessage(FString::Printf(TEXT("Слот %d пуст — камень-навык ещё не найден"), Slot + 1)); return; }
	const FFNSkillDef& D = FNSkills::Def(static_cast<EFNSkillId>(Id));
	const float Cost = D.YarCost + ((EFNSkillId)Id == EFNSkillId::ChainSpark && Stage == EFNStage::Spark ? 5.f : 0.f);
	if (SkillCooldown[Id] > 0.f) { return; }
	if (Yar < Cost) { ShowMessage(FString::Printf(TEXT("Мало Яри: %s — %.0f"), *D.Name, Cost)); return; }
	if (CastSkill(Id))
	{
		Yar -= Cost;
		SkillCooldown[Id] = D.Cooldown;
	}
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
	// One ranged weapon in hand; a second one found goes to the inventory (Q / wheel swaps them).
	const bool bHadRanged = bHasRifle || bHasScatter;
	if (NewWeapon == EFNWeapon::Rifle) { bHasRifle = true; }
	if (NewWeapon == EFNWeapon::Scatter) { bHasScatter = true; }
	const TCHAR* Name = NewWeapon == EFNWeapon::Rifle ? TEXT("Ружьё") : TEXT("Дробовик");
	if (Stage != EFNStage::Spark && !bHadRanged) { Weapon = NewWeapon; ShowMessage(FString::Printf(TEXT("%s — в руке"), Name)); }
	else { ShowMessage(FString::Printf(TEXT("%s — в инвентаре  (Q — сменить)"), Name)); }
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

	if (Stage != EFNStage::Spark && Weapon == EFNWeapon::Plasma) { Weapon = bHasRifle ? EFNWeapon::Rifle : EFNWeapon::Scatter; }
	UpdateHelmet();
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
	LastMoveInput = FVector::ZeroVector;
	// Time nearly stops (not a hard pause, so HUD hit boxes and input keep working).
	UGameplayStatics::SetGlobalTimeDilation(this, bTreeOpen ? 0.02f : 1.f);
	ConfigureCursorInput();
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
				const float Dealt = H->ApplyDamage(Damage * (1.f + TreeMods.Melee), this);
				YarFromHit(H, Dealt, A);
				if (Dealt > 0.f && TreeMods.MeleeHeal > 0.f)
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
	if (!bDead && !bTreeOpen && !bRolling) { UpdateCursorAim(); }
	Boom->SetWorldRotation(FRotator(IsometricPitch, IsometricYaw, 0.f));
	Boom->TargetArmLength = FMath::FInterpTo(Boom->TargetArmLength, DesiredCameraDistance, DeltaSeconds, 8.f);
	Camera->SetFieldOfView(IsometricFOV);
	if (FParse::Param(FCommandLine::Get(), TEXT("IsometricTest"))) { RunIsometricSmokeTest(); }
	UpdateRetarget();
	UpdateFocus();
	{
		// Dry regen: out of ammo and low on Yar -> faster (skills_demo §2).
		const bool bDry = HasRangedWeapon() && Stage != EFNStage::Spark && Ammo + ScatterAmmo + Reserve <= 0;
		AddYar((bDry && Yar < 20.f ? 3.f : YarRegen) * DeltaSeconds);
		for (float& C : SkillCooldown) { C = FMath::Max(0.f, C - DeltaSeconds); }
		if (ShieldTime > 0.f) { ShieldTime -= DeltaSeconds; if (ShieldTime <= 0.f) { Health->Shield = 0.f; } }
	}
	if (PerfectSlowMo > 0.f)
	{
		PerfectSlowMo -= FApp::GetDeltaTime(); // real time
		if (PerfectSlowMo <= 0.f && !bTreeOpen) { UGameplayStatics::SetGlobalTimeDilation(this, 1.f); }
	}
	// Test key "-SkillTest": Flesh with a rifle among the Strelokopni mobs casts all five gems, screenshots each.
	if (FParse::Param(FCommandLine::Get(), TEXT("SkillTest")))
	{
		static int32 KStep = 0;
		const float T = GetWorld()->GetTimeSeconds();
		APlayerController* PC = Cast<APlayerController>(GetController());
		auto Shot = [](const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots") / FString(Name) + TEXT(".png"), false, false); };
		static const float At[] = { 8.f, 9.f, 9.3f, 11.f, 11.4f, 13.2f, 14.f, 14.3f, 15.5f, 15.8f, 17.f, 17.3f, 18.5f };
		if (KStep < UE_ARRAY_COUNT(At) && T > At[KStep])
		{
			switch (KStep)
			{
			case 0:
			{
				SetStage(EFNStage::Flesh, false);
				GiveWeapon(EFNWeapon::Rifle);
				for (int32 k = 0; k < 3; ++k) { GiveSkill(k); }
				FHitResult Hit;
				const FVector From(285.f * 100.f, -40.f * 100.f, 0.f);
				GetWorld()->LineTraceSingleByChannel(Hit, From + FVector(0, 0, 50000.f), From - FVector(0, 0, 10000.f), ECC_WorldStatic);
				SetActorLocation(Hit.ImpactPoint + FVector(0.f, 0.f, 100.f), false, nullptr, ETeleportType::TeleportPhysics);
				LastSafeLocation = GetActorLocation();
				if (PC) { PC->SetControlRotation(FRotator(-12.f, 0.f, 0.f)); }
				Yar = MaxYar;
				break;
			}
			case 1: Shot(TEXT("40_panel")); break;
			case 2: OnAbility(0); break;                       // Громовой удар
			case 3: Shot(TEXT("41_strike_after")); Yar = MaxYar; OnAbility(1); break; // Громоотвод
			case 4: Shot(TEXT("42_rod_telegraph")); break;
			case 5: Shot(TEXT("43_rod_bolt")); Yar = MaxYar; break;
			case 6: OnAbility(2); break;                       // Сполох
			case 7: Shot(TEXT("44_flare")); break;
			case 8: Panel[0] = 3; Panel[1] = 4; Yar = MaxYar; OnAbility(0); break; // Цепная искра
			case 9: Shot(TEXT("45_chain")); break;
			case 10: Yar = MaxYar; OnAbility(1); break;        // Оберег грозы
			case 11: Shot(TEXT("46_ward")); break;
			case 12: if (PC) { PC->ConsoleCommand(TEXT("quit")); } break;
			}
			++KStep;
		}
	}

	// Test key "-RiteTest": walks the two rite chains as Flesh and screenshots each step (UI review).
	if (FParse::Param(FCommandLine::Get(), TEXT("RiteTest")))
	{
		static int32 RStep = 0;
		const float T = GetWorld()->GetTimeSeconds();
		APlayerController* PC = Cast<APlayerController>(GetController());
		auto Near = [this, PC](float X, float Y, float BackM)
		{
			FHitResult Hit;
			const FVector Target(X * 100.f, Y * 100.f, 0.f);
			const FVector From = Target - FVector(BackM * 100.f, 0.f, 0.f);
			GetWorld()->LineTraceSingleByChannel(Hit, From + FVector(0, 0, 50000.f), From - FVector(0, 0, 10000.f), ECC_WorldStatic);
			SetActorLocation(Hit.ImpactPoint + FVector(0.f, 0.f, 100.f), false, nullptr, ETeleportType::TeleportPhysics);
			LastSafeLocation = GetActorLocation(); // a test teleport is not a fall
			if (PC) { PC->SetControlRotation(FRotator(-10.f, 0.f, 0.f)); }
		};
		auto Shot = [](const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots") / FString(Name) + TEXT(".png"), false, false); };
		struct FS { float At; int32 Id; };
		static const FS Plan[] = { { 8, 0 }, { 9.5f, 1 }, { 10.5f, 2 }, { 12, 3 }, { 13, 4 }, { 14, 5 }, { 15, 6 }, { 17, 7 }, { 18, 8 }, { 19.5f, 9 }, { 21, 10 }, { 22, 11 }, { 23.5f, 12 }, { 25, 13 } };
		if (RStep < UE_ARRAY_COUNT(Plan) && T > Plan[RStep].At)
		{
			switch (Plan[RStep].Id)
			{
			case 0: SetStage(EFNStage::Flesh, false); Near(326.f, -54.f, 3.f); break;
			case 1: Shot(TEXT("30_elder_prompt")); break;
			case 2: OnInteract(); break;
			case 3: Shot(TEXT("31_elder_line")); Near(348.f, -30.f, 2.5f); break;
			case 4: Shot(TEXT("32_arrow_prompt")); OnInteract(); break;
			case 5: Near(-23.8f, -15.f, -2.5f); if (PC) { PC->SetControlRotation(FRotator(-10.f, 180.f, 0.f)); } break;
			case 6: Shot(TEXT("33_beam_wait")); for (TActorIterator<AFNVysiGreybox> It(GetWorld()); It; ++It) { It->StrikeNow(); } break;
			case 7: OnInteract(); break;
			case 8: Shot(TEXT("34_cache")); Near(524.f, -38.f, 2.5f); break;
			case 9: Shot(TEXT("35_goat_prompt")); OnInteract(); break;
			case 10: Near(539.f, -28.f, 2.5f); break;
			case 11: Shot(TEXT("36_skull_prompt")); OnInteract(); break;
			case 12: if (PC) { PC->SetControlRotation(FRotator(-15.f, 180.f, 0.f)); } Shot(TEXT("37_helmet")); break;
			case 13: if (PC) { PC->ConsoleCommand(TEXT("quit")); } break;
			}
			++RStep;
		}
	}

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

	GetCharacterMovement()->MaxWalkSpeed = DefaultWalkSpeed;
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

void AFNCharacter::ShowSubtitle(const FString& Speaker, const FString& Text)
{
	SubSpeaker = Speaker;
	SubText = Text;
	SubTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
}

bool AFNCharacter::HasSubtitle() const
{
	return GetWorld() && GetWorld()->GetTimeSeconds() - SubTime < 5.0;
}

void AFNCharacter::UpdateFocus()
{
	Focus = nullptr;
	if (bDead || bTreeOpen) { return; }
	float Best = 330.f;
	const FVector Me = GetActorLocation();
	const FVector Fwd = GetControlRotation().Vector().GetSafeNormal2D();
	for (TActorIterator<AFNRiteObject> It(GetWorld()); It; ++It)
	{
		if (It->IsSpent()) { continue; }
		const FVector To = It->GetActorLocation() - Me;
		const float D = To.Size2D();
		if (D < Best && FMath::Abs(To.Z) < 400.f && FVector::DotProduct(To.GetSafeNormal2D(), Fwd) > 0.2f) { Best = D; Focus = *It; }
	}
}

FString AFNCharacter::GetFocusPrompt(bool& bCan) const
{
	bCan = false;
	return Focus ? Focus->GetPrompt(this, bCan) : FString();
}

void AFNCharacter::OnInteract()
{
	if (Focus && !bDead && !bTreeOpen) { Focus->Use(this); }
}

void AFNCharacter::GiveHelmet()
{
	bHelmet = true;
	auto Make = [this](USceneComponent* Parent, FName Socket, const FVector& Offset)
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
		C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetupAttachment(Parent, Socket);
		C->SetRelativeLocation(Offset);
		C->SetRelativeScale3D(FVector(0.3f, 0.18f, 0.2f));
		C->RegisterComponent();
		if (UMaterialInstanceDynamic* MID = C->CreateDynamicMaterialInstance(0, LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"))))
		{
			MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.82f, 0.79f, 0.7f));
		}
		return C;
	};
	if (!HelmetOnFlesh && GetMesh()->GetSkeletalMeshAsset()) { HelmetOnFlesh = Make(GetMesh(), TEXT("head"), FVector(12.f, 0.f, 0.f)); }
	if (!HelmetOnBones && SkeletonMesh->GetSkinnedAsset()) { HelmetOnBones = Make(SkeletonMesh, TEXT("Head"), FVector(0.f, 12.f, 0.f)); }
	UpdateHelmet();
}

void AFNCharacter::UpdateHelmet()
{
	if (HelmetOnFlesh) { HelmetOnFlesh->SetVisibility(bHelmet && GetMesh()->IsVisible()); }
	if (HelmetOnBones) { HelmetOnBones->SetVisibility(bHelmet && SkeletonMesh->IsVisible()); }
}

float AFNCharacter::GetPerfectDodgeAge() const
{
	return GetWorld() ? static_cast<float>(GetWorld()->GetTimeSeconds() - LastPerfectDodge) : 100.f;
}

void AFNCharacter::OnAttackAvoided()
{
	// Perfect dodge: an attack that would have hit arrives within the window after the dodge started (GDD §5).
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now - DodgeStartTime > PerfectDodgeWindow || Now - LastPerfectDodge < PerfectDodgeCooldown)
	{
		return;
	}
	LastPerfectDodge = Now;
	bool bBigFoe = false;
	for (TActorIterator<AFNPerunBoss> It(GetWorld()); It; ++It) { bBigFoe = It->IsFightActive() && FVector::Dist(It->GetActorLocation(), GetActorLocation()) < 2500.f; }
	AddYar(bBigFoe ? 25.f : 15.f);
	// A beat of slow motion so the player feels it.
	UGameplayStatics::SetGlobalTimeDilation(this, 0.3f);
	PerfectSlowMo = 0.15f;
	SparkLight->SetIntensity(SparkLight->Intensity + 20000.f);
}

float AFNCharacter::GetSkillCooldownRatio(int32 Slot) const
{
	const int32 Id = GetPanelSkill(Slot);
	return Id < 0 ? 0.f : SkillCooldown[Id] / FMath::Max(0.1f, FNSkills::Def(static_cast<EFNSkillId>(Id)).Cooldown);
}

bool AFNCharacter::CanAffordSkill(int32 Slot) const
{
	const int32 Id = GetPanelSkill(Slot);
	return Id >= 0 && Yar >= FNSkills::Def(static_cast<EFNSkillId>(Id)).YarCost;
}

float AFNCharacter::GetShield() const
{
	return Health->Shield;
}

void AFNCharacter::GiveSkill(int32 SkillId)
{
	if (OwnedSkills.Contains(SkillId)) { return; }
	OwnedSkills.Add(SkillId);
	for (int32& P : Panel)
	{
		if (P < 0) { P = SkillId; break; } // demo: auto-equip in order of finding
	}
	ShowMessage(FString::Printf(TEXT("Камень-навык: %s"), *FNSkills::Def(static_cast<EFNSkillId>(SkillId)).Name));
}

void AFNCharacter::YarFromHit(const UFNHealthComponent* Target, float Dealt, const AActor* Victim)
{
	if (!Target || Dealt <= 0.f) { return; }
	// 1 Yar per 5% HP of a normal mob (~20 per kill); bosses 1 per 1% HP, at most 8/s (skills_demo §2).
	if (Cast<AFNPerunBoss>(Victim))
	{
		const double Now = GetWorld()->GetTimeSeconds();
		if (Now - YarBossWindowStart > 1.0) { YarBossWindowStart = Now; YarBossWindow = 0.f; }
		const float Gain = FMath::Min(100.f * Dealt / FMath::Max(1.f, Target->MaxHealth), 8.f - YarBossWindow);
		if (Gain > 0.f) { YarBossWindow += Gain; AddYar(Gain); }
		return;
	}
	AddYar(20.f * Dealt / FMath::Max(1.f, Target->MaxHealth));
}

FVector AFNCharacter::AimPoint(float MaxRange) const
{
	FVector P, Target;
	CursorWorldPoints(P, Target);
	FCollisionQueryParams Q(SCENE_QUERY_STAT(FNAimPoint), false, this);
	const FVector Me = GetActorLocation();
	if (FVector::Dist2D(P, Me) > MaxRange) { P = Me + (P - Me).GetSafeNormal2D() * MaxRange; }
	// Drop onto the ground.
	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, P + FVector(0, 0, 1500.f), P - FVector(0, 0, 3000.f), ECC_WorldStatic, Q)) { P = Hit.ImpactPoint; }
	return P;
}

bool AFNCharacter::CursorRay(FVector& Origin, FVector& Direction) const
{
	const APlayerController* PC = Cast<APlayerController>(Controller);
	if (!PC) { return false; }
	if (bTestCursor) { return PC->DeprojectScreenPositionToWorld(TestCursorScreen.X, TestCursorScreen.Y, Origin, Direction); }
	return PC->DeprojectMousePositionToWorld(Origin, Direction);
}

void AFNCharacter::CursorWorldPoints(FVector& Ground, FVector& Target) const
{
	const FVector Me = GetActorLocation();
	Ground = Me + GetActorForwardVector() * 600.f - FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	FVector Origin, Direction;
	if (CursorRay(Origin, Direction))
	{
		FHitResult Hit;
		const FCollisionQueryParams Q(SCENE_QUERY_STAT(FNCursor), false, this);
		if (GetWorld()->LineTraceSingleByChannel(Hit, Origin, Origin + Direction * 100000.f, ECC_Visibility, Q))
		{
			Ground = Hit.ImpactPoint;
			Target = Ground;
			// Ground clicks aim at body height. Direct enemy/weak-point hits keep their exact height.
			if ((!Hit.GetActor() || !Hit.GetActor()->FindComponentByClass<UFNHealthComponent>()) && Hit.ImpactNormal.Z > 0.5f)
			{
				Target.Z += CursorAimHeight;
			}
			return;
		}
		// Empty sky/edges: intersect the hero's ground plane instead of aiming along the camera.
		if (Direction.Z < -KINDA_SMALL_NUMBER)
		{
			const float T = (Me.Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - Origin.Z) / Direction.Z;
			if (T > 0.f) { Ground = Origin + Direction * T; }
		}
	}
	Target = Ground + FVector(0.f, 0.f, CursorAimHeight);
}

void AFNCharacter::UpdateCursorAim()
{
	FVector Ground, Target;
	CursorWorldPoints(Ground, Target);
	const FVector Direction = Target - GetActorLocation();
	if (!Direction.IsNearlyZero() && !Direction.GetSafeNormal2D().IsNearlyZero())
	{
		const FRotator Aim = Direction.Rotation();
		SetActorRotation(FRotator(0.f, Aim.Yaw, 0.f));
		if (Controller) { Controller->SetControlRotation(Aim); }
	}
}

FVector AFNCharacter::MuzzleLocation() const
{
	return Stage == EFNStage::Spark ? SparkOrb->GetComponentLocation() : Gun->GetComponentLocation() + GetActorForwardVector() * 50.f;
}

bool AFNCharacter::TraceCursorShot(float Range, float SpreadDeg, FHitResult& Hit, FVector& End) const
{
	FVector Ground, Target;
	CursorWorldPoints(Ground, Target);
	const FVector Muzzle = MuzzleLocation();
	FVector Direction = (Target - Muzzle).GetSafeNormal();
	if (Direction.IsNearlyZero()) { Direction = GetActorForwardVector(); }
	if (SpreadDeg > 0.f) { Direction = FMath::VRandCone(Direction, FMath::DegreesToRadians(SpreadDeg)); }
	End = Muzzle + Direction * Range;
	// Trace from the weapon: an overhead cursor must never allow shooting through a wall.
	return GetWorld()->LineTraceSingleByChannel(Hit, Muzzle, End, ECC_Visibility, FCollisionQueryParams(SCENE_QUERY_STAT(FNCursorShot), false, this));
}

bool AFNCharacter::CastSkill(int32 SkillId)
{
	UWorld* W = GetWorld();
	const FVector Me = GetActorLocation();
	auto HitPawnsInSphere = [this, W](const FVector& C, float R, float Damage, TFunctionRef<bool(const FVector&)> Filter, TFunctionRef<void(AActor*)> After)
	{
		TArray<FOverlapResult> Hits;
		W->OverlapMultiByObjectType(Hits, C, FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(R), FCollisionQueryParams(SCENE_QUERY_STAT(FNSkill), false, this));
		TSet<AActor*> Done;
		for (const FOverlapResult& O : Hits)
		{
			AActor* A = O.GetActor();
			if (!A || A == this || Done.Contains(A) || !Filter(A->GetActorLocation())) { continue; }
			Done.Add(A);
			if (UFNHealthComponent* H = A->FindComponentByClass<UFNHealthComponent>())
			{
				YarFromHit(H, H->ApplyDamage(Damage, this), A);
				After(A);
			}
		}
	};
	auto NoFilter = [](const FVector&) { return true; };
	auto NoAfter = [](AActor*) {};

	const FFNSkillDef& D = FNSkills::Def(static_cast<EFNSkillId>(SkillId)); // numbers from skills.csv
	switch (static_cast<EFNSkillId>(SkillId))
	{
	case EFNSkillId::ThunderStrike:
	{
		// 180% of the melee weapon: Spark flash 15 (all around, 3 m), Skeleton fist 25, Flesh axe +20%.
		const float Base = Stage == EFNStage::Spark ? 15.f : (Stage == EFNStage::Skeleton ? 25.f : (MeleeDamage + 5.f) * 1.2f);
		const float R = Stage == EFNStage::Spark ? D.RadiusCm * 0.75f : D.RadiusCm; // Spark fights all around, a bit shorter
		const FVector Fwd = GetActorForwardVector();
		const bool bRing = Stage == EFNStage::Spark || D.ArcDeg >= 359.f;
		const float MinDot = FMath::Cos(FMath::DegreesToRadians(D.ArcDeg * 0.5f));
		HitPawnsInSphere(Me, R, Base * D.Damage * (1.f + TreeMods.Melee), [&](const FVector& P) { return bRing || FVector::DotProduct((P - Me).GetSafeNormal2D(), Fwd) >= MinDot; }, NoAfter);
		DrawDebugCircle(W, Me - FVector(0, 0, 80.f), R, 32, FColor(150, 170, 255), false, 0.25f, 0, 6.f, FVector(1, 0, 0), FVector(0, 1, 0), false);
		return true;
	}
	case EFNSkillId::LightningRod:
	{
		// A rod in the aim point; 1.5 s later a bolt R 3 m with a 0.8 s stun (the exam's lightning in the hero's hands).
		const FVector At = AimPoint(D.RangeCm);
		const float R = D.RadiusCm, Dmg = D.Damage, StunS = D.Stun;
		DrawDebugCylinder(W, At, At + FVector(0, 0, 250.f), 8.f, 8, FColor(150, 170, 255), false, D.Delay, 0, 3.f);
		DrawDebugCircle(W, At + FVector(0, 0, 5.f), R, 32, FColor(150, 170, 255), false, D.Delay, 0, 3.f, FVector(1, 0, 0), FVector(0, 1, 0), false);
		TWeakObjectPtr<AFNCharacter> Self(this);
		FTimerHandle Handle;
		W->GetTimerManager().SetTimer(Handle, [Self, At, R, Dmg, StunS]()
		{
			if (!Self.IsValid()) { return; }
			AFNCharacter* H = Self.Get();
			TArray<FOverlapResult> Hits;
			H->GetWorld()->OverlapMultiByObjectType(Hits, At, FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(R), FCollisionQueryParams(SCENE_QUERY_STAT(FNRod), false, H));
			TSet<AActor*> Done;
			for (const FOverlapResult& O : Hits)
			{
				AActor* A = O.GetActor();
				if (!A || A == H || Done.Contains(A)) { continue; }
				Done.Add(A);
				if (UFNHealthComponent* HC = A->FindComponentByClass<UFNHealthComponent>()) { H->YarFromHit(HC, HC->ApplyDamage(Dmg, H), A); }
				if (AFNMob* Mob = Cast<AFNMob>(A)) { Mob->Stun(StunS); }
			}
			DrawDebugLine(H->GetWorld(), At + FVector(0, 0, 3000.f), At, FColor(170, 180, 255), false, 0.2f, 0, 40.f);
		}, FMath::Max(0.05f, D.Delay), false);
		return true;
	}
	case EFNSkillId::Flare:
	{
		// Dash 6 m through enemies, i-frames 0.25 s, a discharge along the way (60%).
		const FVector Dir = (LastMoveInput.IsNearlyZero() ? GetActorForwardVector() : LastMoveInput).GetSafeNormal2D();
		FHitResult Hit;
		FCollisionQueryParams Q(SCENE_QUERY_STAT(FNFlare), false, this);
		const FVector To = Me + Dir * D.RangeCm;
		const bool bWall = W->SweepSingleByChannel(Hit, Me, To, FQuat::Identity, ECC_WorldStatic, FCollisionShape::MakeSphere(35.f), Q);
		const FVector End = bWall ? Hit.Location : To;
		const FVector Mid = (Me + End) * 0.5f;
		HitPawnsInSphere(Mid, FVector::Dist(Me, End) * 0.5f + 100.f, D.Damage, NoFilter, NoAfter);
		SetActorLocation(End, false, nullptr, ETeleportType::TeleportPhysics);
		DrawDebugLine(W, Me, End, FColor(150, 170, 255), false, 0.25f, 0, 8.f);
		IFramesRemaining = FMath::Max(IFramesRemaining, D.IFrames);
		Health->bInvulnerable = true;
		DodgeStartTime = W->GetTimeSeconds();
		return true;
	}
	case EFNSkillId::ChainSpark:
	{
		// A shot that jumps to 3 targets (-25% per jump); 1 bullet (Spark pays +5 Yar instead).
		if (Stage != EFNStage::Spark)
		{
			int32& Mag = Weapon == EFNWeapon::Scatter ? ScatterAmmo : Ammo;
			if (!HasRangedWeapon() || Mag < D.AmmoCost) { ShowMessage(TEXT("Нет патрона для Цепной искры")); return false; }
			Mag -= D.AmmoCost;
		}
		FHitResult Hit;
		FVector End;
		AActor* Cur = TraceCursorShot(D.RangeCm, 0.f, Hit, End) ? Hit.GetActor() : nullptr;
		FVector Prev = Me;
		float Damage = ShotDamage * D.Damage * TreeMods.Ranged;
		TSet<AActor*> Done;
		for (int32 Jump = 0; Jump < 4 && Cur; ++Jump)
		{
			UFNHealthComponent* H = Cur->FindComponentByClass<UFNHealthComponent>();
			if (!H || Done.Contains(Cur)) { break; }
			Done.Add(Cur);
			YarFromHit(H, H->ApplyDamage(Damage, this), Cur);
			DrawDebugLine(W, Prev, Cur->GetActorLocation(), FColor(150, 170, 255), false, 0.2f, 0, 4.f);
			Prev = Cur->GetActorLocation();
			Damage *= 0.75f;
			// Next: nearest other mob within 8 m.
			AActor* Next = nullptr;
			float Best = 800.f;
			for (TActorIterator<AFNMob> It(W); It; ++It)
			{
				const float Dist = FVector::Dist(It->GetActorLocation(), Prev);
				if (!Done.Contains(*It) && !It->IsDead() && Dist < Best) { Best = Dist; Next = *It; }
			}
			Cur = Next;
		}
		if (Done.Num() == 0) { DrawDebugLine(W, MuzzleLocation(), Hit.bBlockingHit ? Hit.ImpactPoint : End, FColor(150, 170, 255), false, 0.15f, 0, 3.f); }
		return true;
	}
	case EFNSkillId::StormWard:
		// Shield of 25% max HP for 5 s; the break discharge lives in Health->OnShieldBroken.
		Health->Shield = Health->MaxHealth * 0.25f; // shield share: see notes in skills.csv
		ShieldTime = D.Duration;
		return true;
	default:
		return false;
	}
}
