#include "FNVysiGreybox.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PostProcessVolume.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TextRenderActor.h"
#include "EngineUtils.h"
#include "FNCharacter.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/PlayerController.h"
#include "HighResScreenshot.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

namespace
{
	constexpr float M = 100.f; // metres -> cm

	UStaticMesh* Shape(const TCHAR* Name)
	{
		return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"), Name, Name));
	}

	// Ground tones get colder and darker uphill (zone "Dimness" 0.1 -> 1.0).
	FLinearColor Ground(float Dimness)
	{
		return FMath::Lerp(FLinearColor(0.34f, 0.3f, 0.22f), FLinearColor(0.16f, 0.17f, 0.2f), Dimness);
	}

	const FLinearColor Wood(0.25f, 0.18f, 0.12f);
	const FLinearColor Stone(0.3f, 0.3f, 0.3f);
	const FLinearColor Sod(0.22f, 0.26f, 0.14f);
	const FLinearColor Iron(0.12f, 0.12f, 0.13f);
	const FLinearColor Silver(0.7f, 0.75f, 0.85f);
	const FLinearColor Wax(1.f, 0.72f, 0.25f);
}

AFNVysiGreybox::AFNVysiGreybox()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

FVector AFNVysiGreybox::PlayerStart()
{
	return FVector(-30.f, -10.f, 2.f) * M;
}

FVector AFNVysiGreybox::ArenaCenter()
{
	return FVector(720.f, 0.f, 90.f) * M;
}

