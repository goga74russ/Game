#include "FNInventory.h"

#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "FNCharacter.h"
#include "FNRite.h"
#include "FNSkills.h"

FFNInventorySnapshot FFNInventorySnapshot::Capture(const AFNCharacter& Hero, bool bAtTreba)
{
	FFNInventorySnapshot Result;
	Result.bCanEdit = bAtTreba && !Hero.IsDead();
	for (int32 I = 0; I < static_cast<int32>(EFNSkillId::Count); ++I)
	{
		if (Hero.HasSkill(I)) { Result.OwnedSkills.Add(I); }
	}
	for (int32 I = 0; I < 3; ++I) { Result.Panel[I] = Hero.GetPanelSkill(I); }
	const EFNStage Stage = Hero.GetStage();
	Result.bSpark = Stage == EFNStage::Spark;
	Result.Equipment.Add(Stage == EFNStage::Spark ? TEXT("Ближнее: вспышка Искры") :
		Stage == EFNStage::Skeleton ? TEXT("Ближнее: удар рукой") : TEXT("Ближнее: колун"));
	if (Hero.HasWeapon(EFNWeapon::Plasma)) { Result.Equipment.Add(TEXT("Дальнее: плазма Искры")); }
	if (Hero.HasWeapon(EFNWeapon::Rifle)) { Result.Equipment.Add(TEXT("Ружьё") + FString(Hero.GetWeapon() == EFNWeapon::Rifle ? TEXT(" — в руках") : TEXT(" — в запасе"))); }
	if (Hero.HasWeapon(EFNWeapon::Scatter)) { Result.Equipment.Add(TEXT("Дробовик") + FString(Hero.GetWeapon() == EFNWeapon::Scatter ? TEXT(" — в руках") : TEXT(" — в запасе"))); }
	if (!Hero.HasRangedWeapon()) { Result.Equipment.Add(TEXT("Дальнее оружие ещё не найдено")); }
	Result.Equipment.Add(Hero.GetArmor() > 0.f ? FString::Printf(TEXT("Доспех: +%.0f здоровья"), Hero.GetArmor()) : TEXT("Доспех не найден"));
	if (Hero.HasHelmet()) { Result.Equipment.Add(TEXT("Шлем из обряда")); }
	for (FName Item : Hero.GetSatchel()) { Result.Satchel.Add(AFNRiteObject::ItemName(Item)); }
	return Result;
}

bool FNInventory::PlanAssignment(const FFNInventorySnapshot& State, int32 Skill, int32 Slot,
	TArray<int32>& OutPanel, FString& OutError)
{
	OutError.Reset();
	if (!State.bCanEdit) { OutError = TEXT("Камни можно менять только у требы."); return false; }
	if (Slot < 0 || Slot >= 3 || State.Panel.Num() != 3) { OutError = TEXT("Недоступная ячейка навыка."); return false; }
	if (Skill != -1 && (Skill < 0 || Skill >= static_cast<int32>(EFNSkillId::Count) || !State.OwnedSkills.Contains(Skill)))
	{ OutError = TEXT("Этот камень ещё не найден."); return false; }
	TSet<int32> Seen;
	for (int32 Id : State.Panel)
	{
		if (Id == -1) { continue; }
		if (Id < 0 || Id >= static_cast<int32>(EFNSkillId::Count) || !State.OwnedSkills.Contains(Id) || Seen.Contains(Id))
		{ OutError = TEXT("Сборка содержит недоступный или повторный камень."); return false; }
		Seen.Add(Id);
	}
	TArray<int32> Proposed = State.Panel;
	const int32 Existing = Skill == -1 ? INDEX_NONE : Proposed.Find(Skill);
	if (Existing != INDEX_NONE) { Swap(Proposed[Existing], Proposed[Slot]); }
	else { Proposed[Slot] = Skill; }
	OutPanel = MoveTemp(Proposed);
	return true;
}

