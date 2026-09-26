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
	DrawCentered(FString::Printf(TEXT("SKILL TREE   points: %d   rune-keys: %d / %d   (Tab to close)"),
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
		const FString Title = bLocked ? FString(TEXT("??? (find its rune-key)")) : FString(N.Name);
		const FString Body = bLocked ? FString(TEXT("A rune-key dropped by the Heights' creatures will reveal this node.")) : FString(N.Desc);
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

		// Ammo per weapon
		FString AmmoText;
		if (Player->IsReloading()) { AmmoText = TEXT("RELOADING"); }
		else if (Player->GetWeapon() == EFNWeapon::Plasma) { AmmoText = TEXT("PLASMA"); }
		else if (Player->GetWeapon() == EFNWeapon::Scatter) { AmmoText = FString::Printf(TEXT("%d / %d"), Player->GetScatterAmmo(), Player->GetReserve()); }
		else { AmmoText = FString::Printf(TEXT("%d / %d"), Player->GetAmmo(), Player->GetReserve()); }
		DrawText(AmmoText, FLinearColor(1.f, 0.85f, 0.5f), W - 220.f, H - 90.f, Medium, 1.4f);

		// Stage and weapon
		static const TCHAR* StageNames[] = { TEXT("SPARK"), TEXT("SKELETON"), TEXT("FLESH") };
		static const TCHAR* WeaponNames[] = { TEXT("plasma [1]"), TEXT("rifle [2]"), TEXT("scattergun [3]") };
		DrawText(FString::Printf(TEXT("%s  |  %s"), StageNames[static_cast<int32>(Player->GetStage())], WeaponNames[static_cast<int32>(Player->GetWeapon())]),
			FLinearColor(0.85f, 0.9f, 1.f), 40.f, H - 120.f, Small);

		if (Player->GetSkillPoints() > 0)
		{
			DrawText(FString::Printf(TEXT("Tab - skill tree (%d points)"), Player->GetSkillPoints()), FLinearColor(1.f, 0.85f, 0.5f), 40.f, H - 145.f, Small);
		}

		// Short event messages (evolution, pickups, treba).
		if (Player->GetMessageAge() < 3.5f && !Player->GetMessage().IsEmpty())
		{
			DrawCentered(Player->GetMessage(), H * 0.22f, FLinearColor(1.f, 0.9f, 0.7f), 0.9f);
		}

		// Kill counter (drives the Spark -> Skeleton -> Flesh evolution).
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
	if (Player && Player->IsDead() && !Player->IsExamDefeat())
	{
		DrawCentered(TEXT("The Spark gutters..."), H * 0.4f, FLinearColor(0.7f, 0.9f, 1.f), 1.1f);
	}
	else if (Player && Player->IsExamDefeat())
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
