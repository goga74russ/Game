#include "FNHUD.h"

#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/FontFace.h"
#include "EngineUtils.h"
#include "FNCharacter.h"
#include "FNGameMode.h"
#include "FNHealthComponent.h"
#include "FNMob.h"
#include "FNPerunBoss.h"
#include "FNSkillTree.h"
#include "RenderUtils.h"
#include "TextureResource.h"

namespace
{
	// Palette of docs/ui/hud_mockup_v1.html (hex values reinterpreted as-is, Canvas draws in display space).
	FLinearColor Hex(const TCHAR* H, float A = 1.f) { FLinearColor C = FColor::FromHex(H).ReinterpretAsLinear(); C.A = A; return C; }
	const FLinearColor Bone = Hex(TEXT("e8dfc8"));
	const FLinearColor BoneDim = Hex(TEXT("a89f8a"));
	const FLinearColor Ink = Hex(TEXT("0d0b09"), 0.8f);
	const FLinearColor Ink2 = Hex(TEXT("15110d"), 0.93f);
	const FLinearColor BloodTop = Hex(TEXT("c24a36"));
	const FLinearColor BloodBot = Hex(TEXT("6d1a12"));
	const FLinearColor BloodHi = Hex(TEXT("d0503a"));
	const FLinearColor Chip = Hex(TEXT("e8c9a0"), 0.6f);
	const FLinearColor Stam = Hex(TEXT("cfc6a8"));
	const FLinearColor StamDark = Hex(TEXT("8d8468"));
	const FLinearColor Gold = Hex(TEXT("b8955a"));
	const FLinearColor Line = Hex(TEXT("b8955a"), 0.55f);
	const FLinearColor Shadow(0.f, 0.f, 0.f, 0.75f);

	const TCHAR* FacePaths[] = {
		TEXT("/Game/UI/Fonts/FF_RuslanDisplay_Regular.FF_RuslanDisplay_Regular"),
		TEXT("/Game/UI/Fonts/FF_CormorantGaramond_Bold.FF_CormorantGaramond_Bold"),
		TEXT("/Game/UI/Fonts/FF_CormorantGaramond_SemiBold.FF_CormorantGaramond_SemiBold"),
		TEXT("/Game/UI/Fonts/FF_CormorantGaramond_MediumItalic.FF_CormorantGaramond_MediumItalic"),
	};

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

	const TCHAR* MobName(EFNMobType T)
	{
		switch (T)
		{
		case EFNMobType::Otrost: return TEXT("Отросток");
		case EFNMobType::Strelnik: return TEXT("Стрельник");
		default: return TEXT("Рыхлец");
		}
	}

	// Path of the chapter for the minimap (metres, X north).
	const FVector2D MapPath[] = { {-30, -10}, {0, 0}, {45, 12}, {140, 38}, {180, 40}, {280, -20}, {420, -20}, {445, 0}, {525, 10}, {560, 20}, {640, 10}, {698, -8}, {720, 0}, {742, 0}, {850, 0}, {875, 12} };
}

// ---------------------------------------------------------------- drawing helpers

UFont* AFNHUD::Font(int32 Face, float Px720)
{
	if (Faces.Num() == 0)
	{
		for (const TCHAR* P : FacePaths) { Faces.Add(LoadObject<UFontFace>(nullptr, P)); }
	}
	// Slate font size is in points at 96 dpi: px = pt * 4/3.
	const int32 Pt = FMath::Max(6, FMath::RoundToInt(Px720 * S * 0.75f));
	const int32 Key = Face * 1000 + Pt;
	if (TObjectPtr<UFont>* Found = FontCache.Find(Key)) { return *Found; }
	UFont* F = GEngine->GetMediumFont();
	if (Faces.IsValidIndex(Face) && Faces[Face])
	{
		F = NewObject<UFont>(this);
		F->FontCacheType = EFontCacheType::Runtime;
		F->LegacyFontSize = Pt;
		FTypefaceEntry& E = F->CompositeFont.DefaultTypeface.Fonts.AddDefaulted_GetRef();
		E.Name = TEXT("Regular");
		E.Font = FFontData(Faces[Face]);
	}
	FontCache.Add(Key, F);
	return F;
}

