#include "FNHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "EngineUtils.h"
#include "FNCharacter.h"
#include "FNGameMode.h"
#include "FNHealthComponent.h"
#include "FNPerunBoss.h"

void AFNHUD::DrawBar(float X, float Y, float W, float H, float Ratio, const FLinearColor& Fill)
{
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), X - 2.f, Y - 2.f, W + 4.f, H + 4.f);
	DrawRect(Fill, X, Y, W * FMath::Clamp(Ratio, 0.f, 1.f), H);
}

void AFNHUD::DrawCentered(const FString& Text, float Y, const FLinearColor& Color, float Scale)
{
	UFont* Font = GEngine->GetLargeFont();
	float TW = 0.f, TH = 0.f;
	GetTextSize(Text, TW, TH, Font, Scale);
	DrawText(Text, Color, (Canvas->ClipX - TW) * 0.5f, Y, Font, Scale);
}

void AFNHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas)
	{
		return;
	}

	const float W = Canvas->ClipX;
	const float H = Canvas->ClipY;
	UFont* Small = GEngine->GetSmallFont();
	UFont* Medium = GEngine->GetMediumFont();

	AFNCharacter* Player = Cast<AFNCharacter>(GetOwningPawn());
	AFNPerunBoss* Boss = nullptr;
	for (TActorIterator<AFNPerunBoss> It(GetWorld()); It; ++It)
	{
		Boss = *It;
		break;
	}

	// Controls hint
	DrawText(TEXT("WASD move | Mouse aim | LMB fire | RMB aim | Space roll | F melee | R reload | Enter restart"),
		FLinearColor(0.8f, 0.8f, 0.8f, 0.8f), 20.f, 16.f, Small);

	if (Player)
	{
		// Crosshair: flashes on hit, bigger on weak point.
		const float Since = Player->GetTimeSinceHit();
		const bool bHitFlash = Since < 0.12f;
		const float S = bHitFlash && Player->WasLastHitWeak() ? 12.f : 7.f;
		const FLinearColor CH = bHitFlash ? FLinearColor(1.f, 0.8f, 0.3f) : FLinearColor::White;
		const float CX = W * 0.5f, CY = H * 0.5f;
		DrawRect(CH, CX - S - 6.f, CY - 1.f, S, 2.f);
		DrawRect(CH, CX + 6.f, CY - 1.f, S, 2.f);
		DrawRect(CH, CX - 1.f, CY - S - 6.f, 2.f, S);
		DrawRect(CH, CX - 1.f, CY + 6.f, 2.f, S);

		// Health / stamina
		DrawText(TEXT("HP"), FLinearColor::White, 40.f, H - 92.f, Small);
		DrawBar(70.f, H - 90.f, 320.f, 14.f, Player->GetHealth()->GetRatio(), FLinearColor(0.75f, 0.2f, 0.15f));
		DrawText(TEXT("ST"), FLinearColor::White, 40.f, H - 64.f, Small);
		DrawBar(70.f, H - 62.f, 320.f, 8.f, Player->GetStaminaRatio(), FLinearColor(0.75f, 0.7f, 0.4f));

		// Ammo
		const FString AmmoText = Player->IsReloading()
			? FString(TEXT("RELOADING"))
			: FString::Printf(TEXT("%d / %d"), Player->GetAmmo(), Player->GetReserve());
		DrawText(AmmoText, FLinearColor(1.f, 0.85f, 0.5f), W - 220.f, H - 90.f, Medium, 1.4f);

		// Kill counter (drives the Spark -> Skeleton -> Flesh evolution next).
		if (const AFNGameMode* GM = GetWorld()->GetAuthGameMode<AFNGameMode>())
		{
			DrawText(FString::Printf(TEXT("KILLS %d"), GM->GetKills()), FLinearColor(0.8f, 0.8f, 0.8f), W - 220.f, H - 130.f, Small);
		}
	}

	if (Boss)
	{
		const float BW = W * 0.5f;
		DrawCentered(TEXT("PERUN - MENTOR"), 40.f, FLinearColor(0.9f, 0.85f, 0.75f), 0.8f);
		DrawBar((W - BW) * 0.5f, 78.f, BW, 12.f, Boss->GetHealth()->GetRatio(), FLinearColor(0.55f, 0.5f, 0.85f));
	}

	// Exam outcomes (GDD §9): both are full results.
	if (Player && Player->IsDead())
	{
		const bool bNear = Boss && Boss->GetHealth()->GetRatio() <= NearWinRatio;
		DrawCentered(TEXT("The mentor stops the fight."), H * 0.38f, FLinearColor(0.95f, 0.9f, 0.8f), 1.2f);
		DrawCentered(bNear ? TEXT("\"You almost had me. Come back stronger.\"") : TEXT("\"Not yet. Learn the storm first.\""),
			H * 0.45f, FLinearColor(0.8f, 0.8f, 0.8f), 0.9f);
		DrawCentered(TEXT("Press Enter to try again"), H * 0.55f, FLinearColor(0.7f, 0.7f, 0.7f), 0.7f);
	}
	else if (Boss && Boss->IsDead())
	{
		DrawCentered(TEXT("You surpassed your mentor."), H * 0.38f, FLinearColor(1.f, 0.85f, 0.5f), 1.2f);
		DrawCentered(TEXT("Press Enter to restart"), H * 0.55f, FLinearColor(0.7f, 0.7f, 0.7f), 0.7f);
	}
}
