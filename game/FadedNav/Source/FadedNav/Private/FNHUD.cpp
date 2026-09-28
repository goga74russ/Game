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
#include "FNRite.h"
#include "FNSkills.h"
#include "FNSkillTree.h"
#include "RenderUtils.h"
#include "TextureResource.h"

namespace
{
	// Palette of docs/ui/hud_mockup_v1.html: sRGB hex -> linear (Canvas colours are linear).
	FLinearColor Hex(const TCHAR* H, float A = 1.f) { FLinearColor C(FColor::FromHex(H)); C.A = A; return C; }
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
		case EFNMobType::Otrost: return TEXT("Отрост");
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

// ---------------------------------------------------------------- skill tree (docs/ui/tree_mockup_v1.html)

namespace
{
	// Line-art icons in a 24x24 box; polylines separated by breaks (X < 0).
	void AddCircle(TArray<FVector2D>& Out, float CX, float CY, float R, float A0 = 0.f, float A1 = 2.f * PI, int32 Seg = 16)
	{
		for (int32 i = 0; i <= Seg; ++i) { const float A = FMath::Lerp(A0, A1, float(i) / Seg); Out.Add(FVector2D(CX + R * FMath::Cos(A), CY + R * FMath::Sin(A))); }
		Out.Add(FVector2D(-1.f, -1.f));
	}
	void AddPoly(TArray<FVector2D>& Out, std::initializer_list<FVector2D> Pts)
	{
		for (const FVector2D& P : Pts) { Out.Add(P); }
		Out.Add(FVector2D(-1.f, -1.f));
	}
	void IconLines(int32 Id, TArray<FVector2D>& O)
	{
		using V = FVector2D;
		switch (Id)
		{
		case 1: AddPoly(O, { V(13, 2), V(5, 14), V(11, 14), V(9, 22), V(19, 9), V(13, 9), V(13, 2) }); break; // bolt
		case 2: AddCircle(O, 12, 19, 10.6f, -2.35f, -0.79f, 10); AddCircle(O, 12, 5, 10.6f, 0.79f, 2.35f, 10); AddCircle(O, 12, 12, 3.f); break; // eye
		case 3: AddPoly(O, { V(12, 3), V(20, 6), V(20, 12), V(17, 17), V(12, 21), V(7, 17), V(4, 12), V(4, 6), V(12, 3) }); break; // shield
		case 4: AddCircle(O, 9, 8, 4.f); AddCircle(O, 15, 17, 3.2f); break; // foot
		case 5: AddCircle(O, 12, 15, 5.5f, -0.5f, PI + 0.5f, 12); AddPoly(O, { V(7.2f, 12.3f), V(12, 3), V(16.8f, 12.3f) }); break; // drop
		case 6: AddCircle(O, 12, 12, 8.f); AddCircle(O, 12, 12, 4.f); break; // ring
		case 7: AddPoly(O, { V(7, 7), V(17, 17) }); AddCircle(O, 6, 8, 2.2f); AddCircle(O, 8, 6, 2.2f); AddCircle(O, 16, 18, 2.2f); AddCircle(O, 18, 16, 2.2f); break; // bone
		case 8: AddPoly(O, { V(12, 2), V(12, 22) }); AddPoly(O, { V(8, 7), V(16, 7) }); AddPoly(O, { V(7, 11), V(17, 11) }); AddPoly(O, { V(8, 15), V(16, 15) }); break; // spindle
		case 9: AddPoly(O, { V(12, 3), V(16, 9), V(17, 14), V(15, 19), V(12, 21), V(9, 19), V(7, 14), V(9, 10), V(11, 12), V(12, 3) }); break; // flame
		case 10: AddCircle(O, 12, 10, 7.f); AddCircle(O, 9, 11, 1.5f); AddCircle(O, 15, 11, 1.5f); AddPoly(O, { V(8, 17), V(8, 20), V(16, 20), V(16, 17) }); break; // skull
		case 11:
			AddCircle(O, 12, 12, 5.f);
			for (int32 k = 0; k < 8; ++k) { const float A = k * PI / 4.f; AddPoly(O, { V(12 + 7 * FMath::Cos(A), 12 + 7 * FMath::Sin(A)), V(12 + 10 * FMath::Cos(A), 12 + 10 * FMath::Sin(A)) }); }
			break; // sun
		case 12: AddPoly(O, { V(3, 8), V(15, 8) }); AddPoly(O, { V(3, 12), V(11, 12) }); AddPoly(O, { V(3, 16), V(17, 16) }); AddCircle(O, 15, 5, 3.f, PI * 0.5f, PI * 2.2f, 8); break; // wind
		default: break;
		}
	}

