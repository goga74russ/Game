#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "FNInventory.h"
#include "FNInventoryPreview.generated.h"

// Isolated QA fixture, selected explicitly with ?game=...FNInventoryPreviewGameMode.
// It never connects to the user's editor or reads/writes the live hero.
UCLASS()
class FADEDNAV_API AFNInventoryPreviewHUD : public AHUD
{
	GENERATED_BODY()
public:
	virtual void DrawHUD() override;
private:
	FFNInventoryScreen Screen;
	UPROPERTY() TArray<TObjectPtr<UTexture2D>> Icons;
	bool bInitialized = false;
	bool bCaptured = false;
};

UCLASS()
class FADEDNAV_API AFNInventoryPreviewGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	AFNInventoryPreviewGameMode();
};
