#include "FNHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "EngineUtils.h"
#include "FNCharacter.h"
#include "FNGameMode.h"
#include "FNHealthComponent.h"
#include "FNMob.h"
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
	const float X = (Canvas->ClipX - TW) * 0.5f;
	DrawText(Text, FLinearColor(0.f, 0.f, 0.f, 0.85f * Color.A), X + 1.5f, Y + 1.5f, Font, Scale); // drop shadow for readability
	DrawText(Text, Color, X, Y, Font, Scale);
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
	// Remnant-2-like look drawn from scratch (no third-party art): thin dark frames, red health with notches,
	// round relic slot, long bottom boss bar with a trailing damage chip, weapon silhouette + big ammo, minimap.
	const FLinearColor FrameDark(0.02f, 0.02f, 0.02f, 0.78f);
	const FLinearColor FrameLine(0.55f, 0.52f, 0.46f, 0.9f);
	const FLinearColor TextMain(0.93f, 0.91f, 0.86f);
	const FLinearColor TextDim(0.66f, 0.64f, 0.6f);
	const FLinearColor HealthRed(0.66f, 0.07f, 0.06f);
	const FLinearColor HealthChip(0.55f, 0.36f, 0.3f); // trailing damage: muted, so it never reads as health
	const FLinearColor StaminaTone(0.86f, 0.84f, 0.76f);
	const FLinearColor Gold(1.f, 0.78f, 0.35f);
	const FLinearColor BossRed(0.62f, 0.05f, 0.05f);

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

	// Path of the chapter for the minimap (metres, X north).
	const FVector2D MapPath[] = { {-30, -10}, {0, 0}, {45, 12}, {140, 38}, {180, 40}, {280, -20}, {420, -20}, {445, 0}, {525, 10}, {560, 20}, {640, 10}, {698, -8}, {720, 0}, {742, 0}, {850, 0}, {875, 12} };
}

void AFNHUD::DrawPanel(float X, float Y, float PW, float PH)
{
	DrawRect(FrameDark, X, Y, PW, PH);
	DrawLine(X, Y, X + PW, Y, FrameLine, 1.f);
	DrawLine(X, Y + PH, X + PW, Y + PH, FrameLine * FLinearColor(1, 1, 1, 0.5f), 1.f);
}

void AFNHUD::DrawRing(float CX, float CY, float R, const FLinearColor& C, float Thickness)
{
	constexpr int32 Seg = 40;
	for (int32 i = 0; i < Seg; ++i)
	{
		const float A0 = 2.f * PI * i / Seg, A1 = 2.f * PI * (i + 1) / Seg;
		DrawLine(CX + R * FMath::Cos(A0), CY + R * FMath::Sin(A0), CX + R * FMath::Cos(A1), CY + R * FMath::Sin(A1), C, Thickness);
	}
}

