#include "FNCharacter.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "FNHeroAnimInstance.h"
#include "FNHealthComponent.h"
#include "FNMob.h"
#include "FNPerunBoss.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformMisc.h"
#include "HighResScreenshot.h"
#include "Misc/Paths.h"

// -AnimationTest exercises the actual input handlers, contact timing and mesh
// poses. It runs only on explicit request and exits with a failing status if a
// required local animation is missing or a strike survives cancellation.
void AFNCharacter::RunAnimationSmokeTest()
{
	APlayerController* PC = Cast<APlayerController>(Controller);
	if (!PC) { return; }
	const float Now = GetWorld()->GetTimeSeconds();
	if (AnimationTestTarget.IsValid())
	{
		bTestCursor = PC->ProjectWorldLocationToScreen(AnimationTestTarget->GetActorLocation(), TestCursorScreen);
	}
	if (AnimationTestStep == 11 || AnimationTestStep == 12) { AddMovementInput(-GetActorRightVector(), 1.f); }
	if (Now < AnimationTestNextTime) { return; }
	// Screenshot/PSO stalls can advance the world clock before this actor's
	// contact timer has consumed the same tick. Allow a bounded catch-up.
	if ((AnimationTestStep == 3 || AnimationTestStep == 7) && MeleeHitRemaining >= 0.f && Now < AnimationTestNextTime + 0.5f) { return; }
	auto Check = [this](bool Pass, const TCHAR* What)
	{
		if (Pass) { UE_LOG(LogTemp, Display, TEXT("[AnimTest] PASS %s"), What); }
		else { ++AnimationTestFailures; UE_LOG(LogTemp, Error, TEXT("[AnimTest] FAIL %s"), What); }
	};
	auto Shot = [](const TCHAR* Name)
	{
		FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/Animations") / FString(Name) + TEXT(".png"), false, false);
	};
	UFNHeroAnimInstance* Anim = Cast<UFNHeroAnimInstance>(GetMesh()->GetAnimInstance());
	UFNHealthComponent* Target = AnimationTestTarget.IsValid() ? AnimationTestTarget->FindComponentByClass<UFNHealthComponent>() : nullptr;
	AnimationTestNextTime = Now + 0.8f;
	switch (AnimationTestStep++)
	{
	case 0:
	{
		for (TActorIterator<AFNMob> It(GetWorld()); It; ++It) { It->SetActorTickEnabled(false); }
		for (TActorIterator<AFNPerunBoss> It(GetWorld()); It; ++It) { It->SetActorTickEnabled(false); }
		SetStage(EFNStage::Flesh, false);
		Health->bInvulnerable = true;
		Yar = MaxYar;
		DesiredCameraDistance = 1300.f;
		// Place the test on the open Okolitsa slope. A wall beside the initial
		// spawn can intercept the projected cursor before it reaches the target.
		FHitResult Ground;
		const FVector TestXY(10000.f, 2500.f, 0.f);
		if (GetWorld()->LineTraceSingleByChannel(Ground, TestXY + FVector(0.f, 0.f, 50000.f), TestXY - FVector(0.f, 0.f, 10000.f), ECC_WorldStatic))
		{
			SetActorLocation(Ground.ImpactPoint + FVector(0.f, 0.f, 100.f), false, nullptr, ETeleportType::TeleportPhysics);
			LastSafeLocation = GetActorLocation();
		}
		AnimationTestOrigin = GetActorLocation();
		SetActorRotation(FRotator::ZeroRotator);
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AStaticMeshActor* A = GetWorld()->SpawnActor<AStaticMeshActor>(AnimationTestOrigin + FVector(160.f, 0.f, 0.f), FRotator::ZeroRotator, P);
		UStaticMeshComponent* Cube = A->GetStaticMeshComponent();
		Cube->SetMobility(EComponentMobility::Movable);
		Cube->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
		Cube->SetWorldScale3D(FVector(0.7f, 0.7f, 1.5f));
		Cube->SetCollisionObjectType(ECC_Pawn);
		Cube->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Cube->SetGenerateOverlapEvents(true);
		Cube->SetCollisionResponseToAllChannels(ECR_Overlap);
		Cube->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		UFNHealthComponent* H = NewObject<UFNHealthComponent>(A);
		A->AddInstanceComponent(H);
		H->RegisterComponent();
		H->MaxHealth = H->Health = 10000.f;
		AnimationTestTarget = A;
		Check(Anim && MeleeAnim && UnarmedAnim && DodgeAnim, TEXT("native instance and all three actions are loaded"));
		Check(Anim && Anim->ShotSequence && Anim->Locomotion[0] && Anim->Locomotion[4], TEXT("idle, directional movement and full-pose firing assets are loaded"));
		break;
	}
	case 1:
		AnimationTestHealth = Target ? Target->Health : 0.f;
		SetActorRotation((AnimationTestTarget->GetActorLocation() - GetActorLocation()).Rotation());
		OnMelee();
		Check(Target && Target->Health == AnimationTestHealth && MeleeHitRemaining > 0.f, TEXT("melee wind-up does not deal immediate damage"));
		Check(Anim && Anim->HasFullBodyAction(), TEXT("weapon swing starts a full-body action"));
		AnimationTestNextTime = Now + 0.14f;
		break;
	case 2:
		Shot(TEXT("01_flesh_windup"));
		AnimationTestNextTime = Now + 0.13f;
		break;
	case 3:
		Check(Target && Target->Health < AnimationTestHealth, TEXT("melee deals damage at animated contact"));
		Check(FVector::Dist(GetMesh()->GetBoneLocation(TEXT("pelvis")), GetActorLocation()) < 200.f, TEXT("melee root stays inside the capsule neighbourhood"));
		Shot(TEXT("02_flesh_contact"));
		break;
	case 4:
		Stamina = MaxStamina;
		AnimationTestHealth = Target ? Target->Health : 0.f;
		OnMelee();
		OnRoll();
		Check(bRolling && MeleeHitRemaining < 0.f, TEXT("dodge cancels pending melee contact"));
		AnimationTestNextTime = Now + 0.25f;
		break;
	case 5:
		Shot(TEXT("03_flesh_roll"));
		Check(Anim && Anim->HasFullBodyAction() && Anim->ActionSequence == DodgeAnim, TEXT("Flesh uses the rolling pose"));
		Check(FVector::Dist(GetMesh()->GetBoneLocation(TEXT("pelvis")), GetActorLocation()) < 200.f, TEXT("roll strips source travel and jump height"));
		break;
	case 6:
		Check(!bRolling && Target && Target->Health == AnimationTestHealth, TEXT("dodge ends and cancelled strike never hits"));
		SetActorLocation(AnimationTestOrigin, false, nullptr, ETeleportType::TeleportPhysics);
		GetCharacterMovement()->StopMovementImmediately();
		SetStage(EFNStage::Skeleton, false);
		Stamina = MaxStamina;
		AnimationTestHealth = Target ? Target->Health : 0.f;
		SetActorRotation((AnimationTestTarget->GetActorLocation() - GetActorLocation()).Rotation());
		OnMelee();
		AnimationTestNextTime = Now + 0.19f;
		break;
	case 7:
		Shot(TEXT("04_skeleton_strike"));
		Check(Anim && Anim->ActionSequence == UnarmedAnim && Target && Target->Health < AnimationTestHealth, TEXT("Skeleton strikes with its own action and contact timing"));
		Check(FVector::Dist(GetMesh()->GetBoneLocation(TEXT("pelvis")), GetActorLocation()) < 130.f, TEXT("unarmed source charge does not move the body out of its capsule"));
		break;
	case 8:
		Stamina = MaxStamina;
		OnRoll();
		Check(Anim && Anim->ActionSequence == UnarmedAnim, TEXT("Skeleton uses a dash instead of a Flesh roll"));
		AnimationTestNextTime = Now + 0.13f;
		break;
	case 9:
		Shot(TEXT("05_skeleton_dash"));
		break;
	case 10:
		SetStage(EFNStage::Flesh, false);
		GiveWeapon(EFNWeapon::Rifle);
		Weapon = EFNWeapon::Rifle;
		GetCharacterMovement()->StopMovementImmediately();
		SetActorLocation(AnimationTestOrigin, false, nullptr, ETeleportType::TeleportPhysics);
		FireShot();
		Check(Anim && Anim->ShotElapsed == 0.f, TEXT("shooting starts the upper-body pose"));
		AnimationTestNextTime = Now + 0.15f;
		break;
	case 11:
		Shot(TEXT("06_flesh_fire"));
		AnimationTestNextTime = Now + 0.9f;
		break;
	case 12:
		Check(GetVelocity().Size2D() > 50.f && Anim && Anim->LocalVelocity.Size2D() > 50.f, TEXT("directional locomotion receives actual moving velocity"));
		Shot(TEXT("07_flesh_strafe"));
		GetCharacterMovement()->StopMovementImmediately();
		ToggleMap();
		OnMelee();
		OnRoll();
		Check(MeleeHitRemaining < 0.f && !bRolling, TEXT("map blocks animated attacks and dodges"));
		ToggleMap();
		break;
	case 13:
		UE_LOG(LogTemp, Display, TEXT("[AnimTest] COMPLETE failures=%d"), AnimationTestFailures);
		FPlatformMisc::RequestExitWithStatus(false, AnimationTestFailures == 0 ? 0 : 1);
		break;
	}
}
