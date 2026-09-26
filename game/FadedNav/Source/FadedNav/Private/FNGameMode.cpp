#include "FNGameMode.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMeshActor.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Engine/World.h"
#include "FNCharacter.h"
#include "FNHUD.h"
#include "FNPerunBoss.h"
#include "GameFramework/PlayerStart.h"
#include "Materials/MaterialInstanceDynamic.h"

AFNGameMode::AFNGameMode()
{
	DefaultPawnClass = AFNCharacter::StaticClass();
	HUDClass = AFNHUD::StaticClass();
}

void AFNGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	BuildArena();
}

AActor* AFNGameMode::FindPlayerStart_Implementation(AController* Player, const FString& IncomingName)
{
	if (!SpawnPoint)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnPoint = GetWorld()->SpawnActor<APlayerStart>(FVector(-1800.f, 0.f, 120.f), FRotator::ZeroRotator, Params);
	}
	return SpawnPoint;
}

void AFNGameMode::BuildArena()
{
	UWorld* World = GetWorld();
	UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	auto SpawnMesh = [&](UStaticMesh* Mesh, const FVector& Loc, const FVector& Scale, const FLinearColor& Color)
	{
		AStaticMeshActor* A = World->SpawnActor<AStaticMeshActor>(Loc, FRotator::ZeroRotator, Params);
		UStaticMeshComponent* C = A->GetStaticMeshComponent();
		C->SetMobility(EComponentMobility::Movable);
		C->SetStaticMesh(Mesh);
		C->SetWorldScale3D(Scale);
		if (UMaterialInstanceDynamic* MID = C->CreateDynamicMaterialInstance(0))
		{
			MID->SetVectorParameterValue(TEXT("Color"), Color);
		}
		return A;
	};

	// Floor: 80 m plateau ("Thunder Heights" grey-box).
	SpawnMesh(Plane, FVector::ZeroVector, FVector(80.f, 80.f, 1.f), FLinearColor(0.18f, 0.19f, 0.2f));

	// Oak lightning-rods / pillars for spatial reference (low saturation environment, style §2).
	const FLinearColor PillarColor(0.22f, 0.2f, 0.18f);
	for (int32 i = 0; i < 8; ++i)
	{
		const float Angle = i * PI / 4.f;
		const FVector P(FMath::Cos(Angle) * 2600.f, FMath::Sin(Angle) * 2600.f, 400.f);
		SpawnMesh(Cylinder, P, FVector(1.2f, 1.2f, 8.f), PillarColor);
	}
	SpawnMesh(Cube, FVector(-900.f, 700.f, 100.f), FVector(2.f, 4.f, 2.f), PillarColor);
	SpawnMesh(Cube, FVector(-900.f, -700.f, 100.f), FVector(2.f, 4.f, 2.f), PillarColor);

	// Light: low sun + sky atmosphere + real-time sky light + fog ("twilight without darkness").
	if (ADirectionalLight* Sun = World->SpawnActor<ADirectionalLight>(FVector(0.f, 0.f, 1000.f), FRotator(-25.f, 35.f, 0.f), Params))
	{
		UDirectionalLightComponent* L = CastChecked<UDirectionalLightComponent>(Sun->GetLightComponent());
		L->SetMobility(EComponentMobility::Movable);
		L->SetIntensity(6.f);
		L->SetLightColor(FLinearColor(1.f, 0.86f, 0.7f));
		L->SetAtmosphereSunLight(true);
	}

	World->SpawnActor<ASkyAtmosphere>(FVector::ZeroVector, FRotator::ZeroRotator, Params);

	if (ASkyLight* Sky = World->SpawnActor<ASkyLight>(FVector(0.f, 0.f, 500.f), FRotator::ZeroRotator, Params))
	{
		USkyLightComponent* S = Sky->GetLightComponent();
		S->SetMobility(EComponentMobility::Movable);
		S->bRealTimeCapture = true;
		S->SetIntensity(1.f);
		S->RecaptureSky();
	}

	World->SpawnActor<AExponentialHeightFog>(FVector::ZeroVector, FRotator::ZeroRotator, Params);

	// Perun, the mentor.
	World->SpawnActor<AFNPerunBoss>(FVector(1200.f, 0.f, 200.f), FRotator(0.f, 180.f, 0.f), Params);
}
