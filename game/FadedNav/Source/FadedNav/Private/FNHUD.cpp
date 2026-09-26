#include "FNHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "EngineUtils.h"
#include "FNCharacter.h"
#include "FNGameMode.h"
#include "FNHealthComponent.h"
#include "FNPerunBoss.h"
#include "FNSkillTree.h"

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

void AFNHUD::DrawTree(AFNCharacter* Player)
{
	const float W = Canvas->ClipX;
	const float H = Canvas->ClipY;
	UFont* Small = GEngine->GetSmallFont();
	UFNSkillTree* Tree = Player->GetTree();
	const TArray<FFNNode>& Nodes = UFNSkillTree::Nodes();
	const int32 Points = Player->GetSkillPoints();

	DrawRect(FLinearColor(0.02f, 0.02f, 0.04f, 0.88f), 0.f, 0.f, W, H);
	DrawCentered(FString::Printf(TEXT("ДЕРЕВО НАВЫКОВ   очки: %d   руны-ключи: %d / %d   (Tab — закрыть)"),
		Points, Tree->NumRunesFound(), UFNSkillTree::RuneOrder().Num()), 30.f, FLinearColor(1.f, 0.9f, 0.7f), 0.8f);

	auto Pos = [W, H](const FFNNode& N) { return FVector2D(N.X * W, N.Y * H); };

	// Links
	for (int32 i = 0; i < Nodes.Num(); ++i)
	{
		for (int32 L : Nodes[i].Links)
		{
			if (L < i) { continue; }
			const bool bLit = Tree->IsAllocated(i) && Tree->IsAllocated(L);
			const FVector2D A = Pos(Nodes[i]), B = Pos(Nodes[L]);
			DrawLine(A.X, A.Y, B.X, B.Y, bLit ? FLinearColor(1.f, 0.75f, 0.3f) : FLinearColor(0.3f, 0.3f, 0.35f), bLit ? 3.f : 1.5f);
		}
	}

	// Nodes: gold = learned, white = available, dim = needs a point / path, dark with "?" = rune-key not found.
	for (int32 i = 0; i < Nodes.Num(); ++i)
	{
		const FFNNode& N = Nodes[i];
		const float R = N.Kind == EFNNodeKind::Keystone ? 22.f : (N.Kind == EFNNodeKind::Notable ? 17.f : (N.Kind == EFNNodeKind::Root ? 18.f : 11.f));
		const FVector2D P = Pos(N);
		FLinearColor C(0.35f, 0.35f, 0.4f);
		if (Tree->IsAllocated(i)) { C = FLinearColor(1.f, 0.72f, 0.25f); }
		else if (Tree->IsLockedByRune(i)) { C = FLinearColor(0.12f, 0.12f, 0.15f); }
		else if (Tree->CanAllocate(i, Points)) { C = FLinearColor(0.95f, 0.95f, 1.f); }
		if (i == HoveredNode) { DrawRect(FLinearColor::White, P.X - R - 3.f, P.Y - R - 3.f, 2.f * R + 6.f, 2.f * R + 6.f); }
		DrawRect(C, P.X - R, P.Y - R, 2.f * R, 2.f * R);
		if (Tree->IsLockedByRune(i)) { DrawText(TEXT("?"), FLinearColor(0.6f, 0.6f, 0.7f), P.X - 4.f, P.Y - 8.f, Small); }
		AddHitBox(FVector2D(P.X - R, P.Y - R), FVector2D(2.f * R, 2.f * R), FName(*FString::FromInt(i)), true);
	}

	// Tooltip
	if (Nodes.IsValidIndex(HoveredNode))
	{
		const FFNNode& N = Nodes[HoveredNode];
		const bool bLocked = Tree->IsLockedByRune(HoveredNode);
		const FString Title = bLocked ? FString(TEXT("??? (нужна руна-ключ)")) : FString(N.Name);
		const FString Body = bLocked ? FString(TEXT("Руну-ключ роняют твари Высей. Найди её — и узел откроется.")) : FString(N.Desc);
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.8f), W * 0.3f, H - 110.f, W * 0.4f, 80.f);
		DrawText(Title, FLinearColor(1.f, 0.85f, 0.5f), W * 0.31f, H - 100.f, Small, 1.2f);
		DrawText(Body, FLinearColor(0.9f, 0.9f, 0.9f), W * 0.31f, H - 70.f, Small);
	}
}

