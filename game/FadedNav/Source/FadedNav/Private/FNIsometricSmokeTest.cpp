#include "FNCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "FNHealthComponent.h"
#include "FNMob.h"
#include "FNPerunBoss.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "HAL/PlatformMisc.h"
#include "HighResScreenshot.h"
#include "Misc/Paths.h"

// Run with -IsometricTest. Uses actual camera projection, collision, skills and UI mode.
// Only this explicitly requested test spawns a target/wall and disables enemy ticks.
void AFNCharacter::RunIsometricSmokeTest()
{
	APlayerController* PC = Cast<APlayerController>(Controller);
	if (!PC) { return; }
	const float Now = GetWorld()->GetRealTimeSeconds();
	if (IsometricTestTarget.IsValid())
	{
		bTestCursor = PC->ProjectWorldLocationToScreen(IsometricTestTarget->GetActorLocation(), TestCursorScreen);
	}
	if (Now < IsometricTestNextTime) { return; }
	auto Check = [this](bool bPass, const TCHAR* What)
	{
		if (bPass) { UE_LOG(LogTemp, Display, TEXT("[IsoTest] PASS %s"), What); }
		else { ++IsometricTestFailures; UE_LOG(LogTemp, Error, TEXT("[IsoTest] FAIL %s"), What); }
	};
	auto Shot = [](const TCHAR* Name)
	{
		FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/Isometric") / FString(Name) + TEXT(".png"), false, false);
	};
	auto SpawnCube = [this](const FVector& At, const FVector& Scale, bool bTarget) -> AStaticMeshActor*
	{
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AStaticMeshActor* A = GetWorld()->SpawnActor<AStaticMeshActor>(At, FRotator::ZeroRotator, P);
		UStaticMeshComponent* C = A->GetStaticMeshComponent();
		C->SetMobility(EComponentMobility::Movable);
		C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
		C->SetWorldScale3D(Scale);
		C->SetCollisionObjectType(bTarget ? ECC_Pawn : ECC_WorldStatic);
		C->SetCollisionResponseToAllChannels(ECR_Block);
		if (bTarget)
		{
			UFNHealthComponent* H = NewObject<UFNHealthComponent>(A);
			H->RegisterComponent();
			H->MaxHealth = H->Health = 500.f;
		}
		return A;
	};
	UFNHealthComponent* TargetHealth = IsometricTestTarget.IsValid() ? IsometricTestTarget->FindComponentByClass<UFNHealthComponent>() : nullptr;
	IsometricTestNextTime = Now + 0.8f;
	switch (IsometricTestStep++)
	{
	case 0:
		for (TActorIterator<AFNMob> It(GetWorld()); It; ++It) { It->SetActorTickEnabled(false); }
		for (TActorIterator<AFNPerunBoss> It(GetWorld()); It; ++It) { It->SetActorTickEnabled(false); }
		ReviveAt(FVector(-700.f, 0.f, 100.f));
		SetStage(EFNStage::Flesh, false);
		GiveWeapon(EFNWeapon::Rifle);
		Weapon = EFNWeapon::Rifle;
		Health->bInvulnerable = true;
		for (int32 Id = 0; Id < 5; ++Id) { GiveSkill(Id); }
		Yar = MaxYar;
		IsometricTestTarget = SpawnCube(FVector(0.f, 0.f, 100.f), FVector(1.2f, 1.2f, 2.f), true);
		Check(PC->bShowMouseCursor, TEXT("cursor remains available during combat"));
		break;
	case 1:
	{
		UpdateCursorAim();
		Check(bTestCursor, TEXT("world target projects into the viewport"));
		Check(FVector::DotProduct(GetActorForwardVector(), (IsometricTestTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal2D()) > 0.99f, TEXT("hero faces the cursor target"));
		Check(Boom->GetComponentRotation().Equals(FRotator(IsometricPitch, IsometricYaw, 0.f), 0.1f), TEXT("camera stays fixed while hero turns"));
		Check(FVector::Dist2D(AimPoint(800.f), IsometricTestTarget->GetActorLocation()) < 80.f, TEXT("ground skill resolves beneath the cursor"));
		const float Before = TargetHealth ? TargetHealth->Health : 0.f;
		FireTrace(25.f, 0.f, 2000.f, FColor(150, 170, 255));
		Check(TargetHealth && TargetHealth->Health < Before, TEXT("muzzle shot hits the cursor target"));
		Shot(TEXT("01_flesh_aim"));
		break;
	}
	case 2:
	{
		const float Before = TargetHealth ? TargetHealth->Health : 0.f;
		const int32 BeforeAmmo = Ammo;
		CastSkill(3); // ChainSpark
		Check(TargetHealth && TargetHealth->Health < Before && Ammo == BeforeAmmo - 1, TEXT("chain skill uses the same cursor ray and spends ammo"));
		IsometricTestWall = SpawnCube(FVector(-350.f, 0.f, 150.f), FVector(1.f, 4.f, 3.f), false);
		IsometricTestHealth = TargetHealth ? TargetHealth->Health : 0.f;
		break;
	}
	case 3:
		UpdateCursorAim();
		FireTrace(25.f, 0.f, 2000.f, FColor(150, 170, 255));
		Check(TargetHealth && FMath::IsNearlyEqual(TargetHealth->Health, IsometricTestHealth), TEXT("wall blocks an overhead-aimed shot"));
		if (IsometricTestWall.IsValid()) { IsometricTestWall->Destroy(); }
		IsometricTestStart = GetActorLocation();
		OnMove(FInputActionValue(FVector2D(0.f, 1.f)));
		const FVector ScreenForward = FRotationMatrix(FRotator(0.f, Boom->GetComponentRotation().Yaw, 0.f)).GetUnitAxis(EAxis::X);
		Check(FVector::DotProduct(LastMoveInput, ScreenForward) > 0.99f, TEXT("W resolves to the screen-facing camera basis"));
		IsometricTestNextTime = Now + 0.25f;
		break;
	case 4:
	{
		OnMoveStopped();
		Check(LastMoveInput.IsNearlyZero(), TEXT("released movement does not leave a stale dodge direction"));
		OnRoll();
		Check(bRolling && FVector::DotProduct(RollDirection, GetActorForwardVector()) > 0.99f, TEXT("idle dodge follows the cursor-facing direction"));
		break;
	}
	case 5:
		Check(!bRolling, TEXT("dodge finishes normally"));
		OnZoom(FInputActionValue(1000.f));
		Check(FMath::IsNearlyEqual(DesiredCameraDistance, MinCameraDistance), TEXT("zoom-in is clamped"));
		OnZoom(FInputActionValue(-1000.f));
		Check(FMath::IsNearlyEqual(DesiredCameraDistance, MaxCameraDistance), TEXT("zoom-out is clamped"));
		OnZoom(FInputActionValue((MaxCameraDistance - IsometricDistance) / CameraZoomStep));
		ToggleTree();
		OnFireStarted();
		Check(bTreeOpen && !bWantsFire && PC->CurrentMouseCursor == EMouseCursor::Default, TEXT("tree blocks combat and restores the UI cursor"));
		OnZoom(FInputActionValue(1.f));
		Check(FMath::IsNearlyEqual(DesiredCameraDistance, IsometricDistance), TEXT("tree scroll does not zoom the game camera"));
		Shot(TEXT("02_tree"));
		break;
	case 6:
		ToggleTree();
		Check(!bTreeOpen && PC->bShowMouseCursor && PC->CurrentMouseCursor == EMouseCursor::None, TEXT("closing tree restores isometric cursor controls"));
		ToggleMap();
		OnFireStarted();
		OnMove(FInputActionValue(FVector2D(0.f, 1.f)));
		Check(bMapOpen && !bWantsFire && LastMoveInput.IsNearlyZero() && PC->CurrentMouseCursor == EMouseCursor::Default, TEXT("M opens the map and blocks combat and movement"));
		Shot(TEXT("03_map"));
		break;
	case 7:
		ToggleMap();
		Check(!bMapOpen && PC->CurrentMouseCursor == EMouseCursor::None, TEXT("M closes the map and restores combat cursor controls"));
		SetStage(EFNStage::Skeleton, false);
		Shot(TEXT("04_skeleton"));
		break;
	case 8:
		SetStage(EFNStage::Spark, false);
		Yar = MaxYar;
		UpdateCursorAim();
		FireShot();
		Check(Yar < MaxYar, TEXT("Spark ranged attack still spends Yar"));
		Shot(TEXT("05_spark"));
		break;
	case 9:
		SetStage(EFNStage::Flesh, false);
		Check(Boom->GetComponentRotation().Equals(FRotator(IsometricPitch, IsometricYaw, 0.f), 0.1f), TEXT("camera survives all evolution stages"));
		UE_LOG(LogTemp, Display, TEXT("[IsoTest] COMPLETE failures=%d"), IsometricTestFailures);
		break;
	default:
		FPlatformMisc::RequestExitWithStatus(false, IsometricTestFailures == 0 ? 0 : 1);
		break;
	}
}