AActor* AFNVysiGreybox::Box(const FVector& TopCenterM, const FVector& SizeM, const FLinearColor& Color, bool bCollide)
{
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector Center = (TopCenterM - FVector(0.f, 0.f, SizeM.Z * 0.5f)) * M;
	AStaticMeshActor* A = GetWorld()->SpawnActor<AStaticMeshActor>(Center, FRotator::ZeroRotator, P);
	UStaticMeshComponent* C = A->GetStaticMeshComponent();
	C->SetMobility(EComponentMobility::Movable);
	C->SetStaticMesh(Shape(TEXT("Cube")));
	C->SetWorldScale3D(SizeM); // cube is 1 m
	if (!bCollide) { C->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
	if (UMaterialInstanceDynamic* MID = C->CreateDynamicMaterialInstance(0)) { MID->SetVectorParameterValue(TEXT("Color"), Color); }
	return A;
}

AActor* AFNVysiGreybox::Ramp(const FVector& FromM, const FVector& ToM, float WidthM, const FLinearColor& Color)
{
	constexpr float ThickM = 1.f;
	const FVector Dir = ToM - FromM;
	const FRotator Rot = Dir.Rotation();
	const FVector Up = FRotationMatrix(Rot).GetUnitAxis(EAxis::Z);
	const FVector CenterM = (FromM + ToM) * 0.5f - Up * (ThickM * 0.5f);

	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AStaticMeshActor* A = GetWorld()->SpawnActor<AStaticMeshActor>(CenterM * M, Rot, P);
	UStaticMeshComponent* C = A->GetStaticMeshComponent();
	C->SetMobility(EComponentMobility::Movable);
	C->SetStaticMesh(Shape(TEXT("Cube")));
	C->SetWorldScale3D(FVector(Dir.Size() + 1.f, WidthM, ThickM)); // +1 m overlap hides seams
	if (UMaterialInstanceDynamic* MID = C->CreateDynamicMaterialInstance(0)) { MID->SetVectorParameterValue(TEXT("Color"), Color); }
	return A;
}

AActor* AFNVysiGreybox::Cyl(const FVector& CenterM, float DiameterM, float HeightM, const FLinearColor& Color, bool bCollide)
{
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AStaticMeshActor* A = GetWorld()->SpawnActor<AStaticMeshActor>(CenterM * M, FRotator::ZeroRotator, P);
	UStaticMeshComponent* C = A->GetStaticMeshComponent();
	C->SetMobility(EComponentMobility::Movable);
	C->SetStaticMesh(Shape(TEXT("Cylinder")));
	C->SetWorldScale3D(FVector(DiameterM, DiameterM, HeightM));
	if (!bCollide) { C->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
	if (UMaterialInstanceDynamic* MID = C->CreateDynamicMaterialInstance(0)) { MID->SetVectorParameterValue(TEXT("Color"), Color); }
	return A;
}

AActor* AFNVysiGreybox::Ball(const FVector& CenterM, float DiameterM, const FLinearColor& Color)
{
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AStaticMeshActor* A = GetWorld()->SpawnActor<AStaticMeshActor>(CenterM * M, FRotator::ZeroRotator, P);
	UStaticMeshComponent* C = A->GetStaticMeshComponent();
	C->SetMobility(EComponentMobility::Movable);
	C->SetStaticMesh(Shape(TEXT("Sphere")));
	C->SetWorldScale3D(FVector(DiameterM));
	if (UMaterialInstanceDynamic* MID = C->CreateDynamicMaterialInstance(0)) { MID->SetVectorParameterValue(TEXT("Color"), Color); }
	return A;
}

void AFNVysiGreybox::Label(const FVector& PosM, const FString& Text, const FColor& Color)
{
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (ATextRenderActor* T = GetWorld()->SpawnActor<ATextRenderActor>(PosM * M, FRotator(0.f, 180.f, 0.f), P))
	{
		UTextRenderComponent* R = T->GetTextRender();
		R->SetText(FText::FromString(Text));
		R->SetTextRenderColor(Color);
		R->SetWorldSize(80.f);
		R->SetHorizontalAlignment(EHTA_Center);
	}
}

void AFNVysiGreybox::LootColumn(const FVector& PosM)
{
	Cyl(PosM + FVector(0.f, 0.f, 6.f), 0.2f, 12.f, Wax, false);
}

void AFNVysiGreybox::Build(ADirectionalLight* InSun)
{
	Sun = InSun;

	// With an imported Landscape the terrain pieces (floor, masses, ramps, plateaus) are skipped;
	// props, landmarks and labels stay. Heights match the heightmap (tools/heightmap).
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->GetClass()->GetName().Contains(TEXT("Landscape")))
		{
			bHasLandscape = true;
			break;
		}
	}

	// With an imported Landscape the terrain pieces (floor, masses, ramps, plateaus) are skipped;
	// props, landmarks and labels stay. Heights match the heightmap (tools/heightmap).
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->GetClass()->GetName().Contains(TEXT("Landscape")))
		{
			bHasLandscape = true;
			break;
		}
	}

	// ---------- Mountain mass: valley floor + stepped body under the path, so it reads as one slope ----------
	if (!bHasLandscape) { Box(FVector(420.f, 0.f, -4.f), FVector(1400.f, 900.f, 2.f), FLinearColor(0.2f, 0.19f, 0.16f)); }
	struct FMass { FVector TopM; FVector SizeM; };
	const FMass Masses[] = {
		{ FVector(95.f, 25.f, 9.f),    FVector(90.f, 60.f, 13.f) },   // Okolitsa shoulder
		{ FVector(160.f, 40.f, 17.f),  FVector(60.f, 80.f, 21.f) },   // Oath Stone ledge
		{ FVector(235.f, 5.f, 21.f),   FVector(80.f, 110.f, 25.f) },
		{ FVector(310.f, -20.f, 30.f), FVector(70.f, 110.f, 34.f) },  // Strelokopni steps
		{ FVector(370.f, -20.f, 42.f), FVector(60.f, 110.f, 46.f) },
		{ FVector(430.f, -10.f, 53.f), FVector(70.f, 110.f, 57.f) },
		{ FVector(500.f, 10.f, 58.f),  FVector(80.f, 190.f, 62.f) },  // Bucket Row shelf
		{ FVector(560.f, 20.f, 64.f),  FVector(50.f, 190.f, 68.f) },
		{ FVector(610.f, 10.f, 70.f),  FVector(60.f, 90.f, 74.f) },
		{ FVector(655.f, 5.f, 77.f),   FVector(40.f, 60.f, 81.f) },   // treba
		{ FVector(720.f, 0.f, 87.f),   FVector(60.f, 60.f, 91.f) },   // hill under the arena
		{ FVector(790.f, 0.f, 100.f),  FVector(60.f, 30.f, 104.f) },  // ridge spine
		{ FVector(860.f, 5.f, 116.f),  FVector(50.f, 25.f, 120.f) },
	};
	for (const FMass& Mass : Masses)
	{
		if (!bHasLandscape) { Box(Mass.TopM, Mass.SizeM, FLinearColor(0.22f, 0.21f, 0.18f)); }
	}

	// ---------- 1. Sukhorechye (village) + Okolitsa. Dimness 0.1 ----------
	if (!bHasLandscape) { Box(FVector(0.f, 0.f, 0.f), FVector(90.f, 80.f, 4.f), Ground(0.1f)); }
	if (!bHasLandscape) { Box(FVector(0.f, -40.f, -2.f), FVector(90.f, 8.f, 1.f), Ground(0.15f)); } // dry riverbed
	if (!bHasLandscape) { Box(FVector(-45.f, 0.f, 6.f), FVector(1.f, 80.f, 6.f), Ground(0.2f)); } // southern clay wall (edge of the ring)
	const FVector Houses[] = { {-25, 25, 0}, {-10, 30, 0}, {8, 28, 0}, {25, 22, 0}, {-28, -15, 0}, {20, -18, 0}, {30, 5, 0} };
	for (const FVector& H : Houses)
	{
		Box(H + FVector(0.f, 0.f, 3.f), FVector(6.f, 5.f, 3.f), Wood);
		Box(H + FVector(0.f, 0.f, 4.f), FVector(6.4f, 5.4f, 1.f), Sod);              // sod roof
		Cyl(H + FVector(3.5f, 0.f, 2.f), 0.3f, 4.f, Iron);                            // thunder-vein pole
	}
	Cyl(FVector(0.f, 0.f, 0.5f), 2.f, 1.f, Stone);                                   // well
	Label(FVector(0.f, 0.f, 4.f), TEXT("SUKHORECHYE"), FColor(255, 220, 170));

	if (!bHasLandscape) { Ramp(FVector(40.f, 15.f, 0.f), FVector(140.f, 38.f, 18.f), 16.f, Ground(0.18f)); } // Okolitsa corridor
	Label(FVector(110.f, 30.f, 19.f), TEXT("SPARK -> SKELETON (5 kills)"), FColor(200, 200, 255));

	// ---------- 2. Oath Stone. Dimness 0.25 ----------
	if (!bHasLandscape) { Box(FVector(160.f, 40.f, 18.f), FVector(42.f, 42.f, 4.f), Ground(0.25f)); }
	Box(FVector(160.f, 40.f, 18.8f), FVector(3.f, 2.f, 0.8f), Stone);                // oath stone
	if (!bHasLandscape) { Ball(FVector(175.f, 70.f, 18.f), 14.f, Sod); } // burial mound (half-buried)
	LootColumn(FVector(158.f, 36.f, 18.f));
	Label(FVector(160.f, 40.f, 22.f), TEXT("OATH STONE - WEAPON 1"), FColor(255, 200, 120));

	// ---------- 3. Strelokopni. Dimness 0.4 ----------
	if (!bHasLandscape) { Ramp(FVector(180.f, 40.f, 18.f), FVector(280.f, -20.f, 25.f), 14.f, Ground(0.32f)); }
	if (!bHasLandscape) { Ramp(FVector(280.f, -20.f, 25.f), FVector(420.f, -20.f, 55.f), 90.f, Ground(0.4f)); } // the dug-up slope
	for (int32 i = 0; i < 12; ++i)                                                        // pit rims (craters)
	{
		const float X = 300.f + (i % 4) * 30.f;
		const float Y = -55.f + (i / 4) * 30.f + ((i % 2) ? 8.f : -6.f);
		const float Z = 25.f + (X - 280.f) * (30.f / 140.f);
		if (!bHasLandscape) { Cyl(FVector(X, Y, Z + 0.4f), 3.f + (i % 3), 0.8f, Ground(0.55f)); }
	}
	for (int32 i = 0; i < 20; ++i)                                                        // thunder-arrows sticking out
	{
		const float X = 295.f + FMath::Fmod(i * 37.f, 120.f);
		const float Y = -60.f + FMath::Fmod(i * 23.f, 80.f);
		const float Z = 25.f + (X - 280.f) * (30.f / 140.f);
		Cyl(FVector(X, Y, Z + 0.7f), 0.12f, 1.4f, Iron, false);
	}
	Box(FVector(360.f, 10.f, 48.f), FVector(4.f, 4.f, 0.3f), Wood);                   // shelter roof
	for (const FVector& Leg : { FVector(358.5f, 8.5f, 46.f), FVector(361.5f, 8.5f, 46.f), FVector(358.5f, 11.5f, 46.f), FVector(361.5f, 11.5f, 46.f) })
	{
		Cyl(Leg, 0.2f, 4.f, Wood);
	}
	Label(FVector(360.f, 10.f, 50.f), TEXT("SKELETON -> FLESH (5 kills)"), FColor(200, 200, 255));
	if (!bHasLandscape) { Ramp(FVector(245.f, -50.f, 22.f), FVector(160.f, 20.f, 18.f), 3.f, Ground(0.3f)); } // shortcut crawl back to the Oath Stone
	Label(FVector(230.f, -40.f, 24.f), TEXT("SHORTCUT"), FColor(180, 180, 180));
	if (!bHasLandscape) { Box(FVector(435.f, 0.f, 55.f), FVector(20.f, 30.f, 3.f), Ground(0.45f)); }
	Label(FVector(430.f, 0.f, 59.f), TEXT("TREE CHOICE 1"), FColor(255, 200, 120));

	// ---------- 4. Bucket Row. Dimness 0.6 ----------
	if (!bHasLandscape) { Ramp(FVector(445.f, 0.f, 55.f), FVector(525.f, 10.f, 65.f), 20.f, Ground(0.5f)); }
	if (!bHasLandscape) { Box(FVector(540.f, 20.f, 65.f), FVector(35.f, 170.f, 4.f), Ground(0.6f)); }
	for (int32 i = 0; i < 8; ++i)                                                      // wells with sweeps and buckets
	{
		const float Y = -60.f + i * 22.f;
		Box(FVector(548.f, Y, 66.f), FVector(2.f, 2.f, 1.f), Wood);
		Cyl(FVector(550.f, Y, 68.f), 0.2f, 6.f, Wood);
		Cyl(FVector(548.f, Y, 68.5f), 0.4f, 0.5f, Iron);                              // bucket
	}
	Box(FVector(528.f, -35.f, 66.f), FVector(20.f, 15.f, 1.f), Wood, false);          // goat pen (low fence plate)
	Box(FVector(520.f, 60.f, 69.f), FVector(6.f, 8.f, 4.f), Wood);                    // gear shed
	LootColumn(FVector(516.f, 60.f, 65.f));
	Label(FVector(520.f, 60.f, 72.f), TEXT("SHED - WEAPON 2 + ARMOR"), FColor(255, 200, 120));
	Label(FVector(620.f, 20.f, 78.f), TEXT("TREE CHOICE 2"), FColor(255, 200, 120));

	// Climb to the treba and the hill.
	if (!bHasLandscape) { Ramp(FVector(555.f, 20.f, 65.f), FVector(640.f, 10.f, 78.f), 8.f, Ground(0.65f)); }
	if (!bHasLandscape) { Box(FVector(647.f, 10.f, 78.f), FVector(14.f, 14.f, 3.f), Ground(0.7f)); }
	Cyl(FVector(645.f, 10.f, 79.f), 1.5f, 2.f, Silver);                              // treba (moon silver)
	Label(FVector(645.f, 10.f, 82.f), TEXT("TREBA"), FColor(200, 215, 255));

	// ---------- 5. Kumirnaya Hill: exam arena. Dimness 0.8 ----------
	if (!bHasLandscape) { Ramp(FVector(652.f, 10.f, 78.f), FVector(698.f, -8.f, 90.f), 5.f, Ground(0.75f)); }
	if (!bHasLandscape) { Cyl(FVector(720.f, 0.f, 88.f), 44.f, 4.f, Ground(0.8f)); } // arena disc, top at Z 90
	Cyl(FVector(720.f, 0.f, 90.75f), 1.2f, 1.5f, FLinearColor(0.05f, 0.05f, 0.05f)); // charred stump (landmark, not cover)
	for (int32 i = 0; i < 32; ++i)                                                     // invisible wall at R 24 (gap at the entrance)
	{
		const float A = i * 2.f * PI / 32.f;
		const FVector Dir(FMath::Cos(A), FMath::Sin(A), 0.f);
		if (FVector::DotProduct(Dir, FVector(-0.95f, -0.3f, 0.f).GetSafeNormal()) > 0.97f) { continue; } // entrance
		if (FVector::DotProduct(Dir, FVector(1.f, 0.f, 0.f)) > 0.98f) { continue; }                         // exit to the ridge
		AActor* W = Box(FVector(720.f, 0.f, 90.f) + Dir * 24.f + FVector(0.f, 0.f, 6.f), FVector(4.8f, 4.8f, 6.f), FLinearColor::Black);
		W->SetActorHiddenInGame(true);
	}
	Label(FVector(705.f, -5.f, 96.f), TEXT("EXAM ARENA"), FColor(255, 120, 120));

	// ---------- 6. Thunder Ridge. Dimness 1.0 ----------
	if (!bHasLandscape) { Ramp(FVector(742.f, 0.f, 90.f), FVector(850.f, 0.f, 118.f), 6.f, Ground(0.9f)); }
	if (!bHasLandscape) { Box(FVector(867.f, 5.f, 118.f), FVector(35.f, 20.f, 3.f), Ground(1.f)); }
	Cyl(FVector(860.f, 0.f, 129.f), 3.f, 22.f, Wood);                                // oak trunk
	Ball(FVector(860.f, 0.f, 142.f), 16.f, FLinearColor(0.12f, 0.16f, 0.1f));       // crown
	for (int32 i = 0; i < 5; ++i)
	{
		Cyl(FVector(860.f, 0.f, 121.f + i * 3.f), 3.3f, 0.3f, Iron, false);          // iron hoops
	}
	if (AActor* Portal = Cyl(FVector(875.f, 15.f, 121.f), 5.f, 0.3f, FLinearColor(0.35f, 0.4f, 0.9f), false))
	{
		Portal->SetActorRotation(FRotator(90.f, 0.f, 0.f));                          // vertical ring stand-in
	}
	Label(FVector(875.f, 15.f, 125.f), TEXT("PORTAL (to be continued)"), FColor(180, 190, 255));

	// Lightning flash in the oak every 12 s — the compass and clock of the Heights.
	OakFlash = NewObject<UPointLightComponent>(this, TEXT("OakFlash"));
	OakFlash->SetupAttachment(RootComponent);
	OakFlash->RegisterComponent();
	OakFlash->SetWorldLocation(FVector(860.f, 0.f, 150.f) * M);
	OakFlash->SetLightColor(FLinearColor(0.6f, 0.65f, 1.f));
	OakFlash->SetAttenuationRadius(60000.f);
	OakFlash->SetIntensity(0.f);

	SetupAtmosphere();
}