void AFNHUD::Txt(const FString& Text, float X, float Y, UFont* InFont, const FLinearColor& Color, EAlign Align, float Scale)
{
	float TW = 0.f, TH = 0.f;
	GetTextSize(Text, TW, TH, InFont, Scale);
	float PX = X * S;
	if (Align == EAlign::Center) { PX -= TW * 0.5f; }
	else if (Align == EAlign::Right) { PX -= TW; }
	const FLinearColor Sh(0.f, 0.f, 0.f, 0.8f * Color.A);
	DrawText(Text, Sh, PX + 1.f * S, Y * S + 1.5f * S, InFont, Scale);
	DrawText(Text, Color, PX, Y * S, InFont, Scale);
}

void AFNHUD::DrawCentered(const FString& Text, float Y, const FLinearColor& Color, float Scale)
{
	Txt(Text, Canvas->ClipX * 0.5f / S, Y / S, Font(2, 18.f), Color, EAlign::Center, Scale);
}

void AFNHUD::FillTri(const FVector2D& A, const FVector2D& B, const FVector2D& C, const FLinearColor& Col)
{
	FCanvasTriangleItem Tri(A * S, B * S, C * S, GWhiteTexture);
	Tri.SetColor(Col);
	Tri.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Tri);
}

void AFNHUD::FillBevel(float X, float Y, float W, float H, float Cut, const FLinearColor& Col)
{
	// Octagon-ish panel: top-left and bottom-right corners cut (the mockup's slot shape).
	DrawRect(Col, (X + Cut) * S, Y * S, (W - Cut) * S, (H - Cut) * S);
	DrawRect(Col, X * S, (Y + Cut) * S, Cut * S, (H - Cut) * S);
	DrawRect(Col, (X + Cut) * S, (Y + H - Cut) * S, (W - 2.f * Cut) * S, Cut * S);
	FillTri({ X, Y + Cut }, { X + Cut, Y }, { X + Cut, Y + Cut }, Col);
	FillTri({ X + W - Cut, Y + H - Cut }, { X + W, Y + H - Cut }, { X + W - Cut, Y + H }, Col);
}

void AFNHUD::LineBevel(float X, float Y, float W, float H, float Cut, const FLinearColor& Col, float Thick)
{
	const FVector2D P[] = { { X + Cut, Y }, { X + W, Y }, { X + W, Y + H - Cut }, { X + W - Cut, Y + H }, { X, Y + H }, { X, Y + Cut } };
	for (int32 i = 0; i < 6; ++i)
	{
		const FVector2D& A = P[i];
		const FVector2D& B = P[(i + 1) % 6];
		DrawLine(A.X * S, A.Y * S, B.X * S, B.Y * S, Col, Thick * S);
	}
}

void AFNHUD::FillDisc(float CX, float CY, float R, const FLinearColor& C)
{
	constexpr int32 Seg = 32;
	for (int32 i = 0; i < Seg; ++i)
	{
		const float A0 = 2.f * PI * i / Seg, A1 = 2.f * PI * (i + 1) / Seg;
		FillTri({ CX, CY }, { CX + R * FMath::Cos(A0), CY + R * FMath::Sin(A0) }, { CX + R * FMath::Cos(A1), CY + R * FMath::Sin(A1) }, C);
	}
}

void AFNHUD::DrawRing(float CX, float CY, float R, const FLinearColor& C, float Thickness)
{
	constexpr int32 Seg = 48;
	for (int32 i = 0; i < Seg; ++i)
	{
		const float A0 = 2.f * PI * i / Seg, A1 = 2.f * PI * (i + 1) / Seg;
		DrawLine((CX + R * FMath::Cos(A0)) * S, (CY + R * FMath::Sin(A0)) * S, (CX + R * FMath::Cos(A1)) * S, (CY + R * FMath::Sin(A1)) * S, C, Thickness * S);
	}
}

void AFNHUD::GradRect(float X, float Y, float W, float H, const FLinearColor& Top, const FLinearColor& Bottom)
{
	if (W <= 0.f) { return; }
	constexpr int32 Steps = 6;
	for (int32 i = 0; i < Steps; ++i)
	{
		const float T = (i + 0.5f) / Steps;
		DrawRect(FMath::Lerp(Top, Bottom, T), X * S, (Y + H * i / Steps) * S, W * S, H / Steps * S + 0.5f);
	}
}

