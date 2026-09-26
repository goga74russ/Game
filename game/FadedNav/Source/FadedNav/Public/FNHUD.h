#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "FNHUD.generated.h"

// Tech-test HUD drawn on Canvas. Labels are ASCII until a Cyrillic UI font is set up (UMG, stage 1).
UCLASS()
class FADEDNAV_API AFNHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;
	virtual void NotifyHitBoxClick(FName BoxName) override;
	virtual void NotifyHitBoxBeginCursorOver(FName BoxName) override;
	virtual void NotifyHitBoxEndCursorOver(FName BoxName) override;

	// Exam outcome A "almost won": boss HP at or below this ratio when the player falls [D].
	UPROPERTY(EditAnywhere, Category = "Exam")
	float NearWinRatio = 0.3f;

private:
	void DrawBar(float X, float Y, float W, float H, float Ratio, const FLinearColor& Fill);
	void DrawCentered(const FString& Text, float Y, const FLinearColor& Color, float Scale);
	void DrawTree(class AFNCharacter* Player);
	int32 HoveredNode = -1;
};
