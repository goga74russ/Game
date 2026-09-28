#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "FNHUD.generated.h"

class UFont;

// Chapter HUD on Canvas, look from docs/ui/hud_mockup_v1.html (approved 2026-09-26): Ruslan Display titles,
// Cormorant Garamond text (OFL), ember health, bevelled slots, gold ornament lines. Layout designed at 1280x720, scaled.
UCLASS()
class FADEDNAV_API AFNHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	// Exam outcome A "almost won": boss HP at or below this ratio when the player falls [D].
	UPROPERTY(EditAnywhere, Category = "Exam")
	float NearWinRatio = 0.3f;

private:
	enum class EAlign : uint8 { Left, Center, Right };

	// Text with a soft dark shadow; X is the anchor for the alignment. Sizes are in 720p pixels.
	void Txt(const FString& Text, float X, float Y, UFont* Font, const FLinearColor& Color, EAlign Align = EAlign::Left, float Scale = 1.f);
	void DrawCentered(const FString& Text, float Y, const FLinearColor& Color, float Scale);
	void FillTri(const FVector2D& A, const FVector2D& B, const FVector2D& C, const FLinearColor& Col);
	void FillBevel(float X, float Y, float W, float H, float Cut, const FLinearColor& Col);
	void LineBevel(float X, float Y, float W, float H, float Cut, const FLinearColor& Col, float Thick);
	void FillDisc(float CX, float CY, float R, const FLinearColor& C);
	void DrawRing(float CX, float CY, float R, const FLinearColor& C, float Thickness);
	void GradRect(float X, float Y, float W, float H, const FLinearColor& Top, const FLinearColor& Bottom);
	void Ornament(float CX, float Y, float HalfW);
	void DrawTree(class AFNCharacter* Player);
	void DrawMap(class AFNCharacter* Player);
	void TreeInput(class AFNCharacter* Player);

	// Skill tree camera (tree space -> screen): pan in tree units, zoom as pixels per unit at 720p.
	FVector2D TreePan = FVector2D(0.f, 560.f);
	float TreeZoom = 0.45f;
	bool bTreeLmbDown = false, bTreeDragging = false;
	FVector2D TreeDragStart = FVector2D::ZeroVector, TreePanStart = FVector2D::ZeroVector;

	// Faces: 0 = Ruslan Display (titles), 1 = Cormorant Bold, 2 = Cormorant SemiBold (text), 3 = Cormorant Medium Italic.
	// Fonts are built per pixel size at the current scale, so glyphs stay crisp at any resolution.
	UFont* Font(int32 Face, float Px720);
	UPROPERTY() TArray<TObjectPtr<UObject>> Faces;
	UPROPERTY() TMap<int32, TObjectPtr<UFont>> FontCache;

	float S = 1.f; // UI scale = viewport height / 720
	float ShownHealth = -1.f;
	float ShownBoss = -1.f;
	int32 HoveredNode = -1;
};