void FFNInventoryScreen::Draw(UCanvas* Canvas, const FFNInventorySnapshot& State,
	UFont* TitleFont, UFont* TextFont, const TMap<int32, UTexture2D*>& Icons)
{
	Hits.Reset();
	if (!Canvas || Canvas->ClipX <= 0.f || Canvas->ClipY <= 0.f) { return; }
	if (SelectedSkill >= 0 && !State.OwnedSkills.Contains(SelectedSkill)) { SelectedSkill = -1; }
	if (!TextFont) { TextFont = GEngine->GetSmallFont(); }
	if (!TitleFont) { TitleFont = TextFont; }
	const float S = FMath::Min(Canvas->ClipX / 1280.f, Canvas->ClipY / 720.f);
	const FVector2D Origin((Canvas->ClipX - 1180.f * S) / 2.f, (Canvas->ClipY - 640.f * S) / 2.f);
	const FLinearColor Ink(FColor::FromHex(TEXT("15110D"))), Bone(FColor::FromHex(TEXT("E8DFC8"))),
		Dim(FColor::FromHex(TEXT("A89F8A"))), Accent(FColor::FromHex(TEXT("8FA8FF")));
	auto Rect = [&](float X, float Y, float W, float H, FLinearColor Color)
	{
		FCanvasTileItem Item(Origin + FVector2D(X, Y) * S, FVector2D(W, H) * S, Color);
		Item.BlendMode = SE_BLEND_Translucent; Canvas->DrawItem(Item);
	};
	auto Text = [&](const FString& Value, float X, float Y, FLinearColor Color, bool Title = false)
	{
		FCanvasTextItem Item(Origin + FVector2D(X, Y) * S, FText::FromString(Value), Title ? TitleFont : TextFont, Color);
		Item.Scale = FVector2D(S, S); Canvas->DrawItem(Item);
	};
	auto Box = [&](float X, float Y, float W, float H, bool Highlight)
	{
		Rect(X, Y, W, H, Highlight ? Accent : Dim);
		Rect(X + 1.f, Y + 1.f, W - 2.f, H - 2.f, Ink);
	};
	auto Hit = [&](float X, float Y, float W, float H, int32 Skill, int32 Slot)
	{
		Hits.Add({ FBox2D(Origin + FVector2D(X, Y) * S, Origin + FVector2D(X + W, Y + H) * S), Skill, Slot });
	};
	FCanvasTileItem Shade(FVector2D::ZeroVector, FVector2D(Canvas->ClipX, Canvas->ClipY), FLinearColor(0, 0, 0, 0.85f));
	Shade.BlendMode = SE_BLEND_Translucent; Canvas->DrawItem(Shade);
	Box(0, 0, 1180, 640, false);
	Text(TEXT("СНАРЯЖЕНИЕ И КАЛИТА"), 28, 20, Bone, true);
	Text(TEXT("I / Esc — закрыть"), 960, 22, Dim);
	Text(TEXT("Снаряжение"), 28, 76, Bone, true);
	float Y = 112;
	for (const FString& Item : State.Equipment) { Text(Item, 28, Y, Bone); Y += 28; }
	Text(TEXT("Калита — обрядовые предметы"), 28, 328, Bone, true);
	for (int32 I = 0; I < AFNCharacter::SatchelSize; ++I)
	{
		Box(28, 366 + I * 34, 310, 30, false);
		Text(State.Satchel.IsValidIndex(I) ? State.Satchel[I] : TEXT("—"), 38, 370 + I * 34, State.Satchel.IsValidIndex(I) ? Bone : Dim);
	}
	Text(TEXT("Найденные камни"), 376, 76, Bone, true);
	for (int32 I = 0; I < static_cast<int32>(EFNSkillId::Count); ++I)
	{
		const float X = 376 + (I % 3) * 155, CardY = 112 + (I / 3) * 136;
		const bool Owned = State.OwnedSkills.Contains(I);
		Box(X, CardY, 145, 126, SelectedSkill == I);
		if (Owned)
		{
			UTexture2D* const* Icon = Icons.Find(I);
			if (Icon && *Icon)
			{
				FCanvasTileItem Tile(Origin + FVector2D(X + 36, CardY + 8) * S, (*Icon)->GetResource(), FVector2D(72, 72) * S, FLinearColor::White);
				Tile.BlendMode = SE_BLEND_Translucent; Canvas->DrawItem(Tile);
			}
			else { Text(FNSkills::Def(static_cast<EFNSkillId>(I)).Short, X + 12, CardY + 30, Accent); }
			Text(FNSkills::Def(static_cast<EFNSkillId>(I)).Name, X + 8, CardY + 90, Bone);
			Hit(X, CardY, 145, 126, I, -1);
		}
		else { Text(TEXT("Не найден"), X + 12, CardY + 46, Dim); }
	}
	Box(864, 112, 288, 264, false);
	if (SelectedSkill >= 0)
	{
		const FFNSkillDef& Def = FNSkills::Def(static_cast<EFNSkillId>(SelectedSkill));
		Text(Def.Name, 880, 128, Bone, true);
		Text(Def.Tag == EFNSkillTag::Strike ? TEXT("Удар") : Def.Tag == EFNSkillTag::Shot ? TEXT("Выстрел") : TEXT("Чары"), 880, 166, Dim);
		const float Cost = Def.YarCost + (State.bSpark && SelectedSkill == static_cast<int32>(EFNSkillId::ChainSpark) ? 5.f : 0.f);
		Text(FString::Printf(TEXT("Ярь: %.0f   Откат: %.1f с"), Cost, Def.Cooldown), 880, 200, Bone);
		const TCHAR* Effect[] = { TEXT("Удар по области перед героем"), TEXT("Молния с задержкой и оглушением"), TEXT("Рывок со вспышкой"), TEXT("Выстрел цепью между врагами"), TEXT("Щит вокруг героя") };
		Text(Effect[SelectedSkill], 880, 234, Accent);
		if (Def.RangeCm > 0) { Text(FString::Printf(TEXT("Дальность: %.0f м"), Def.RangeCm / 100.f), 880, 266, Dim); }
		else if (Def.RadiusCm > 0) { Text(FString::Printf(TEXT("Радиус: %.0f м"), Def.RadiusCm / 100.f), 880, 266, Dim); }
		if (Def.AmmoCost > 0) { Text(FString::Printf(TEXT("Патроны: %d"), Def.AmmoCost), 880, 296, Dim); }
		Text(TEXT("Выбери ячейку 1–3 ниже"), 880, 338, Dim);
	}
	else { Text(TEXT("Выбери найденный камень"), 880, 140, Dim); }
	Text(TEXT("Активные навыки"), 376, 406, Bone, true);
	for (int32 I = 0; I < 3; ++I)
	{
		const float X = 376 + I * 155;
		Box(X, 444, 145, 74, State.bCanEdit);
		const int32 Id = State.Panel.IsValidIndex(I) ? State.Panel[I] : -1;
		Text(FString::Printf(TEXT("%d"), I + 1), X + 8, 452, Accent);
		Text(Id >= 0 && Id < static_cast<int32>(EFNSkillId::Count) ? FNSkills::Def(static_cast<EFNSkillId>(Id)).Short : TEXT("Пусто"), X + 8, 482, Bone);
		Hit(X, 444, 145, 74, -1, I);
	}
	Text(TEXT("Поддержки — следующий шаг"), 864, 446, Dim);
	Text(State.bCanEdit ? TEXT("У требы: выбери камень, затем ячейку. Повторный камень меняет ячейки местами.") : TEXT("В пути — просмотр. Менять камни можно у требы."), 376, 542, State.bCanEdit ? Accent : Dim);
	Text(Notice, 376, 580, Bone);
}

bool FFNInventoryScreen::Click(const FVector2D& Point, const FFNInventorySnapshot& State,
	TFunctionRef<bool(const TArray<int32>&, FString&)> Apply)
{
	for (const FHit& Hit : Hits)
	{
		if (!Hit.Rect.IsInsideOrOn(Point)) { continue; }
		if (Hit.Skill >= 0) { SelectedSkill = Hit.Skill; Notice.Reset(); return true; }
		if (SelectedSkill < 0) { Notice = TEXT("Сначала выбери камень."); return true; }
		TArray<int32> Proposed;
		if (FNInventory::PlanAssignment(State, SelectedSkill, Hit.Slot, Proposed, Notice))
		{
			if (Proposed == State.Panel) { Notice = TEXT("Камень уже в этой ячейке."); }
			else if (Apply(Proposed, Notice)) { Notice = TEXT("Сборка обновлена."); }
			else if (Notice.IsEmpty()) { Notice = TEXT("Сборку сейчас изменить нельзя."); }
		}
		return true;
	}
	return false;
}