void AFNHUD::Ornament(float CX, float Y, float HalfW)
{
	// Gold hairline fading to both sides, with a small lightning mark in the middle.
	constexpr int32 N = 12;
	for (int32 i = 0; i < N; ++i)
	{
		const float A = 1.f - static_cast<float>(i) / N;
		FLinearColor C = Gold; C.A = 0.8f * A;
		const float X0 = HalfW * i / N, X1 = HalfW * (i + 1) / N;
		DrawLine((CX + 14.f + X0) * S, Y * S, (CX + 14.f + X1) * S, Y * S, C, 1.f);
		DrawLine((CX - 14.f - X0) * S, Y * S, (CX - 14.f - X1) * S, Y * S, C, 1.f);
	}
	const FVector2D Bolt[] = { { CX + 2.f, Y - 8.f }, { CX - 4.f, Y + 1.f }, { CX + 1.f, Y + 1.f }, { CX - 2.f, Y + 8.f }, { CX + 5.f, Y - 2.f }, { CX, Y - 2.f }, { CX + 2.f, Y - 8.f } };
	for (int32 i = 0; i + 1 < UE_ARRAY_COUNT(Bolt); ++i)
	{
		DrawLine(Bolt[i].X * S, Bolt[i].Y * S, Bolt[i + 1].X * S, Bolt[i + 1].Y * S, Gold, 1.2f * S);
	}
}

// ---------------------------------------------------------------- skill tree

