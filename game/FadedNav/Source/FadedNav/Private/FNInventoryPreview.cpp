#include "FNInventoryPreview.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "ImageUtils.h"
#include "HighResScreenshot.h"
#include "HAL/PlatformMisc.h"
#include "Misc/Paths.h"

AFNInventoryPreviewGameMode::AFNInventoryPreviewGameMode()
{
	HUDClass = AFNInventoryPreviewHUD::StaticClass();
	DefaultPawnClass = nullptr;
}

void AFNInventoryPreviewHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas || !FParse::Param(FCommandLine::Get(), TEXT("InventoryPreview"))) { return; }
	if (!bInitialized)
	{
		bInitialized = true;
		FString IconDir;
		FParse::Value(FCommandLine::Get(), TEXT("InventoryIconDir="), IconDir);
		const TCHAR* Names[] = { TEXT("thunder_strike"), TEXT("lightning_rod"), TEXT("flare"), TEXT("chain_spark"), TEXT("storm_ward") };
		for (const TCHAR* Name : Names)
		{
			Icons.Add(IconDir.IsEmpty() ? nullptr : FImageUtils::ImportFileAsTexture2D(IconDir / (FString(Name) + TEXT(".png"))));
		}
	}
	FFNInventorySnapshot State;
	State.OwnedSkills = { 0, 1, 2, 3, 4 };
	State.Panel = { 0, 2, 4 };
	State.bCanEdit = true;
	State.Equipment = { TEXT("Ближнее: колун"), TEXT("Ружьё — в руках"), TEXT("Дробовик — в запасе"), TEXT("Доспех: +20 здоровья") };
	State.Satchel = { TEXT("Громовая стрела"), TEXT("Молоко") };
	TMap<int32, UTexture2D*> TextureMap;
	for (int32 I = 0; I < Icons.Num(); ++I) { if (Icons[I]) { TextureMap.Add(I, Icons[I]); } }
	Screen.Draw(Canvas, State, GEngine->GetMediumFont(), GEngine->GetSmallFont(), TextureMap);
	const float Scale = FMath::Min(Canvas->ClipX / 1280.f, Canvas->ClipY / 720.f);
	const FVector2D Origin((Canvas->ClipX - 1180.f * Scale) / 2.f, (Canvas->ClipY - 640.f * Scale) / 2.f);
	Screen.Click(Origin + FVector2D(400, 150) * Scale, State, [](const TArray<int32>&, FString&) { return false; });
	if (!bCaptured && GetWorld()->GetTimeSeconds() > 3.f)
	{
		bCaptured = true;
		FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/Inventory/preview.png"), false, false);
	}
	if (GetWorld()->GetTimeSeconds() > 5.f) { FPlatformMisc::RequestExitWithStatus(false, 0); }
}