void AFNHUD::NotifyHitBoxClick(FName BoxName)
{
	if (AFNCharacter* Player = Cast<AFNCharacter>(GetOwningPawn()))
	{
		Player->TryAllocate(FCString::Atoi(*BoxName.ToString()));
	}
}

void AFNHUD::NotifyHitBoxBeginCursorOver(FName BoxName)
{
	HoveredNode = FCString::Atoi(*BoxName.ToString());
}

void AFNHUD::NotifyHitBoxEndCursorOver(FName BoxName)
{
	if (HoveredNode == FCString::Atoi(*BoxName.ToString()))
	{
		HoveredNode = -1;
	}
}

namespace
{
	// HUD palette (style_v0.1): dark translucent panels with a thin wax-gold edge.
	const FLinearColor PanelBg(0.02f, 0.02f, 0.03f, 0.62f);
	const FLinearColor Edge(0.78f, 0.6f, 0.3f, 0.85f);
	const FLinearColor TextMain(0.95f, 0.92f, 0.85f);
	const FLinearColor TextDim(0.7f, 0.68f, 0.62f);
	const FLinearColor HealthRed(0.72f, 0.16f, 0.12f);
	const FLinearColor StaminaTone(0.82f, 0.74f, 0.45f);
	const FLinearColor Gold(1.f, 0.78f, 0.35f);

	const TCHAR* ZoneName(float XMetres)
	{
		if (XMetres < 60.f) return TEXT("Сухоречье");
		if (XMetres < 135.f) return TEXT("Околица");
		if (XMetres < 230.f) return TEXT("Присяжный камень");
		if (XMetres < 440.f) return TEXT("Стрелокопни");
		if (XMetres < 640.f) return TEXT("Ведёрный ряд");
		if (XMetres < 750.f) return TEXT("Кумирная горка");
		return TEXT("Громовой гребень");
	}
}