	float NodeRadius(EFNNodeKind Kind)
	{
		return Kind == EFNNodeKind::Root ? 34.f : (Kind == EFNNodeKind::Keystone ? 30.f : (Kind == EFNNodeKind::Notable ? 20.f : 9.f));
	}
}

void AFNHUD::TreeInput(AFNCharacter* Player)
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC) { return; }
	float MX = 0.f, MY = 0.f;
	PC->GetMousePosition(MX, MY);
	const FVector2D Mouse(MX, MY);
	const FVector2D Center(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.5f);
	const float K = TreeZoom * S;

	// Hover: nearest node under the cursor.
	const TArray<FFNNode>& Nodes = UFNSkillTree::Nodes();
	HoveredNode = -1;
	float Best = 1e9f;
	for (int32 i = 0; i < Nodes.Num(); ++i)
	{
		const float D = FVector2D::Distance(Center + (Nodes[i].Pos + TreePan) * K, Mouse);
		if (D < FMath::Max(NodeRadius(Nodes[i].Kind) * K + 4.f, 10.f) && D < Best) { Best = D; HoveredNode = i; }
	}

	// LMB: drag pans; a click without dragging learns the whole path to the hovered node.
	const bool bDown = PC->IsInputKeyDown(EKeys::LeftMouseButton);
	if (bDown && !bTreeLmbDown) { TreeDragStart = Mouse; TreePanStart = TreePan; bTreeDragging = false; }
	if (bDown && FVector2D::Distance(Mouse, TreeDragStart) > 5.f) { bTreeDragging = true; }
	if (bDown && bTreeDragging) { TreePan = TreePanStart + (Mouse - TreeDragStart) / K; }
	if (!bDown && bTreeLmbDown && !bTreeDragging && HoveredNode >= 0) { Player->TryAllocate(HoveredNode); }
	bTreeLmbDown = bDown;

	if (PC->WasInputKeyJustPressed(EKeys::RightMouseButton) && HoveredNode >= 0) { Player->TryRefund(HoveredNode); }

	// Wheel: zoom around the cursor.
	const int32 Wheel = (PC->WasInputKeyJustPressed(EKeys::MouseScrollUp) ? 1 : 0) - (PC->WasInputKeyJustPressed(EKeys::MouseScrollDown) ? 1 : 0);
	if (Wheel != 0)
	{
		const float Z0 = TreeZoom, Z1 = FMath::Clamp(Z0 * (Wheel > 0 ? 1.15f : 1.f / 1.15f), 0.22f, 1.8f);
		const FVector2D M = (Mouse - Center) / S;
		TreePan += M / Z1 - M / Z0;
		TreeZoom = Z1;
	}
}