void AFNHUD::FillDisc(float CX, float CY, float R, const FLinearColor& C)
{
	for (float Dy = -R; Dy <= R; Dy += 1.f)
	{
		const float Half = FMath::Sqrt(FMath::Max(0.f, R * R - Dy * Dy));
		DrawRect(C, CX - Half, CY + Dy, 2.f * Half, 1.f);
	}
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
	const float Dt = GetWorld()->GetDeltaSeconds();

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

	if (GetWorld()->GetTimeSeconds() < 15.0)
	{
		DrawCentered(TEXT("WASD — ход   ЛКМ — удар   ПКМ+ЛКМ — выстрел   Пробел — уклонение   R — перезарядка   Колесо/Q — оружие   1–4 — способности   Tab — дерево"),
			H * 0.9f, TextDim, 0.8f);
	}

	if (Player)
	{
		// ---- Crosshair: small ring + dot; ring tightens when aiming, warms on hit.
		{
			const bool bHit = Player->GetTimeSinceHit() < 0.12f;
			const FLinearColor C = bHit ? (Player->WasLastHitWeak() ? Gold : FLinearColor(1.f, 0.6f, 0.4f)) : FLinearColor(1.f, 1.f, 1.f, 0.8f);
			const float R = Player->IsAiming() ? 9.f : 14.f;
			DrawRing(W * 0.5f, H * 0.5f, R, C, 1.2f);
			DrawRect(C, W * 0.5f - 1.f, H * 0.5f - 1.f, 2.f, 2.f);
			if (bHit && Player->WasLastHitWeak())
			{
				DrawRing(W * 0.5f, H * 0.5f, R + 6.f, Gold, 1.f);
			}
		}

		// ---- Bottom-centre: ability slots 1-4 on the left, then stamina over health (notched, trailing chip).
		{
			const float SlotSize = 46.f, Gap = 6.f, BarW = 340.f;
			const float BlockW = 4.f * (SlotSize + Gap) + 14.f + BarW;
			const float X0 = (W - BlockW) * 0.5f, Y0 = H - 78.f;

			for (int32 i = 0; i < 4; ++i)
			{
				const float SX = X0 + i * (SlotSize + Gap);
				const bool bOpen = Player->IsAbilitySlotOpen(i);
				DrawRect(FrameDark, SX, Y0, SlotSize, SlotSize);
				DrawLine(SX, Y0, SX + SlotSize, Y0, bOpen ? FrameLine : FLinearColor(0.3f, 0.3f, 0.3f), 1.f);
				DrawLine(SX, Y0 + SlotSize, SX + SlotSize, Y0 + SlotSize, FrameLine * FLinearColor(1, 1, 1, 0.4f), 1.f);
				if (!bOpen)
				{
					// Slot 4: the ultimate, sealed until Nav.
					DrawLine(SX + 12.f, Y0 + 12.f, SX + SlotSize - 12.f, Y0 + SlotSize - 12.f, FLinearColor(0.35f, 0.35f, 0.35f), 1.5f);
					DrawLine(SX + SlotSize - 12.f, Y0 + 12.f, SX + 12.f, Y0 + SlotSize - 12.f, FLinearColor(0.35f, 0.35f, 0.35f), 1.5f);
				}
				DrawText(FString::FromInt(i + 1), bOpen ? TextMain : TextDim, SX + 4.f, Y0 + 2.f, Small, 0.8f);
			}

			const float BX = X0 + 4.f * (SlotSize + Gap) + 14.f;
			const float Max = Player->GetHealth()->MaxHealth;

			// Stamina above health.
			DrawRect(FrameDark, BX - 2.f, Y0 - 2.f, BarW + 4.f, 10.f);
			DrawRect(StaminaTone, BX, Y0, BarW * Player->GetStaminaRatio(), 6.f);

			const float HY = Y0 + 12.f;
			const float Ratio = Player->GetHealth()->GetRatio();
			ShownHealth = ShownHealth < 0.f ? Ratio : FMath::FInterpTo(ShownHealth, Ratio, Dt, Ratio < ShownHealth ? 2.5f : 20.f);
			DrawRect(FrameDark, BX - 3.f, HY - 3.f, BarW + 6.f, 22.f);
			DrawRect(HealthChip, BX, HY, BarW * FMath::Max(ShownHealth, Ratio), 16.f);
			DrawRect(HealthRed, BX, HY, BarW * Ratio, 16.f);
			for (float V = 25.f; V < Max; V += 25.f)
			{
				DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.5f), BX + BarW * (V / Max), HY, 1.5f, 16.f);
			}
			DrawText(FString::Printf(TEXT("%.0f / %.0f"), Player->GetHealth()->Health, Max), TextMain, BX + 6.f, HY - 1.f, Small, 0.8f);

			static const TCHAR* StageNames[] = { TEXT("Искра"), TEXT("Скелет"), TEXT("Плоть") };
			DrawText(StageNames[static_cast<int32>(Player->GetStage())], TextDim, BX, HY + 22.f, Small, 0.8f);
		}

		// ---- Bottom-left: round "relic" slot = the Spark's emblem, count = rune-keys found.
		{
			const float CX = 78.f, CY = H - 86.f, R = 34.f;
			const int32 StageIdx = static_cast<int32>(Player->GetStage());
			const FLinearColor Ember = Player->GetSparkColor();
			const FLinearColor Core = StageIdx == 0 ? Ember : (StageIdx == 1 ? Ember * 0.4f + FLinearColor(0.35f, 0.33f, 0.3f) : FLinearColor(0.5f, 0.12f, 0.1f));
			FillDisc(CX, CY, R + 4.f, FrameDark);
			FillDisc(CX, CY, R - 6.f, Core * 0.55f);
			FillDisc(CX, CY, R - 14.f, Core);
			DrawRing(CX, CY, R, FrameLine, 2.f);
			DrawText(FString::FromInt(Player->GetTree()->NumRunesFound()), TextMain, CX + R - 2.f, CY - R - 6.f, Medium, 1.1f);
			// Small consumable-like pips: skill points waiting.
			for (int32 i = 0; i < FMath::Min(Player->GetSkillPoints(), 6); ++i)
			{
				DrawRect(Gold, CX + R + 14.f + i * 12.f, CY + 18.f, 8.f, 8.f);
			}
		}

		// ---- Bottom-right: weapon silhouette, big magazine / small reserve, round mod slot, secondary above.
		{
			const float X = W - 360.f, Y = H - 104.f;
			static const TCHAR* WeaponNames[] = { TEXT("Плазма"), TEXT("Ружьё"), TEXT("Дробовик") };
			const EFNWeapon Cur = Player->GetWeapon();

			// Silhouette drawn from rectangles (placeholder art).
			const FLinearColor Sil(0.8f, 0.78f, 0.72f, 0.9f);
			if (Cur == EFNWeapon::Plasma)
			{
				FillDisc(X + 60.f, Y + 44.f, 16.f, Player->GetSparkColor());
				DrawRing(X + 60.f, Y + 44.f, 22.f, Sil, 1.f);
			}
			else
			{
				const float Len = Cur == EFNWeapon::Rifle ? 150.f : 110.f;
				DrawRect(Sil, X, Y + 38.f, Len, 8.f);                       // barrel + receiver
				DrawRect(Sil, X + Len - 50.f, Y + 38.f, 50.f, 14.f);        // body
				DrawRect(Sil, X + Len - 12.f, Y + 44.f, 26.f, 22.f);        // stock
				DrawRect(Sil, X + Len - 70.f, Y + 50.f, 10.f, 18.f);        // grip
			}
			DrawText(WeaponNames[static_cast<int32>(Cur)], TextDim, X, Y + 76.f, Small, 0.85f);

			const float AX = X + 190.f;
			if (Player->IsReloading())
			{
				DrawText(TEXT("—"), Gold, AX, Y + 18.f, Large, 1.8f);
			}
			else if (Cur == EFNWeapon::Plasma)
			{
				DrawText(TEXT("∞"), TextMain, AX, Y + 14.f, Large, 2.f);
			}
			else
			{
				const int32 Mag = Cur == EFNWeapon::Scatter ? Player->GetScatterAmmo() : Player->GetAmmo();
				DrawText(FString::Printf(TEXT("%02d"), Mag), Mag == 0 ? FLinearColor(0.9f, 0.25f, 0.2f) : TextMain, AX, Y + 14.f, Large, 2.f);
				DrawText(FString::Printf(TEXT("%02d"), Player->GetReserve()), TextDim, AX + 4.f, Y + 64.f, Medium, 0.85f);
			}

			// Round slot on the far right (the reference's mod icon): weapon number.
			const float MX = W - 58.f, MY = Y + 44.f;
			FillDisc(MX, MY, 22.f, FrameDark);
			DrawRing(MX, MY, 22.f, FrameLine, 1.5f);
			DrawText(TEXT("Q"), TextMain, MX - 6.f, MY - 10.f, Medium, 0.9f);

			// Other owned weapons, small, above the block.
			float SY = Y - 22.f;
			for (int32 i = 0; i < 3; ++i)
			{
				const EFNWeapon Wp = static_cast<EFNWeapon>(i);
				if (Wp == Cur || !Player->HasWeapon(Wp)) { continue; }
				DrawText(WeaponNames[i], TextDim, W - 190.f, SY, Small, 0.8f);
				SY -= 18.f;
			}
		}

		// ---- Top-right: minimap (path of the chapter, player dot), zone name, skill points notice.
		{
			const float MW = 190.f, MH = 150.f, X = W - MW - 30.f, Y = 26.f;
			DrawRect(FrameDark, X, Y, MW, MH);
			DrawLine(X, Y, X + MW, Y, FrameLine, 1.f);
			const FVector P = Player->GetActorLocation() / 100.f;
			auto ToMap = [&](const FVector2D& Mt) // north up, centred on the player, 1 px = 3 m
			{
				return FVector2D(X + MW * 0.5f + (Mt.Y - P.Y) / 3.f, Y + MH * 0.5f - (Mt.X - P.X) / 3.f);
			};
			for (int32 i = 0; i + 1 < UE_ARRAY_COUNT(MapPath); ++i)
			{
				FVector2D A = ToMap(MapPath[i]), B = ToMap(MapPath[i + 1]);
				const bool bIn = [&](const FVector2D& V) { return V.X > X && V.X < X + MW && V.Y > Y && V.Y < Y + MH; }(A)
					&& [&](const FVector2D& V) { return V.X > X && V.X < X + MW && V.Y > Y && V.Y < Y + MH; }(B);
				if (bIn) { DrawLine(A.X, A.Y, B.X, B.Y, FLinearColor(0.7f, 0.6f, 0.42f, 0.9f), 3.f); }
			}
			const FVector2D Arena = ToMap(FVector2D(720.f, 0.f));
			if (Arena.X > X && Arena.X < X + MW && Arena.Y > Y && Arena.Y < Y + MH) { DrawRing(Arena.X, Arena.Y, 7.f, FLinearColor(0.8f, 0.2f, 0.2f), 1.5f); }
			const FVector2D Treba = ToMap(FVector2D(645.f, 10.f));
			if (Treba.X > X && Treba.X < X + MW && Treba.Y > Y && Treba.Y < Y + MH) { DrawRect(FLinearColor(0.75f, 0.82f, 1.f), Treba.X - 3.f, Treba.Y - 3.f, 6.f, 6.f); }
			DrawRect(Gold, X + MW * 0.5f - 3.f, Y + MH * 0.5f - 3.f, 6.f, 6.f);

			DrawText(ZoneName(P.X), TextMain, X, Y + MH + 6.f, Small, 0.95f);
			if (Player->GetSkillPoints() > 0)
			{
				DrawText(FString::Printf(TEXT("Доступны очки навыков: %d  ⚠"), Player->GetSkillPoints()), TextMain, X, Y + MH + 26.f, Small, 0.85f);
			}
		}

		// ---- Event messages, upper centre.
		if (Player->GetMessageAge() < 3.5f && !Player->GetMessage().IsEmpty())
		{
			const float A = FMath::Clamp(3.5f - Player->GetMessageAge(), 0.f, 1.f);
			DrawCentered(Player->GetMessage(), H * 0.2f, FLinearColor(1.f, 0.92f, 0.78f, A), 0.9f);
		}
	}

	// ---- Mob health bars above their heads (only near or wounded).
	if (Player)
	{
		for (TActorIterator<AFNMob> It(GetWorld()); It; ++It)
		{
			AFNMob* Mob = *It;
			const UFNHealthComponent* MH = Mob->FindComponentByClass<UFNHealthComponent>();
			if (!MH || Mob->IsDead() || MH->IsDead()) { continue; }
			const float Dist = FVector::Dist(Mob->GetActorLocation(), Player->GetActorLocation());
			if (Dist > 3500.f || (MH->GetRatio() >= 1.f && Dist > 1800.f)) { continue; }
			const FVector S = Project(Mob->GetActorLocation() + FVector(0.f, 0.f, 150.f));
			if (S.Z <= 0.f) { continue; } // behind the camera
			const float BW = 56.f;
			DrawRect(FrameDark, S.X - BW * 0.5f - 1.f, S.Y - 1.f, BW + 2.f, 7.f);
			DrawRect(HealthRed, S.X - BW * 0.5f, S.Y, BW * MH->GetRatio(), 5.f);
		}
	}

	// ---- Boss / elite: big bar at the top centre with the name (god or elite).
	const bool bBossBar = Boss && Boss->IsFightActive();
	if (bBossBar)
	{
		const float BW = W * 0.5f, X = (W - BW) * 0.5f, Y = 64.f;
		const float Ratio = Boss->GetHealth()->GetRatio();
		ShownBoss = ShownBoss < 0.f ? Ratio : FMath::FInterpTo(ShownBoss, Ratio, Dt, Ratio < ShownBoss ? 1.5f : 20.f);
		DrawCentered(TEXT("ПЕРУН"), 18.f, TextMain, 1.1f);
		DrawCentered(FString::Printf(TEXT("наставник  ·  фаза %d"), Boss->GetPhase()), 44.f, TextDim, 0.75f);
		DrawRect(FrameDark, X - 3.f, Y - 3.f, BW + 6.f, 18.f);
		DrawRect(HealthChip, X, Y, BW * FMath::Max(ShownBoss, Ratio), 12.f);
		DrawRect(BossRed, X, Y, BW * Ratio, 12.f);
		for (int32 i = 1; i < 3; ++i) // phase marks at 66% / 33%
		{
			DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.7f), X + BW * (i / 3.f), Y, 2.f, 12.f);
		}
	}
	else
	{
		ShownBoss = -1.f;
	}

	if (Player && Player->IsDead() && !Player->IsExamDefeat())
	{
		DrawCentered(TEXT("Искра гаснет…"), H * 0.4f, FLinearColor(0.7f, 0.9f, 1.f), 1.1f);
	}

	// Subtitles sit above the boss bar, like the reference.
	if (Boss && Boss->HasSubtitle())
	{
		DrawCentered(FString::Printf(TEXT("%s:  %s"), *Boss->GetSubtitleSpeaker(), *Boss->GetSubtitle()), H - 128.f, FLinearColor(1.f, 0.97f, 0.9f), 0.85f);
	}

	if (Boss && Boss->ShowEndCard())
	{
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.72f), 0.f, H * 0.35f, W, H * 0.25f);
		DrawCentered(TEXT("ПРОДОЛЖЕНИЕ СЛЕДУЕТ"), H * 0.4f, FLinearColor(0.85f, 0.9f, 1.f), 1.6f);
		DrawCentered(TEXT("Enter — начать заново"), H * 0.5f, TextDim, 0.7f);
	}
}