void AFNHUD::DrawTree(AFNCharacter* Player)
{
	const float W = Canvas->ClipX;
	const float H = Canvas->ClipY;
	UFont* Small = Font(2, 15.f);
	UFNSkillTree* Tree = Player->GetTree();
	const TArray<FFNNode>& Nodes = UFNSkillTree::Nodes();
	const int32 Points = Player->GetSkillPoints();

	DrawRect(FLinearColor(0.02f, 0.02f, 0.03f, 0.9f), 0.f, 0.f, W, H);
	Txt(TEXT("ДРЕВО"), W * 0.5f / S, 18.f, Font(0, 28.f), Bone, EAlign::Center);
	Txt(FString::Printf(TEXT("очки: %d   ·   руны-ключи: %d / %d   ·   Tab — закрыть"), Points, Tree->NumRunesFound(), UFNSkillTree::RuneOrder().Num()),
		W * 0.5f / S, 56.f, Font(3, 16.f), BoneDim, EAlign::Center);
	Ornament(W * 0.5f / S, 84.f, 220.f);

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
		if (Tree->IsLockedByRune(i)) { DrawText(TEXT("?"), FLinearColor(0.6f, 0.6f, 0.7f), P.X - 4.f, P.Y - 10.f, Small); }
		AddHitBox(FVector2D(P.X - R, P.Y - R), FVector2D(2.f * R, 2.f * R), FName(*FString::FromInt(i)), true);
	}

	// Tooltip
	if (Nodes.IsValidIndex(HoveredNode))
	{
		const FFNNode& N = Nodes[HoveredNode];
		const bool bLocked = Tree->IsLockedByRune(HoveredNode);
		const FString Title = bLocked ? FString(TEXT("??? (нужна руна-ключ)")) : FString(N.Name);
		const FString Body = bLocked ? FString(TEXT("Руну-ключ роняют твари Высей. Найди её — и узел откроется.")) : FString(N.Desc);
		const float TX = W * 0.3f / S, TY = H / S - 118.f;
		FillBevel(TX, TY, W * 0.4f / S, 88.f, 10.f, Ink2);
		LineBevel(TX, TY, W * 0.4f / S, 88.f, 10.f, Line, 1.f);
		Txt(Title, TX + 16.f, TY + 10.f, Font(1, 20.f), Hex(TEXT("e8c9a0")));
		Txt(Body, TX + 16.f, TY + 42.f, Small, Bone);
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

// ---------------------------------------------------------------- HUD

void AFNHUD::DrawHUD()
{
	Super::DrawHUD();
	static const bool bNoHUD = FParse::Param(FCommandLine::Get(), TEXT("NoHUD")); // test key: clean frames for UI mockups
	if (bNoHUD || !Canvas)
	{
		return;
	}

	// Everything below is laid out in 720p units; S maps them to the real viewport.
	const float NewS = Canvas->ClipY / 720.f;
	if (!FMath::IsNearlyEqual(NewS, S, 0.01f)) { S = NewS; FontCache.Reset(); }
	const float W = Canvas->ClipX / S; // virtual width
	const float H = 720.f;
	const float Dt = GetWorld()->GetDeltaSeconds();
	const float Time = GetWorld()->GetRealTimeSeconds();

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
		Txt(TEXT("WASD — ход   ·   ЛКМ — удар   ·   ПКМ+ЛКМ — выстрел   ·   Пробел — уклонение   ·   R — перезарядка   ·   Колесо/Q — оружие   ·   1–4 — способности   ·   Tab — древо"),
			W * 0.5f, 604.f, Font(3, 15.f), BoneDim, EAlign::Center);
	}

	if (Player)
	{
		// ---- Crosshair: thin ring with four ticks; tightens when aiming, warms on hit, gold ring on a weak point.
		{
			const bool bHit = Player->GetTimeSinceHit() < 0.12f;
			const FLinearColor C = bHit ? (Player->WasLastHitWeak() ? Hex(TEXT("ffb35c")) : BloodHi) : Hex(TEXT("e8dfc8"), 0.8f);
			const float CX = W * 0.5f, CY = H * 0.5f;
			const float R = Player->IsAiming() ? 8.f : 11.f;
			DrawRing(CX, CY, R, C, 1.2f);
			FillDisc(CX, CY, 1.6f, C);
			for (int32 i = 0; i < 4; ++i)
			{
				const FVector2D D = FVector2D(FMath::Cos(i * PI * 0.5f), FMath::Sin(i * PI * 0.5f));
				const FVector2D A = FVector2D(CX, CY) + D * (R + 4.f), B = FVector2D(CX, CY) + D * (R + 9.f);
				DrawLine(A.X * S, A.Y * S, B.X * S, B.Y * S, C, 1.2f * S);
			}
			if (bHit && Player->WasLastHitWeak()) { DrawRing(CX, CY, R + 13.f, Hex(TEXT("ffb35c")), 1.f); }
		}

		// ---- Bottom-centre: ability slots 1-4, then stage line, ember health, stamina.
		{
			const float Slot = 50.f, Gap = 6.f, BarW = 360.f;
			const float BlockW = 4.f * Slot + 3.f * Gap + 16.f + BarW;
			const float X0 = (W - BlockW) * 0.5f, SY = H - 26.f - Slot;

			for (int32 i = 0; i < 4; ++i)
			{
				const float SX = X0 + i * (Slot + Gap);
				const bool bOpen = Player->IsAbilitySlotOpen(i);
				FLinearColor Fill = Ink2; if (!bOpen) { Fill.A *= 0.55f; }
				FillBevel(SX, SY, Slot, Slot, 8.f, Fill);
				LineBevel(SX, SY, Slot, Slot, 8.f, bOpen ? Line : Hex(TEXT("b8955a"), 0.25f), 1.f);
				Txt(FString::FromInt(i + 1), SX + 5.f, SY + 1.f, Font(1, 13.f), BoneDim);
				if (!bOpen)
				{
					// Slot 4: the ultimate, sealed until Nav — a small padlock.
					const float LX = SX + Slot * 0.5f, LY = SY + Slot * 0.5f + 3.f;
					const FLinearColor LC = BoneDim;
					DrawLine((LX - 6.f) * S, (LY - 2.f) * S, (LX + 6.f) * S, (LY - 2.f) * S, LC, 1.f * S);
					DrawLine((LX - 6.f) * S, (LY + 7.f) * S, (LX + 6.f) * S, (LY + 7.f) * S, LC, 1.f * S);
					DrawLine((LX - 6.f) * S, (LY - 2.f) * S, (LX - 6.f) * S, (LY + 7.f) * S, LC, 1.f * S);
					DrawLine((LX + 6.f) * S, (LY - 2.f) * S, (LX + 6.f) * S, (LY + 7.f) * S, LC, 1.f * S);
					for (int32 k = 0; k < 8; ++k)
					{
						const float A0 = PI + PI * k / 8.f, A1 = PI + PI * (k + 1) / 8.f;
						DrawLine((LX + 4.f * FMath::Cos(A0)) * S, (LY - 2.f + 5.f * FMath::Sin(A0)) * S, (LX + 4.f * FMath::Cos(A1)) * S, (LY - 2.f + 5.f * FMath::Sin(A1)) * S, LC, 1.f * S);
					}
				}
			}

			const float BX = X0 + 4.f * Slot + 3.f * Gap + 16.f;
			const float StY = H - 26.f - 6.f;     // stamina, bottom line
			const float HY = StY - 4.f - 18.f;   // health above it
			const float LY = HY - 24.f;          // stage line above health

			// Stage + evolution progress.
			static const TCHAR* StageNames[] = { TEXT("Искра"), TEXT("Скелет"), TEXT("Плоть") };
			const int32 StageIdx = static_cast<int32>(Player->GetStage());
			FillDisc(BX + 4.f, LY + 11.f, 3.5f, Player->GetSparkColor());
			FillDisc(BX + 4.f, LY + 11.f, 7.f, Player->GetSparkColor() * FLinearColor(1.f, 1.f, 1.f, 0.25f));
			Txt(StageNames[StageIdx], BX + 14.f, LY, Font(2, 17.f), BoneDim);
			const int32 Kills = Player->GetKills();
			const FString Prog = StageIdx == 0 ? FString::Printf(TEXT("убито %d / %d"), Kills, Player->KillsToSkeleton)
				: (StageIdx == 1 ? FString::Printf(TEXT("убито %d / %d"), Kills, Player->KillsToFlesh) : FString::Printf(TEXT("убито %d"), Kills));
			Txt(Prog, BX + BarW, LY, Font(2, 16.f), BoneDim, EAlign::Right);

			// Health: dark well, trailing chip, gradient blood, ember pulse (stronger when low), notches every 25.
			const float Max = Player->GetHealth()->MaxHealth;
			const float Ratio = Player->GetHealth()->GetRatio();
			ShownHealth = ShownHealth < 0.f ? Ratio : FMath::FInterpTo(ShownHealth, Ratio, Dt, Ratio < ShownHealth ? 2.5f : 20.f);
			DrawRect(Ink, (BX - 1.f) * S, (HY - 1.f) * S, (BarW + 2.f) * S, 20.f * S);
			const float Pulse = 0.5f + 0.5f * FMath::Sin(Time * (Ratio < 0.3f ? 6.f : 2.6f));
			FLinearColor Glow = BloodHi; Glow.A = (Ratio < 0.3f ? 0.35f : 0.18f) * Pulse;
			DrawRect(Glow, (BX - 3.f) * S, (HY - 3.f) * S, (BarW * Ratio + 6.f) * S, 24.f * S);
			DrawRect(Chip, BX * S, HY * S, BarW * FMath::Max(ShownHealth, Ratio) * S, 18.f * S);
			GradRect(BX, HY, BarW * Ratio, 18.f, BloodTop, BloodBot);
			DrawRect(Hex(TEXT("ffb080"), 0.18f * Pulse), BX * S, HY * S, BarW * Ratio * S, 2.f * S); // hot top edge
			for (float V = 25.f; V < Max; V += 25.f)
			{
				DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.5f), (BX + BarW * (V / Max)) * S, HY * S, 1.f * S, 18.f * S);
			}
			DrawLine(BX * S, (HY - 1.f) * S, (BX + BarW) * S, (HY - 1.f) * S, Line, 1.f);
			Txt(FString::Printf(TEXT("%.0f / %.0f"), Player->GetHealth()->Health, Max), BX + 8.f, HY - 1.f, Font(1, 16.f), Bone);

			// Stamina: thin bone line.
			DrawRect(Ink, BX * S, StY * S, BarW * S, 6.f * S);
			const float SR = Player->GetStaminaRatio();
			for (int32 k = 0; k < 8; ++k) // horizontal gradient dark -> bone
			{
				const float A = k / 8.f, B = (k + 1) / 8.f;
				if (A >= SR) { break; }
				DrawRect(FMath::Lerp(StamDark, Stam, A), (BX + BarW * A) * S, StY * S, BarW * (FMath::Min(B, SR) - A) * S + 0.5f, 6.f * S);
			}
		}

		// ---- Bottom-left: round emblem of the Spark (colour by stage), count = rune-keys found.
		{
			const float CX = 66.f, CY = H - 30.f - 36.f, R = 36.f;
			const int32 StageIdx = static_cast<int32>(Player->GetStage());
			const FLinearColor Ember = Player->GetSparkColor();
			const FLinearColor Core = StageIdx == 0 ? Ember : (StageIdx == 1 ? Ember * 0.4f + FLinearColor(0.35f, 0.33f, 0.3f) : Hex(TEXT("d88b3a")));
			FillDisc(CX, CY, R, Hex(TEXT("1a140e"), 0.92f));
			FLinearColor Halo = Core; Halo.A = 0.18f + 0.08f * FMath::Sin(Time * 2.f);
			FillDisc(CX, CY, 20.f, Halo);
			FillDisc(CX, CY, 11.f, Core);
			FillDisc(CX, CY - 2.f, 5.f, Hex(TEXT("fff2d0"), 0.8f));
			DrawRing(CX, CY, R, Line, 1.f);
			Txt(FString::FromInt(Player->GetTree()->NumRunesFound()), CX + R + 10.f, CY - 22.f, Font(1, 24.f), Bone);
			Txt(TEXT("руны"), CX + R + 10.f, CY + 4.f, Font(2, 15.f), BoneDim);
		}

		// ---- Bottom-right: owned weapons list, big ammo, round weapon icon, keys.
		{
			static const TCHAR* WeaponNames[] = { TEXT("Плазма"), TEXT("Ружьё"), TEXT("Дробовик") };
			const EFNWeapon Cur = Player->GetWeapon();
			const float RX = W - 30.f;
			const float IcoR = 32.f, ICX = RX - IcoR, ICY = H - 28.f - 22.f - IcoR;

			float LY = ICY - IcoR - 16.f - 20.f * 3.f;
			for (int32 i = 0; i < 3; ++i)
			{
				const EFNWeapon Wp = static_cast<EFNWeapon>(i);
				FLinearColor C = Wp == Cur ? Bone : BoneDim;
				if (!Player->HasWeapon(Wp)) { C.A = 0.35f; }
				Txt(WeaponNames[i], RX, LY, Font(Wp == Cur ? 1 : 2, 16.f), C, EAlign::Right);
				LY += 20.f;
			}

			FillDisc(ICX, ICY, IcoR, Hex(TEXT("0f1314"), 0.92f));
			DrawRing(ICX, ICY, IcoR, Line, 1.f);
			const FLinearColor Sil = Bone;
			if (Cur == EFNWeapon::Plasma)
			{
				FLinearColor E = Player->GetSparkColor(); E.A = 0.85f;
				FillDisc(ICX, ICY, 9.f, E);
				E.A = 0.4f;
				DrawRing(ICX, ICY, 16.f, E, 1.f);
			}
			else
			{
				// Line-art gun: barrel, body, stock, grip.
				const float L = Cur == EFNWeapon::Rifle ? 22.f : 15.f;
				DrawLine((ICX - L) * S, (ICY - 3.f) * S, (ICX + 8.f) * S, (ICY - 3.f) * S, Sil, 2.f * S);
				DrawRect(Sil, (ICX - 4.f) * S, (ICY - 5.f) * S, 14.f * S, 6.f * S);
				FillTri({ ICX + 10.f, ICY - 5.f }, { ICX + 20.f, ICY + 1.f }, { ICX + 10.f, ICY + 1.f }, Sil);
				DrawLine((ICX + 2.f) * S, (ICY + 1.f) * S, (ICX - 1.f) * S, (ICY + 9.f) * S, Sil, 2.f * S);
			}

			const float AX = ICX - IcoR - 14.f;
			if (Player->IsReloading())
			{
				Txt(TEXT("—"), AX, ICY - 30.f, Font(1, 44.f), Gold, EAlign::Right);
			}
			else if (Cur == EFNWeapon::Plasma)
			{
				Txt(TEXT("∞"), AX, ICY - 30.f, Font(1, 44.f), Bone, EAlign::Right);
			}
			else
			{
				const int32 Mag = Cur == EFNWeapon::Scatter ? Player->GetScatterAmmo() : Player->GetAmmo();
				const FString Res = FString::Printf(TEXT("/ %d"), Player->GetReserve());
				float RW = 0.f, RH = 0.f;
				GetTextSize(Res, RW, RH, Font(2, 20.f));
				Txt(Res, AX, ICY - 6.f, Font(2, 20.f), BoneDim, EAlign::Right);
				Txt(FString::Printf(TEXT("%02d"), Mag), AX - RW / S - 6.f, ICY - 30.f, Font(1, 44.f), Mag == 0 ? BloodHi : Bone, EAlign::Right);
			}

			// Key hints under the icon.
			float KX = RX;
			for (const TCHAR* K : { TEXT("колесо"), TEXT("Q") })
			{
				float KW = 0.f, KH = 0.f;
				GetTextSize(K, KW, KH, Font(2, 13.f));
				const float BW = KW / S + 12.f;
				KX -= BW;
				const float KY = H - 28.f - 16.f;
				LineBevel(KX, KY, BW, 17.f, 0.f, Line, 1.f);
				Txt(K, KX + 6.f, KY, Font(2, 13.f), BoneDim);
				KX -= 5.f;
			}
		}

		// ---- Top-right: round minimap (north up, centred on the player), zone name, skill points.
		{
			const float R = 75.f, CX = W - 26.f - R, CY = 22.f + R;
			FillDisc(CX, CY, R, Hex(TEXT("17120c"), 0.9f));
			FillDisc(CX, CY, R * 0.6f, Hex(TEXT("231c14"), 0.35f));
			DrawRing(CX, CY, R, Line, 1.f);
			const FVector P = Player->GetActorLocation() / 100.f;
			auto ToMap = [&](const FVector2D& Mt) { return FVector2D(CX + (Mt.Y - P.Y) / 3.f, CY - (Mt.X - P.X) / 3.f); }; // 1 px = 3 m
			auto Inside = [&](const FVector2D& V) { return FVector2D::Distance(V, FVector2D(CX, CY)) < R - 4.f; };
			for (int32 i = 0; i + 1 < UE_ARRAY_COUNT(MapPath); ++i)
			{
				const FVector2D A = ToMap(MapPath[i]), B = ToMap(MapPath[i + 1]);
				const float Len = FVector2D::Distance(A, B);
				// Dashed trail: 4 px dash, 3 px gap, only inside the disc.
				for (float D = 0.f; D < Len; D += 7.f)
				{
					const FVector2D U = FMath::Lerp(A, B, D / Len), V = FMath::Lerp(A, B, FMath::Min(D + 4.f, Len) / Len);
					if (Inside(U) && Inside(V)) { DrawLine(U.X * S, U.Y * S, V.X * S, V.Y * S, Gold, 2.f * S); }
				}
			}
			const FVector2D Arena = ToMap(FVector2D(720.f, 0.f));
			if (Inside(Arena)) { DrawRing(Arena.X, Arena.Y, 6.f, BloodHi, 1.5f); }
			const FVector2D Treba = ToMap(FVector2D(645.f, 10.f));
			if (Inside(Treba)) { FillDisc(Treba.X, Treba.Y, 3.f, Hex(TEXT("d88b3a"))); }
			FLinearColor Me = Player->GetSparkColor();
			FillDisc(CX, CY, 4.f, Me);
			Me.A = 0.35f;
			DrawRing(CX, CY, 9.f, Me, 1.f);
			FillTri({ CX, CY - R + 4.f }, { CX - 4.f, CY - R + 12.f }, { CX + 4.f, CY - R + 12.f }, Bone); // north

			Txt(ZoneName(P.X), W - 26.f, CY + R + 6.f, Font(1, 19.f), Bone, EAlign::Right);
			if (Player->GetSkillPoints() > 0)
			{
				Txt(FString::Printf(TEXT("✦ %d очк. древа  ·  Tab"), Player->GetSkillPoints()), W - 26.f, CY + R + 30.f, Font(2, 15.f), Gold, EAlign::Right);
			}
		}

		// ---- Event messages, upper centre, with a faint ember glow.
		if (Player->GetMessageAge() < 3.5f && !Player->GetMessage().IsEmpty())
		{
			const float A = FMath::Clamp(3.5f - Player->GetMessageAge(), 0.f, 1.f);
			FLinearColor G = Player->GetSparkColor(); G.A = 0.25f * A;
			Txt(Player->GetMessage(), W * 0.5f + 0.5f, 170.5f, Font(1, 24.f), G, EAlign::Center, 1.02f);
			Txt(Player->GetMessage(), W * 0.5f, 170.f, Font(1, 24.f), Hex(TEXT("f1e3c0"), A), EAlign::Center);
		}
	}

	// ---- Mob name + health above their heads (only near or wounded).
	if (Player)
	{
		for (TActorIterator<AFNMob> It(GetWorld()); It; ++It)
		{
			AFNMob* Mob = *It;
			const UFNHealthComponent* MH = Mob->FindComponentByClass<UFNHealthComponent>();
			if (!MH || Mob->IsDead() || MH->IsDead()) { continue; }
			const float Dist = FVector::Dist(Mob->GetActorLocation(), Player->GetActorLocation());
			if (Dist > 3500.f || (MH->GetRatio() >= 1.f && Dist > 1800.f)) { continue; }
			const FVector SP = Project(Mob->GetActorLocation() + FVector(0.f, 0.f, 150.f));
			if (SP.Z <= 0.f) { continue; } // behind the camera
			const float MX = SP.X / S, MY = SP.Y / S, BW = 70.f;
			Txt(MobName(Mob->GetMobType()), MX, MY - 18.f, Font(2, 13.f), BoneDim, EAlign::Center);
			DrawRect(Ink, (MX - BW * 0.5f - 1.f) * S, (MY - 1.f) * S, (BW + 2.f) * S, 8.f * S);
			DrawRect(BloodHi, (MX - BW * 0.5f) * S, MY * S, BW * MH->GetRatio() * S, 6.f * S);
		}
	}

	// ---- Boss / elite: name in Ruslan Display, subtitle, long bar with gold phase marks, ornament.
	const bool bBossBar = Boss && Boss->IsFightActive();
	if (bBossBar)
	{
		const float BW = 640.f, X = (W - BW) * 0.5f, Y = 84.f;
		const float Ratio = Boss->GetHealth()->GetRatio();
		ShownBoss = ShownBoss < 0.f ? Ratio : FMath::FInterpTo(ShownBoss, Ratio, Dt, Ratio < ShownBoss ? 1.5f : 20.f);
		Txt(TEXT("ПЕРУН"), W * 0.5f, 18.f, Font(0, 30.f), Bone, EAlign::Center);
		Txt(FString::Printf(TEXT("наставник  ·  фаза %d"), Boss->GetPhase()), W * 0.5f, 56.f, Font(3, 15.f), BoneDim, EAlign::Center);
		DrawRect(Ink, (X - 1.f) * S, (Y - 1.f) * S, (BW + 2.f) * S, 14.f * S);
		DrawRect(Chip, X * S, Y * S, BW * FMath::Max(ShownBoss, Ratio) * S, 12.f * S);
		FLinearColor Glow = BloodHi; Glow.A = 0.22f;
		DrawRect(Glow, (X - 2.f) * S, (Y - 2.f) * S, (BW * Ratio + 4.f) * S, 16.f * S);
		GradRect(X, Y, BW * Ratio, 12.f, BloodTop, Hex(TEXT("7c1f16")));
		DrawLine((X - 1.f) * S, (Y - 1.f) * S, (X + BW + 1.f) * S, (Y - 1.f) * S, Line, 1.f);
		for (int32 i = 1; i < 3; ++i) // phase marks at 66% / 33%
		{
			DrawRect(Gold, (X + BW * (i / 3.f) - 1.f) * S, (Y - 3.f) * S, 2.f * S, 18.f * S);
		}
		Ornament(W * 0.5f, Y + 26.f, 220.f);
	}
	else
	{
		ShownBoss = -1.f;
	}

	if (Player && Player->IsDead() && !Player->IsExamDefeat())
	{
		Txt(TEXT("Искра гаснет…"), W * 0.5f, H * 0.4f, Font(3, 34.f), Hex(TEXT("bfe6f5")), EAlign::Center);
	}

	// Subtitles: gold speaker in small caps, italic line.
	if (Boss && Boss->HasSubtitle())
	{
		const FString Who = Boss->GetSubtitleSpeaker().ToUpper();
		const FString Line1 = Boss->GetSubtitle();
		float WW = 0.f, WH = 0.f, LW = 0.f, LH = 0.f;
		GetTextSize(Who, WW, WH, Font(1, 17.f));
		GetTextSize(Line1, LW, LH, Font(3, 24.f));
		const float Total = (WW + LW) / S + 10.f;
		const float SX = (W - Total) * 0.5f, SY = H - 128.f - 28.f;
		Txt(Who, SX, SY + 6.f, Font(1, 17.f), Gold);
		Txt(Line1, SX + WW / S + 10.f, SY, Font(3, 24.f), Bone);
	}

	if (Boss && Boss->ShowEndCard())
	{
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.72f), 0.f, H * 0.35f * S, W * S, H * 0.25f * S);
		Txt(TEXT("ПРОДОЛЖЕНИЕ СЛЕДУЕТ"), W * 0.5f, H * 0.39f, Font(0, 40.f), Bone, EAlign::Center);
		Ornament(W * 0.5f, H * 0.39f + 60.f, 240.f);
		Txt(TEXT("Enter — начать заново"), W * 0.5f, H * 0.39f + 74.f, Font(3, 17.f), BoneDim, EAlign::Center);
	}
}