void AFNHUD::DrawTree(AFNCharacter* Player)
{
	TreeInput(Player);

	const float PW = Canvas->ClipX, PH = Canvas->ClipY;
	const float W = PW / S, H = 720.f;
	const float K = TreeZoom * S;
	const FVector2D Center(PW * 0.5f, PH * 0.5f);
	const float Time = GetWorld()->GetRealTimeSeconds();
	UFNSkillTree* Tree = Player->GetTree();
	const TArray<FFNNode>& Nodes = UFNSkillTree::Nodes();
	const int32 Points = Player->GetSkillPoints();
	const FLinearColor Lit = UFNSkillTree::GodColor(0);

	auto ToS = [&](const FVector2D& P) { return Center + (P + TreePan) * K; };
	auto Tri = [&](const FVector2D& A, const FVector2D& B, const FVector2D& C, const FLinearColor& Col)
	{
		FCanvasTriangleItem T(A, B, C, GWhiteTexture); T.SetColor(Col); T.BlendMode = SE_BLEND_Translucent; Canvas->DrawItem(T);
	};
	auto Disc = [&](const FVector2D& C, float R, const FLinearColor& Col)
	{
		const int32 Seg = FMath::Clamp(FMath::RoundToInt(R * 0.8f), 8, 40);
		for (int32 i = 0; i < Seg; ++i)
		{
			const float A0 = 2.f * PI * i / Seg, A1 = 2.f * PI * (i + 1) / Seg;
			Tri(C, C + FVector2D(FMath::Cos(A0), FMath::Sin(A0)) * R, C + FVector2D(FMath::Cos(A1), FMath::Sin(A1)) * R, Col);
		}
	};
	auto Ring = [&](const FVector2D& C, float R, const FLinearColor& Col, float Th, bool bDash = false)
	{
		const int32 Seg = FMath::Clamp(FMath::RoundToInt(R * 0.8f), 12, 160);
		for (int32 i = 0; i < Seg; ++i)
		{
			if (bDash && (i % 2)) { continue; }
			const float A0 = 2.f * PI * i / Seg, A1 = 2.f * PI * (i + 1) / Seg;
			const FVector2D P0 = C + FVector2D(FMath::Cos(A0), FMath::Sin(A0)) * R, P1 = C + FVector2D(FMath::Cos(A1), FMath::Sin(A1)) * R;
			DrawLine(P0.X, P0.Y, P1.X, P1.Y, Col, Th);
		}
	};
	auto Curve = [&](int32 A, int32 B, TArray<FVector2D>& Out)
	{
		// Branch-like quadratic curve, bent alternately left/right.
		const FVector2D PA = Nodes[A].Pos, PB = Nodes[B].Pos, D = PB - PA;
		const float L = FMath::Max(1.f, D.Size());
		const float O = L * 0.12f * (((A + B) % 2) ? 1.f : -1.f);
		const FVector2D Ctrl = (PA + PB) * 0.5f + FVector2D(-D.Y, D.X) / L * O;
		Out.Reset();
		for (int32 i = 0; i <= 10; ++i)
		{
			const float T = i / 10.f;
			Out.Add(ToS((1 - T) * (1 - T) * PA + 2 * (1 - T) * T * Ctrl + T * T * PB));
		}
	};

	// Background: dark with a warm centre.
	DrawRect(Hex(TEXT("070605")), 0.f, 0.f, PW, PH);
	for (int32 i = 6; i >= 1; --i) { Disc(ToS(FVector2D::ZeroVector), 1100.f * K * i / 6.f, Hex(TEXT("2a2016"), 0.07f)); }

	// Oak hints: roots under the heart, dashed crown ring.
	{
		static const FVector2D Roots[][3] = {
			{ FVector2D(-38, 40), FVector2D(-40, 200), FVector2D(-150, 360) }, { FVector2D(38, 40), FVector2D(40, 200), FVector2D(150, 360) },
			{ FVector2D(0, 50), FVector2D(-5, 250), FVector2D(0, 420) }, { FVector2D(-20, 60), FVector2D(-120, 200), FVector2D(-320, 280) },
			{ FVector2D(20, 60), FVector2D(120, 200), FVector2D(320, 280) } };
		for (const auto& R : Roots)
		{
			FVector2D Prev = ToS(R[0]);
			for (int32 i = 1; i <= 12; ++i)
			{
				const float T = i / 12.f;
				const FVector2D Pt = ToS((1 - T) * (1 - T) * R[0] + 2 * (1 - T) * T * R[1] + T * T * R[2]);
				DrawLine(Prev.X, Prev.Y, Pt.X, Pt.Y, Hex(TEXT("3a2e20"), 0.3f), FMath::Max(1.f, 6.f * K * (1.f - T * 0.7f)));
				Prev = Pt;
			}
		}
		Ring(ToS(FVector2D::ZeroVector), 930.f * K, Hex(TEXT("b8955a"), 0.12f), 1.f, true);
		Ring(ToS(FVector2D::ZeroVector), 560.f * K, Hex(TEXT("b8955a"), 0.07f), 1.f);
	}

	// Wood: every link is a tapered branch.
	TArray<FVector2D> C;
	for (int32 A = 0; A < Nodes.Num(); ++A)
	{
		for (int32 B : Nodes[A].Links)
		{
			if (B < A) { continue; }
			Curve(A, B, C);
			const float R = FMath::Min(Nodes[A].Pos.Size(), Nodes[B].Pos.Size());
			const float Wd = FMath::Max(2.f, 11.f - R / 70.f) * K;
			for (int32 i = 0; i + 1 < C.Num(); ++i) { DrawLine(C[i].X, C[i].Y, C[i + 1].X, C[i + 1].Y, Hex(TEXT("2b2218")), Wd + 4.f * K); }
			for (int32 i = 0; i + 1 < C.Num(); ++i) { DrawLine(C[i].X, C[i].Y, C[i + 1].X, C[i + 1].Y, Hex(TEXT("4a3b28")), Wd); }
		}
	}

	// Fog over closed sectors: dark wedge + soft layered mist blobs.
	for (int32 G = 1; G < 8; ++G)
	{
		const float A0 = -PI * 0.5f + G * PI * 0.25f, Wg = PI / 8.f;
		constexpr int32 Steps = 12;
		for (int32 i = 0; i < Steps; ++i)
		{
			const float B0 = A0 - Wg + 2.f * Wg * i / Steps, B1 = A0 - Wg + 2.f * Wg * (i + 1) / Steps;
			const FVector2D D0(FMath::Cos(B0), FMath::Sin(B0)), D1(FMath::Cos(B1), FMath::Sin(B1));
			const FVector2D I0 = ToS(D0 * 100.f), I1 = ToS(D1 * 100.f), O0 = ToS(D0 * 960.f), O1 = ToS(D1 * 960.f);
			Tri(I0, O0, O1, Hex(TEXT("0b0907"), 0.6f));
			Tri(I0, O1, I1, Hex(TEXT("0b0907"), 0.6f));
		}
		for (int32 k = 0; k < 6; ++k)
		{
			const float R = 220.f + 120.f * k, Aoff = FMath::Sin(G * 3.1f + k * 1.7f) * Wg * 0.7f;
			const FVector2D P = ToS(FVector2D(FMath::Cos(A0 + Aoff), FMath::Sin(A0 + Aoff)) * R);
			const float Drift = 1.f + 0.06f * FMath::Sin(Time * 0.3f + k + G);
			for (int32 L = 4; L >= 1; --L) { Disc(P, (70.f + 25.f * L) * K * Drift, Hex(TEXT("cfc6b0"), 0.018f)); }
		}
	}

	// Learned links glow with light flowing outward; the hover path is a dashed preview.
	TArray<int32> PreviewPath;
	if (HoveredNode >= 0) { Tree->FindPath(HoveredNode, PreviewPath); }
	auto InPreview = [&](int32 N) { return PreviewPath.Contains(N); };
	for (int32 A = 0; A < Nodes.Num(); ++A)
	{
		for (int32 B : Nodes[A].Links)
		{
			if (B < A) { continue; }
			const bool bOn = Tree->IsAllocated(A) && Tree->IsAllocated(B);
			const bool bPv = !bOn && (InPreview(A) || InPreview(B)) && (InPreview(A) || Tree->IsAllocated(A)) && (InPreview(B) || Tree->IsAllocated(B));
			if (!bOn && !bPv) { continue; }
			Curve(A, B, C);
			const float R = FMath::Min(Nodes[A].Pos.Size(), Nodes[B].Pos.Size());
			const float Wd = FMath::Max(2.f, (11.f - R / 70.f) * 0.45f) * K;
			const float Phase = FMath::Fmod(Time * 1.2f + A * 0.37f, 1.f);
			for (int32 i = 0; i + 1 < C.Num(); ++i)
			{
				if (bOn)
				{
					FLinearColor G = Lit; G.A = 0.25f;
					DrawLine(C[i].X, C[i].Y, C[i + 1].X, C[i + 1].Y, G, Wd * 3.f);
					G.A = 0.9f;
					DrawLine(C[i].X, C[i].Y, C[i + 1].X, C[i + 1].Y, G, FMath::Max(1.5f, Wd));
					if (FMath::Abs(i / 10.f - Phase) < 0.06f) { DrawLine(C[i].X, C[i].Y, C[i + 1].X, C[i + 1].Y, Hex(TEXT("e9fbff")), FMath::Max(1.5f, Wd * 0.8f)); }
				}
				else if (i % 2 == 0)
				{
					DrawLine(C[i].X, C[i].Y, C[i + 1].X, C[i + 1].Y, Hex(TEXT("e8dfc8"), 0.9f), 1.5f);
				}
			}
		}
	}

	// Nodes.
	TArray<FVector2D> Icon;
	for (int32 i = 0; i < Nodes.Num(); ++i)
	{
		const FFNNode& N = Nodes[i];
		const FVector2D P = ToS(N.Pos);
		if (P.X < -60.f || P.Y < -60.f || P.X > PW + 60.f || P.Y > PH + 60.f) { continue; }
		const bool bOpen = UFNSkillTree::IsSectorOpen(N.God);
		const bool bAlloc = Tree->IsAllocated(i);
		const bool bSeal = Tree->IsLockedByRune(i);
		bool bAvail = false;
		if (!bAlloc && Tree->IsPassable(i)) { for (int32 L : N.Links) { if (Tree->IsAllocated(L)) { bAvail = true; break; } } }
		const bool bPv = InPreview(i);
		const float R = NodeRadius(N.Kind) * K;
		const float Fade = bOpen ? 1.f : 0.35f;
		const float Breathe = bAvail && !bPv ? 0.7f + 0.3f * FMath::Sin(Time * 3.5f) : 1.f;

		FLinearColor Stroke = bAlloc ? Lit : (bPv || bAvail ? Bone : (bSeal ? Hex(TEXT("8a6fb0")) : (bOpen ? Hex(TEXT("5a5244")) : Hex(TEXT("2e2a24")))));
		Stroke.A = Fade * Breathe;
		FLinearColor Fill = bAlloc ? Hex(TEXT("10252c")) : (bOpen ? Hex(TEXT("15110d")) : Hex(TEXT("0e0c0a")));
		Fill.A = Fade;
		const float GoldA = bOpen ? 1.f : 0.12f; // gold trim nearly vanishes in the fog
		const FLinearColor GoldF(Gold.R, Gold.G, Gold.B, GoldA);

		if (bAlloc || bPv) { FLinearColor Hl = bAlloc ? Lit : Bone; Hl.A = bAlloc ? (N.Kind == EFNNodeKind::Small ? 0.22f : 0.3f) : 0.12f; Disc(P, R + 9.f * K, Hl); }

		if (N.Kind == EFNNodeKind::Root)
		{
			for (int32 L = 5; L >= 1; --L) { FLinearColor G = Lit; G.A = 0.07f; Disc(P, (R + 26.f * K) * L / 5.f + R * 0.5f, G); }
			Disc(P, R, Hex(TEXT("10171a")));
			Ring(P, R, Lit, 3.f * K);
			Disc(P, 12.f * K * (1.f + 0.08f * FMath::Sin(Time * 2.f)), Hex(TEXT("e9fbff")));
		}
		else if (N.Kind == EFNNodeKind::Notable)
		{
			const FVector2D D[] = { FVector2D(P.X, P.Y - R * 1.3f), FVector2D(P.X + R * 1.3f, P.Y), FVector2D(P.X, P.Y + R * 1.3f), FVector2D(P.X - R * 1.3f, P.Y) };
			Tri(D[0], D[1], D[2], Fill); Tri(D[0], D[2], D[3], Fill);
			for (int32 k = 0; k < 4; ++k) { DrawLine(D[k].X, D[k].Y, D[(k + 1) % 4].X, D[(k + 1) % 4].Y, Stroke, 2.f * K); }
			for (int32 k = 0; k < 4; ++k)
			{
				const FVector2D A1 = P + (D[k] - P) * 0.72f, B1 = P + (D[(k + 1) % 4] - P) * 0.72f;
				DrawLine(A1.X, A1.Y, B1.X, B1.Y, FLinearColor(Gold.R, Gold.G, Gold.B, 0.5f * GoldA), 1.f);
			}
		}
		else if (N.Kind == EFNNodeKind::Keystone)
		{
			Disc(P, R, Fill);
			Ring(P, R, Stroke, 2.5f * K);
			Ring(P, R + 6.f * K, FLinearColor(Gold.R, Gold.G, Gold.B, 0.8f * GoldA), 1.f, true);
			for (int32 k = 0; k < 8; ++k)
			{
				const FVector2D Dk(FMath::Cos(k * PI / 4.f), FMath::Sin(k * PI / 4.f));
				const FVector2D A1 = P + Dk * (R + 6.f * K), B1 = P + Dk * (R + 13.f * K);
				DrawLine(A1.X, A1.Y, B1.X, B1.Y, GoldF, 1.5f * K);
			}
		}
		else
		{
			Disc(P, R, Fill);
			Ring(P, R, Stroke, FMath::Max(1.f, 2.f * K));
		}

		// Icon.
		if (N.Icon > 0 && (N.Kind == EFNNodeKind::Notable || N.Kind == EFNNodeKind::Keystone) && R > 6.f)
		{
			Icon.Reset();
			IconLines(N.Icon, Icon);
			const float Sc = (N.Kind == EFNNodeKind::Keystone ? 1.35f : 0.95f) * K;
			const FLinearColor IC = bAlloc ? Hex(TEXT("dff7ff")) : (bOpen ? (bAvail || bPv ? Bone : Hex(TEXT("6d6555"))) : Hex(TEXT("2e2a24")));
			for (int32 k = 0; k + 1 < Icon.Num(); ++k)
			{
				if (Icon[k].X < 0.f || Icon[k + 1].X < 0.f) { continue; }
				const FVector2D A1 = P + (Icon[k] - FVector2D(12.f, 12.f)) * Sc, B1 = P + (Icon[k + 1] - FVector2D(12.f, 12.f)) * Sc;
				DrawLine(A1.X, A1.Y, B1.X, B1.Y, IC, FMath::Max(1.f, 1.6f * K));
			}
		}

		// Rune seal: violet disc with a padlock.
		if (bSeal)
		{
			Disc(P, R + 4.f * K, Hex(TEXT("2a1e38"), 0.85f));
			const FLinearColor LC = Hex(TEXT("c9a0ff"));
			const float U = FMath::Max(0.6f, K);
			const FVector2D Q[] = { FVector2D(-6, -1), FVector2D(6, -1), FVector2D(6, 8), FVector2D(-6, 8) };
			for (int32 k = 0; k < 4; ++k) { const FVector2D A1 = P + Q[k] * U, B1 = P + Q[(k + 1) % 4] * U; DrawLine(A1.X, A1.Y, B1.X, B1.Y, LC, 1.6f * U); }
			for (int32 k = 0; k < 8; ++k)
			{
				const float A0 = PI + PI * k / 8.f, A1 = PI + PI * (k + 1) / 8.f;
				const FVector2D P0 = P + FVector2D(4.f * FMath::Cos(A0), -1.f + 5.f * FMath::Sin(A0)) * U;
				const FVector2D P1 = P + FVector2D(4.f * FMath::Cos(A1), -1.f + 5.f * FMath::Sin(A1)) * U;
				DrawLine(P0.X, P0.Y, P1.X, P1.Y, LC, 1.6f * U);
			}
		}
	}

	// Sector names at the rim.
	for (int32 G = 0; G < 8; ++G)
	{
		const float A0 = -PI * 0.5f + G * PI * 0.25f;
		const FVector2D L = ToS(FVector2D(FMath::Cos(A0), FMath::Sin(A0)) * 1010.f) / S;
		FLinearColor Col = UFNSkillTree::GodColor(G); Col.A = G == 0 ? 1.f : 0.45f;
		Txt(FString(UFNSkillTree::GodName(G)).ToUpper(), L.X, L.Y - 18.f, Font(0, G == 0 ? 30.f : 24.f), Col, EAlign::Center);
		const FString Sub = G == 0 ? FString(TEXT("Гром · Грозовые Выси")) : FString::Printf(TEXT("%s · откроется в Нави"), UFNSkillTree::GodElement(G));
		FLinearColor SubCol = BoneDim; SubCol.A = G == 0 ? 1.f : 0.7f;
		Txt(Sub, L.X, L.Y + 14.f, Font(3, 16.f), SubCol, EAlign::Center);
	}

	// Vignette.
	for (int32 i = 0; i < 10; ++i)
	{
		const FLinearColor V(0.f, 0.f, 0.f, 0.075f * (1.f - i / 10.f));
		const float Bw = PW * 0.012f * (i + 1), Bh = PH * 0.012f * (i + 1);
		DrawRect(V, 0.f, 0.f, PW, Bh); DrawRect(V, 0.f, PH - Bh, PW, Bh);
		DrawRect(V, 0.f, 0.f, Bw, PH); DrawRect(V, PW - Bw, 0.f, Bw, PH);
	}

	// Chrome: title, points and seals, legend, controls.
	Txt(TEXT("ДРЕВО"), W * 0.5f, 12.f, Font(0, 32.f), Bone, EAlign::Center);
	Txt(TEXT("восемь ветвей — восемь богов  ·  открыта ветвь Перуна"), W * 0.5f, 52.f, Font(3, 16.f), BoneDim, EAlign::Center);
	Ornament(W * 0.5f, 80.f, 230.f);
	Txt(FString::FromInt(Points), W - 28.f, 14.f, Font(1, 40.f), Points > 0 ? Bone : BoneDim, EAlign::Right);
	Txt(TEXT("очков древа"), W - 28.f, 60.f, Font(2, 15.f), BoneDim, EAlign::Right);
	Txt(FString::Printf(TEXT("печати: %d / %d"), Tree->NumRunesFound(), UFNSkillTree::RuneOrder().Num()), W - 28.f, 84.f, Font(2, 17.f), Gold, EAlign::Right);

	const TCHAR* LegText[] = { TEXT("изучено"), TEXT("доступно"), TEXT("нужен путь"), TEXT("под печатью — нужна руна-ключ"), TEXT("в тумане — ветвь другого бога") };
	const FLinearColor LegCol[] = { Lit, Bone, Hex(TEXT("5a5244")), Hex(TEXT("c9a0ff")), Hex(TEXT("3a352c")) };
	for (int32 i = 0; i < 5; ++i)
	{
		const float LY = H - 128.f + i * 21.f;
		FillDisc(32.f, LY + 11.f, 5.f, LegCol[i]);
		Txt(LegText[i], 44.f, LY, Font(2, 15.f), BoneDim);
	}
	Txt(TEXT("ЛКМ — изучить путь  ·  ПКМ — вернуть очко"), W - 28.f, H - 64.f, Font(3, 15.f), BoneDim, EAlign::Right);
	Txt(TEXT("колесо — масштаб  ·  тащить — сдвиг  ·  Tab — закрыть"), W - 28.f, H - 44.f, Font(3, 15.f), BoneDim, EAlign::Right);

	// Tooltip card next to the cursor.
	if (Nodes.IsValidIndex(HoveredNode))
	{
		const FFNNode& N = Nodes[HoveredNode];
		const bool bOpen = UFNSkillTree::IsSectorOpen(N.God);
		static const TCHAR* Kinds[] = { TEXT("СЕРДЦЕ"), TEXT("МАЛЫЙ УЗЕЛ"), TEXT("ЗНАЧИМЫЙ УЗЕЛ"), TEXT("КЛЮЧЕВОЙ УЗЕЛ") };
		const FString Head = FString::Printf(TEXT("%s  ·  %s"), Kinds[static_cast<int32>(N.Kind)], *FString(UFNSkillTree::GodName(N.God)).ToUpper());
		const FString Name = bOpen ? N.Name : FString(TEXT("???"));
		const FString Desc = bOpen ? N.Desc : (N.God >= 0 ? FString::Printf(TEXT("Ветвь в тумане. Откроется, когда %s вернётся — в Нави."), UFNSkillTree::GodName(N.God)) : N.Desc);
		FString Foot;
		FLinearColor FootCol = Gold;
		if (bOpen)
		{
			if (Tree->IsLockedByRune(HoveredNode)) { Foot = TEXT("Под печатью: нужна руна-ключ. Её роняют твари Высей."); FootCol = Hex(TEXT("c9a0ff")); }
			else if (Tree->IsAllocated(HoveredNode)) { Foot = HoveredNode == 0 ? TEXT("Начало пути") : TEXT("Изучено  ·  ПКМ — вернуть"); }
			else if (PreviewPath.Num() > 0)
			{
				const int32 Cost = PreviewPath.Num();
				Foot = FString::Printf(TEXT("Путь: %d %s%s"), Cost, Cost == 1 ? TEXT("очко") : (Cost < 5 ? TEXT("очка") : TEXT("очков")), Cost > Points ? TEXT("  —  не хватает") : TEXT(""));
				if (Cost > Points) { FootCol = BloodHi; }
			}
			else { Foot = TEXT("Нет пути: мешает печать"); FootCol = BoneDim; }
		}
		float DW = 0.f, DH = 0.f, NW = 0.f, NH = 0.f, FW = 0.f, FH = 0.f;
		GetTextSize(Desc, DW, DH, Font(2, 17.f));
		GetTextSize(Name, NW, NH, Font(1, 22.f));
		GetTextSize(Foot, FW, FH, Font(2, 15.f));
		const float CardW = FMath::Max3(260.f, DW / S, FMath::Max(NW, FW) / S) + 32.f;
		const float CardH = Foot.IsEmpty() ? 92.f : 118.f;
		float MX = 0.f, MY = 0.f;
		if (APlayerController* PC = GetOwningPlayerController()) { PC->GetMousePosition(MX, MY); }
		const float TX = FMath::Min(MX / S + 18.f, W - CardW - 10.f), TY = FMath::Min(MY / S + 14.f, H - CardH - 10.f);
		FillBevel(TX, TY, CardW, CardH, 10.f, Hex(TEXT("120e0a"), 0.95f));
		LineBevel(TX, TY, CardW, CardH, 10.f, Line, 1.f);
		Txt(Head, TX + 16.f, TY + 10.f, Font(2, 13.f), BoneDim);
		Txt(Name, TX + 16.f, TY + 28.f, Font(1, 22.f), Bone);
		Txt(Desc, TX + 16.f, TY + 58.f, Font(2, 17.f), Bone);
		if (!Foot.IsEmpty()) { Txt(Foot, TX + 16.f, TY + 88.f, Font(2, 15.f), FootCol); }
	}
}