void AFNHUD::DrawPanel(float X, float Y, float PW, float PH)
{
	DrawRect(PanelBg, X, Y, PW, PH);
	DrawRect(Edge, X, Y, PW, 1.f);
	DrawRect(Edge * FLinearColor(1.f, 1.f, 1.f, 0.4f), X, Y + PH - 1.f, PW, 1.f);
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
	UFont* Large = GEngine->GetLargeFont();

	AFNCharacter* Player = Cast<AFNCharacter>(GetOwningPawn());
	if (Player && Player->IsTreeOpen())
	{
		DrawTree(Player);
		return;
	}
	AFNPerunBoss* Boss = nullptr;
	for (TActorIterator<AFNPerunBoss> It(GetWorld()); It; ++It)
	{
		Boss = *It;
		break;
	}

	// Controls: only for the first seconds of play.
	if (GetWorld()->GetTimeSeconds() < 15.0)
	{
		DrawPanel(20.f, 20.f, 560.f, 30.f);
		DrawText(TEXT("WASD — ход   Мышь — обзор   ЛКМ — огонь   ПКМ — прицел   Пробел — уклонение   F — удар   R — перезарядка   1/2/3 — оружие   Tab — дерево"),
			TextDim, 30.f, 27.f, Small, 0.85f);
	}

	if (Player)
	{
		// ---- Crosshair: small centre dot + ticks, gold flash on hit, wider on a weak point.
		{
			const bool bHitFlash = Player->GetTimeSinceHit() < 0.12f;
			const float S = bHitFlash && Player->WasLastHitWeak() ? 10.f : 6.f;
			const FLinearColor CH = bHitFlash ? Gold : FLinearColor(1.f, 1.f, 1.f, 0.85f);
			const float CX = W * 0.5f, CY = H * 0.5f;
			DrawRect(CH, CX - 1.f, CY - 1.f, 2.f, 2.f);
			DrawRect(CH, CX - S - 7.f, CY - 0.5f, S, 1.5f);
			DrawRect(CH, CX + 7.f, CY - 0.5f, S, 1.5f);
			DrawRect(CH, CX - 0.5f, CY + 7.f, 1.5f, S);
		}

		// ---- Bottom-left: stage emblem + health + stamina (Remnant 2 layout, our content).
		{
			const float X = 32.f, Y = H - 118.f;
			static const TCHAR* StageNames[] = { TEXT("ИСКРА"), TEXT("СКЕЛЕТ"), TEXT("ПЛОТЬ") };
			const int32 StageIdx = static_cast<int32>(Player->GetStage());
			const FLinearColor Ember = Player->GetSparkColor();
			const FLinearColor EmblemColor = StageIdx == 0 ? Ember : (StageIdx == 1 ? Ember * 0.45f + FLinearColor(0.4f, 0.38f, 0.33f) : FLinearColor(0.55f, 0.35f, 0.25f));

			// Emblem: framed square with the stage colour (the "relic" slot of the reference).
			DrawRect(Edge, X - 2.f, Y - 2.f, 68.f, 68.f);
			DrawRect(FLinearColor(0.03f, 0.03f, 0.04f, 0.9f), X, Y, 64.f, 64.f);
			DrawRect(EmblemColor, X + 14.f, Y + 14.f, 36.f, 36.f);
			DrawText(StageNames[StageIdx], TextMain, X, Y + 70.f, Small, 0.8f);

			const float BX = X + 80.f;
			DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), BX - 1.f, Y + 17.f, 302.f, 16.f);
			DrawRect(HealthRed, BX, Y + 18.f, 300.f * Player->GetHealth()->GetRatio(), 14.f);
			DrawText(FString::Printf(TEXT("%.0f / %.0f"), Player->GetHealth()->Health, Player->GetHealth()->MaxHealth), TextMain, BX + 6.f, Y + 17.f, Small, 0.8f);
			DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), BX - 1.f, Y + 38.f, 242.f, 7.f);
			DrawRect(StaminaTone, BX, Y + 39.f, 240.f * Player->GetStaminaRatio(), 5.f);
		}

		// ---- Bottom-right: current weapon, big magazine / reserve, other weapons small.
		{
			const float PW = 300.f, PH = 96.f, X = W - PW - 32.f, Y = H - PH - 30.f;
			DrawPanel(X, Y, PW, PH);
			static const TCHAR* WeaponNames[] = { TEXT("Плазма"), TEXT("Ружьё"), TEXT("Дробовик") };
			const EFNWeapon Cur = Player->GetWeapon();
			DrawText(WeaponNames[static_cast<int32>(Cur)], TextMain, X + 14.f, Y + 8.f, Medium, 0.9f);

			if (Player->IsReloading())
			{
				DrawText(TEXT("перезарядка…"), Gold, X + 14.f, Y + 44.f, Medium, 1.f);
			}
			else if (Cur == EFNWeapon::Plasma)
			{
				DrawText(TEXT("∞"), Gold, X + 14.f, Y + 34.f, Large, 1.6f);
			}
			else
			{
				const int32 Mag = Cur == EFNWeapon::Scatter ? Player->GetScatterAmmo() : Player->GetAmmo();
				DrawText(FString::Printf(TEXT("%02d"), Mag), Mag == 0 ? HealthRed : Gold, X + 14.f, Y + 30.f, Large, 1.8f);
				DrawText(FString::Printf(TEXT("%d"), Player->GetReserve()), TextDim, X + 110.f, Y + 56.f, Medium, 0.9f);
			}

			// Weapon slots on the right edge of the panel.
			for (int32 i = 0; i < 3; ++i)
			{
				const EFNWeapon Wp = static_cast<EFNWeapon>(i);
				const bool bOwned = Player->HasWeapon(Wp);
				const FLinearColor C = Wp == Cur ? Gold : (bOwned ? TextDim : FLinearColor(0.3f, 0.3f, 0.3f));
				DrawText(FString::Printf(TEXT("%d  %s"), i + 1, bOwned ? WeaponNames[i] : TEXT("—")), C, X + 180.f, Y + 12.f + i * 24.f, Small, 0.85f);
			}
		}

		// ---- Top-right: zone name, skill points notice, rune-keys.
		{
			const float PW = 300.f, X = W - PW - 32.f, Y = 24.f;
			const bool bPoints = Player->GetSkillPoints() > 0;
			DrawPanel(X, Y, PW, bPoints ? 84.f : 58.f);
			DrawText(ZoneName(Player->GetActorLocation().X / 100.f), TextMain, X + 14.f, Y + 8.f, Medium, 0.9f);
			DrawText(FString::Printf(TEXT("Руны-ключи: %d / 7      Убито: %d"), Player->GetTree()->NumRunesFound(), Player->GetKills()), TextDim, X + 14.f, Y + 34.f, Small, 0.85f);
			if (bPoints)
			{
				DrawText(FString::Printf(TEXT("Доступны очки навыков: %d   ▲ Tab"), Player->GetSkillPoints()), Gold, X + 14.f, Y + 56.f, Small, 0.9f);
			}
		}

		// ---- Event messages (evolution, pickups, treba), upper centre.
		if (Player->GetMessageAge() < 3.5f && !Player->GetMessage().IsEmpty())
		{
			const float A = FMath::Clamp(3.5f - Player->GetMessageAge(), 0.f, 1.f);
			DrawCentered(Player->GetMessage(), H * 0.2f, FLinearColor(1.f, 0.92f, 0.75f, A), 0.9f);
		}
	}

	// ---- Bottom-centre: boss bar (name + phase), subtitles sit above it.
	const bool bBossBar = Boss && Boss->IsFightActive();
	if (bBossBar)
	{
		const float BW = W * 0.42f, X = (W - BW) * 0.5f, Y = H - 58.f;
		DrawCentered(FString::Printf(TEXT("Перун — наставник   ·   фаза %d"), Boss->GetPhase()), Y - 26.f, TextMain, 0.8f);
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.65f), X - 2.f, Y - 2.f, BW + 4.f, 12.f);
		DrawRect(FLinearColor(0.62f, 0.55f, 0.9f), X, Y, BW * Boss->GetHealth()->GetRatio(), 8.f);
	}

	// Spark returning to the treba (deaths outside the arena).
	if (Player && Player->IsDead() && !Player->IsExamDefeat())
	{
		DrawCentered(TEXT("Искра гаснет…"), H * 0.4f, FLinearColor(0.7f, 0.9f, 1.f), 1.1f);
	}

	// Subtitles (exam, outcomes, epilogue).
	if (Boss && Boss->HasSubtitle())
	{
		const FString Line = FString::Printf(TEXT("%s:  %s"), *Boss->GetSubtitleSpeaker(), *Boss->GetSubtitle());
		DrawCentered(Line, H - (bBossBar ? 118.f : 84.f), FLinearColor(1.f, 0.96f, 0.88f), 0.85f);
	}

	// End of the demo chapter.
	if (Boss && Boss->ShowEndCard())
	{
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.72f), 0.f, H * 0.35f, W, H * 0.25f);
		DrawCentered(TEXT("ПРОДОЛЖЕНИЕ СЛЕДУЕТ"), H * 0.4f, FLinearColor(0.85f, 0.9f, 1.f), 1.6f);
		DrawCentered(TEXT("Enter — начать заново"), H * 0.5f, TextDim, 0.7f);
	}
}