void AFNVysiGreybox::SetupAtmosphere()
{
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Low golden sun, the storm front sits upper-left of the climb (style_v0.1: Vysi motif).
	if (Sun)
	{
		Sun->SetActorRotation(FRotator(-24.f, 60.f, 0.f));
	}

	// Height fog with volumetric scattering: dusty haze in the valleys, clearer toward the ridge.
	for (TActorIterator<AExponentialHeightFog> It(GetWorld()); It; ++It)
	{
		UExponentialHeightFogComponent* Fog = It->GetComponent();
		Fog->SetFogDensity(0.018f);
		Fog->SetFogHeightFalloff(0.04f);
		Fog->SetFogInscatteringColor(FLinearColor(0.45f, 0.5f, 0.6f));
		Fog->SetDirectionalInscatteringColor(FLinearColor(0.9f, 0.7f, 0.45f));
		Fog->SetVolumetricFog(true);
		break;
	}

	// Volumetric clouds from the engine's simple cloud material.
	if (AVolumetricCloud* Clouds = GetWorld()->SpawnActor<AVolumetricCloud>(FVector::ZeroVector, FRotator::ZeroRotator, P))
	{
		if (UMaterialInterface* CloudMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineSky/VolumetricClouds/m_SimpleVolumetricCloud_Inst.m_SimpleVolumetricCloud_Inst")))
		{
			if (UVolumetricCloudComponent* C = Clouds->FindComponentByClass<UVolumetricCloudComponent>()) { C->SetMaterial(CloudMat); }
		}
	}

	// Global post process: manual exposure + grading; values follow the climb in Tick.
	Grade = GetWorld()->SpawnActor<APostProcessVolume>(FVector::ZeroVector, FRotator::ZeroRotator, P);
	if (Grade)
	{
		Grade->bUnbound = true;
		FPostProcessSettings& S = Grade->Settings;
		S.bOverride_AutoExposureMethod = true;
		S.AutoExposureMethod = AEM_Manual;
		S.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
		S.AutoExposureApplyPhysicalCameraExposure = false;
		S.bOverride_AutoExposureBias = true;
		S.bOverride_WhiteTemp = true;
		S.bOverride_ColorSaturation = true;
		S.bOverride_ColorContrast = true;
		S.ColorContrast = FVector4(1.08f, 1.08f, 1.08f, 1.f);
		S.bOverride_VignetteIntensity = true;
		S.VignetteIntensity = 0.45f;
		S.bOverride_FilmGrainIntensity = true;
		S.FilmGrainIntensity = 0.06f;
	}
}

void AFNVysiGreybox::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// "-AutoShot": fly through key viewpoints, save screenshots to Saved/Screenshots, then quit (for remote review).
	if (FParse::Param(FCommandLine::Get(), TEXT("AutoShot")))
	{
		struct FShot { FVector PosM; float Yaw; float Pitch; const TCHAR* Name; };
		static const FShot Shots[] = {
			{ FVector(-30.f, -10.f, 2.f), 20.f, -5.f, TEXT("01_village") },
			{ FVector(140.f, 20.f, 20.f), 10.f, -5.f, TEXT("02_oath_stone") },
			{ FVector(290.f, -20.f, 30.f), 0.f, 0.f, TEXT("03_strelokopni") },
			{ FVector(500.f, 0.f, 67.f), 10.f, 0.f, TEXT("04_bucket_row") },
			{ FVector(660.f, 5.f, 80.f), 0.f, 3.f, TEXT("05_treba_arena") },
			{ FVector(760.f, -10.f, 97.f), 5.f, 8.f, TEXT("06_ridge_oak") },
		};
		constexpr int32 NumShots = static_cast<int32>(UE_ARRAY_COUNT(Shots));
		ShotClock = GetWorld()->GetRealTimeSeconds(); // real time: the tree shot slows game time
		const int32 Idx = FMath::FloorToInt((ShotClock - 15.f) / 5.f);
		APlayerController* PC = GetWorld()->GetFirstPlayerController();
		if (PC && PC->GetPawn() && Idx >= 0 && Idx < NumShots)
		{
			const FShot& S = Shots[Idx];
			PC->GetPawn()->SetActorLocation(S.PosM * M + FVector(0.f, 0.f, 100.f), false, nullptr, ETeleportType::TeleportPhysics);
			PC->SetControlRotation(FRotator(S.Pitch, S.Yaw, 0.f));
			const float Local = FMath::Fmod(ShotClock - 15.f, 5.f);
			if (Local > 3.5f && ShotTaken != Idx)
			{
				ShotTaken = Idx;
				FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots") / FString(S.Name) + TEXT(".png"), false, false);
			}
		}
		if (Idx == NumShots && PC)
		{
			if (AFNCharacter* Hero = Cast<AFNCharacter>(PC->GetPawn()))
			{
				if (!Hero->IsTreeOpen()) { Hero->FindRune(3); Hero->ToggleTree(); }
				if (FMath::Fmod(ShotClock - 15.f, 5.f) > 3.5f && ShotTaken != Idx)
				{
					ShotTaken = Idx;
					FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots") / TEXT("07_tree.png"), false, false);
				}
			}
		}
		if (Idx >= NumShots + 1 && PC)
		{
			PC->ConsoleCommand(TEXT("quit"));
		}
	}

	// Dimness follows the climb: warm morning below, twilight without warmth on the ridge.
	float PlayerX = 0.f;
	for (TActorIterator<AFNCharacter> It(GetWorld()); It; ++It)
	{
		PlayerX = It->GetActorLocation().X / M;
		break;
	}
	const float Dim = FMath::Clamp(PlayerX / 860.f, 0.f, 1.f);
	if (Sun)
	{
		UDirectionalLightComponent* L = CastChecked<UDirectionalLightComponent>(Sun->GetLightComponent());
		L->SetIntensity(FMath::Lerp(6.f, 3.f, Dim));
		L->SetLightColor(FMath::Lerp(FLinearColor(1.f, 0.8f, 0.55f), FLinearColor(0.7f, 0.76f, 0.95f), Dim));
	}
	if (Grade)
	{
		FPostProcessSettings& S = Grade->Settings;
		S.AutoExposureBias = FMath::Lerp(0.2f, 0.7f, Dim);                       // tame the bright valley, keep the ridge readable
		S.WhiteTemp = FMath::Lerp(7200.f, 5600.f, Dim);                              // warm below, cold above
		const float Sat = FMath::Lerp(0.95f, 0.72f, Dim);                            // colour drains with the memory
		S.ColorSaturation = FVector4(Sat, Sat, Sat, 1.f);
	}

	LightningTimer -= DeltaSeconds;
	if (LightningTimer <= 0.f && OakFlash)
	{
		LightningTimer = 12.f;
		FlashRemaining = 0.12f;
		OakFlash->SetIntensity(2000000.f);

		// Visible bolt: jagged blue-violet line from the clouds into the oak's crown (never pure white: GDD §9).
		FVector Prev = FVector(860.f, 0.f, 420.f) * M;
		const FVector Target = FVector(860.f, 0.f, 150.f) * M;
		for (int32 i = 1; i <= 7; ++i)
		{
			const float A = i / 7.f;
			FVector Next = FMath::Lerp(FVector(860.f, 0.f, 420.f) * M, Target, A);
			if (i < 7) { Next += FVector(FMath::FRandRange(-1500.f, 1500.f), FMath::FRandRange(-1500.f, 1500.f), 0.f); }
			DrawDebugLine(GetWorld(), Prev, Next, FColor(170, 180, 255), false, 0.15f, 0, 60.f);
			Prev = Next;
		}
	}
	if (FlashRemaining > 0.f)
	{
		FlashRemaining -= DeltaSeconds;
		if (FlashRemaining <= 0.f && OakFlash)
		{
			OakFlash->SetIntensity(0.f);
		}
	}
}