void AFNHUD::DrawMap(AFNCharacter* Player)
{
	const float W = Canvas->ClipX / S, H = 720.f;
	const float PanelW = FMath::Min(W - 72.f, 980.f);
	const float PanelH = 510.f;
	const float X0 = (W - PanelW) * 0.5f, Y0 = (H - PanelH) * 0.5f;

	// A restrained map plate: the world stays dimly visible behind it, like ink on old bark.
	DrawRect(Hex(TEXT("050403"), 0.78f), 0.f, 0.f, Canvas->ClipX, Canvas->ClipY);
	FillBevel(X0, Y0, PanelW, PanelH, 14.f, Hex(TEXT("17120d"), 0.97f));
	LineBevel(X0, Y0, PanelW, PanelH, 14.f, Line, 1.5f);
	Txt(TEXT("ГРОЗОВЫЕ ВЫСИ"), W * 0.5f, Y0 + 22.f, Font(0, 30.f), Bone, EAlign::Center);
	Txt(TEXT("Карта Яви  ·  M / Esc — закрыть"), W * 0.5f, Y0 + 60.f, Font(3, 16.f), BoneDim, EAlign::Center);
	Ornament(W * 0.5f, Y0 + 78.f, 155.f);

	const float MapX = X0 + 38.f, MapY = Y0 + 98.f, MapW = PanelW - 76.f, MapH = 310.f;
	const float MinX = -80.f, MaxX = 930.f, MinY = -190.f, MaxY = 190.f;
	const float Scale = FMath::Min(MapW / (MaxX - MinX), MapH / (MaxY - MinY));
	const FVector2D MapCenter(MapX + MapW * 0.5f, MapY + MapH * 0.5f);
	auto ToMap = [&](const FVector2D& P)
	{
		return MapCenter + FVector2D((P.X - (MinX + MaxX) * 0.5f) * Scale, -(P.Y - (MinY + MaxY) * 0.5f) * Scale);
	};

	// Faint coordinate lines keep the route readable without turning this into a minimap HUD.
	for (int32 i = 1; i < 5; ++i)
	{
		const float GX = MapX + MapW * i / 5.f, GY = MapY + MapH * i / 5.f;
		DrawLine(GX * S, MapY * S, GX * S, (MapY + MapH) * S, Hex(TEXT("b8955a"), 0.10f), 1.f * S);
		DrawLine(MapX * S, GY * S, (MapX + MapW) * S, GY * S, Hex(TEXT("b8955a"), 0.10f), 1.f * S);
	}

	// Main route, with a second dim stroke suggesting the remembered road beneath it.
	for (int32 i = 0; i + 1 < UE_ARRAY_COUNT(MapPath); ++i)
	{
		const FVector2D A = ToMap(MapPath[i]), B = ToMap(MapPath[i + 1]);
		DrawLine(A.X * S, A.Y * S, B.X * S, B.Y * S, Hex(TEXT("5a4730"), 0.8f), 8.f * S);
		DrawLine(A.X * S, A.Y * S, B.X * S, B.Y * S, Gold, 2.2f * S);
	}

	const FVector P = Player ? Player->GetActorLocation() / 100.f : FVector::ZeroVector;
	const FVector2D Me = ToMap(FVector2D(P.X, P.Y));
	FillDisc(Me.X, Me.Y, 8.f, Player ? Player->GetSparkColor() : Bone);
	DrawRing(Me.X, Me.Y, 14.f, Player ? Player->GetSparkColor() : Bone, 1.5f);
	DrawRing(Me.X, Me.Y, 20.f, Hex(TEXT("e8dfc8"), 0.25f), 1.f);

	const FVector2D Treba = ToMap(FVector2D(645.f, 10.f));
	FillDisc(Treba.X, Treba.Y, 5.f, Hex(TEXT("d88b3a")));
	DrawRing(Treba.X, Treba.Y, 10.f, Hex(TEXT("e8c9a0"), 0.65f), 1.f);
	const FVector2D Arena = ToMap(FVector2D(720.f, 0.f));
	DrawRing(Arena.X, Arena.Y, 9.f, BloodHi, 2.f);

	struct FMapLabel { const TCHAR* Text; FVector2D Point; };
	const FMapLabel Labels[] = {
		{ TEXT("Сухоречье"), FVector2D(0.f, 0.f) }, { TEXT("Околица"), FVector2D(140.f, 38.f) },
		{ TEXT("Присяжный камень"), FVector2D(280.f, -20.f) }, { TEXT("Стрелокопни"), FVector2D(445.f, 0.f) },
		{ TEXT("Ведёрный ряд"), FVector2D(560.f, 20.f) }, { TEXT("Треба"), FVector2D(645.f, 10.f) },
		{ TEXT("Экзамен"), FVector2D(720.f, 0.f) },
	};
	for (const FMapLabel& L : Labels)
	{
		const FVector2D Q = ToMap(L.Point);
		Txt(L.Text, Q.X, Q.Y + 16.f, Font(2, 14.f), BoneDim, EAlign::Center);
	}

	Txt(TEXT("●  герой"), X0 + 32.f, Y0 + PanelH - 58.f, Font(2, 15.f), Bone, EAlign::Left);
	Txt(TEXT("◆  треба"), X0 + 145.f, Y0 + PanelH - 58.f, Font(2, 15.f), Gold, EAlign::Left);
	Txt(TEXT("○  экзамен"), X0 + 255.f, Y0 + PanelH - 58.f, Font(2, 15.f), BloodHi, EAlign::Left);
	Txt(FString(TEXT("Текущая зона: ")) + ZoneName(P.X), X0 + PanelW - 28.f, Y0 + PanelH - 58.f, Font(2, 15.f), BoneDim, EAlign::Right);
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
	if (Player && Player->IsMapOpen())
	{
		DrawMap(Player);
		return;
	}
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
		Txt(TEXT("WASD — ход   ·   Курсор — цель   ·   ЛКМ — выстрел   ·   ПКМ — удар   ·   Пробел — уклонение   ·   R — перезарядка   ·   Q — оружие   ·   Колесо — масштаб   ·   1–3 — навыки   ·   Tab — древо   ·   M — карта"),
			W * 0.5f, 604.f, Font(3, 15.f), BoneDim, EAlign::Center);
	}

	if (Player)
	{
		// ---- The combat reticle follows the screen cursor, not the centre of the isometric camera.
		{
			const bool bHit = Player->GetTimeSinceHit() < 0.12f;
			const FLinearColor C = bHit ? (Player->WasLastHitWeak() ? Hex(TEXT("ffb35c")) : BloodHi) : Hex(TEXT("e8dfc8"), 0.8f);
			float MouseX = Canvas->ClipX * 0.5f, MouseY = Canvas->ClipY * 0.5f;
			if (APlayerController* PC = GetOwningPlayerController()) { PC->GetMousePosition(MouseX, MouseY); }
			const float CX = MouseX / S, CY = MouseY / S;
			const float R = Player->IsFiring() ? 8.f : 11.f;
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
				if (bOpen && Player->GetPanelSkill(i) >= 0)
				{
					// Gem: rune frame by tag (square = strike, diamond = shot, circle = spell) in the thunder colour,
					// short name, Yar cost; dark sweep while on cooldown, dimmed when Yar is short.
					const FFNSkillDef& D = FNSkills::Def(static_cast<EFNSkillId>(Player->GetPanelSkill(i)));
					const bool bAfford = Player->CanAffordSkill(i);
					FLinearColor RC = Player->GetSparkColor(); RC.A = bAfford ? 1.f : 0.35f;
					const float CX = SX + Slot * 0.5f, CY = SY + Slot * 0.5f - 3.f, RR = 11.f;
					if (D.Tag == EFNSkillTag::Strike) { LineBevel(CX - RR, CY - RR, 2.f * RR, 2.f * RR, 0.f, RC, 1.5f); }
					else if (D.Tag == EFNSkillTag::Shot)
					{
						const FVector2D P4[] = { { CX, CY - RR - 2.f }, { CX + RR + 2.f, CY }, { CX, CY + RR + 2.f }, { CX - RR - 2.f, CY } };
						for (int32 k = 0; k < 4; ++k) { DrawLine(P4[k].X * S, P4[k].Y * S, P4[(k + 1) % 4].X * S, P4[(k + 1) % 4].Y * S, RC, 1.5f * S); }
					}
					else { DrawRing(CX, CY, RR, RC, 1.5f); }
					// Bolt glyph (all demo gems are thunder).
					const FVector2D Bolt[] = { { CX + 2.f, CY - 7.f }, { CX - 3.f, CY + 1.f }, { CX + 1.f, CY + 1.f }, { CX - 2.f, CY + 7.f } };
					for (int32 k = 0; k < 3; ++k) { DrawLine(Bolt[k].X * S, Bolt[k].Y * S, Bolt[k + 1].X * S, Bolt[k + 1].Y * S, RC, 1.3f * S); }
					Txt(D.Short, CX, SY + Slot - 15.f, Font(2, 11.f), bAfford ? Bone : BoneDim, EAlign::Center);
					Txt(FString::Printf(TEXT("%.0f"), D.YarCost), SX + Slot - 4.f, SY + 1.f, Font(2, 11.f), Player->GetSparkColor(), EAlign::Right);
					const float CD = Player->GetSkillCooldownRatio(i);
					if (CD > 0.f) { DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), (SX + 1.f) * S, (SY + 1.f + (Slot - 2.f) * (1.f - CD)) * S, (Slot - 2.f) * S, (Slot - 2.f) * CD * S); }
				}
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

			// Yar: amber bar over the slots; flares after a perfect dodge.
			{
				const float YW = 4.f * Slot + 3.f * Gap, YY = SY - 9.f;
				DrawRect(Ink, X0 * S, YY * S, YW * S, 5.f * S);
				const float Flare = FMath::Clamp(1.f - Player->GetPerfectDodgeAge() / 0.6f, 0.f, 1.f);
				// Yar in the hero element colour (no copper = Svarog, no pale gold = loot); brighter on a perfect dodge.
				const FLinearColor Elem = Player->GetSparkColor();
				FLinearColor YC = FMath::Lerp(Elem * 0.75f, FLinearColor(0.85f, 0.88f, 1.f), Flare * 0.6f); YC.A = 1.f;
				DrawRect(YC, X0 * S, YY * S, YW * Player->GetYarRatio() * S, 5.f * S);
				Txt(TEXT("ярь"), X0 - 6.f, YY - 7.f, Font(3, 13.f), BoneDim, EAlign::Right);
				if (Flare > 0.f) { FLinearColor TC = Elem; TC.A = Flare; Txt(TEXT("точно!"), X0 + YW * 0.5f, YY - 26.f, Font(1, 18.f), TC, EAlign::Center); }
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
			for (const TCHAR* K : { TEXT("Q") })
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

		// ---- Interaction prompt (rites): key box + "Коза — подоить"; greyed with the reason when not possible.
		{
			bool bCan = false;
			const FString Prompt = Player->GetFocusPrompt(bCan);
			if (!Prompt.IsEmpty())
			{
				float TW = 0.f, TH = 0.f;
				GetTextSize(Prompt, TW, TH, Font(2, 18.f));
				const float PY = H - 212.f;
				if (bCan)
				{
					const float KX = W * 0.5f - TW / S * 0.5f - 34.f;
					FillBevel(KX, PY + 1.f, 24.f, 22.f, 4.f, Ink2);
					LineBevel(KX, PY + 1.f, 24.f, 22.f, 4.f, Line, 1.f);
					Txt(TEXT("E"), KX + 12.f, PY + 1.f, Font(1, 16.f), Bone, EAlign::Center);
					Txt(Prompt, W * 0.5f + 8.f, PY, Font(2, 18.f), Bone, EAlign::Center);
				}
				else
				{
					Txt(Prompt, W * 0.5f, PY, Font(3, 17.f), BoneDim, EAlign::Center);
				}
			}
		}

		// ---- Satchel (rite items), above the Spark emblem.
		if (Player->GetSatchel().Num() > 0)
		{
			float SY = H - 30.f - 72.f - 26.f - 20.f * Player->GetSatchel().Num();
			Txt(TEXT("сума"), 30.f, SY, Font(3, 14.f), BoneDim);
			for (const FName& Item : Player->GetSatchel())
			{
				SY += 20.f;
				Txt(FString(TEXT("·  ")) + AFNRiteObject::ItemName(Item), 30.f, SY, Font(2, 16.f), Bone);
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

	// Subtitles: gold speaker in small caps, italic line (the boss, or an NPC the hero talks to).
	const bool bBossSub = Boss && Boss->HasSubtitle();
	if (bBossSub || (Player && Player->HasSubtitle()))
	{
		const FString Who = (bBossSub ? Boss->GetSubtitleSpeaker() : Player->GetSubSpeaker()).ToUpper();
		const FString Line1 = bBossSub ? Boss->GetSubtitle() : Player->GetSubText();
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
