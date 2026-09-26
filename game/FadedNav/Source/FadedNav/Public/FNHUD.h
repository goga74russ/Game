#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "FNHUD.generated.h"

// Chapter HUD on Canvas, layout after Remnant 2 (reference only, no assets): stage + health bottom-left,
// weapon bottom-right, zone + skill points top-right, boss bar and subtitles bottom-centre. Russian text.
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
	void DrawPanel(float X, float Y, float PW, float PH);
	int32 HoveredNode = -1;
};
