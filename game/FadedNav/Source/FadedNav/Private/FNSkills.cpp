#include "FNSkills.h"

#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	TArray<FFNSkillDef> Defs;
	bool bLoaded = false;

	void Defaults()
	{
		auto Make = [](const TCHAR* Id, const TCHAR* Name, const TCHAR* Short, EFNSkillTag Tag, float Yar, float Cd, float Dmg, float R, float Range, float Arc, float Delay, float Dur, float Stun, int32 Ammo, float IF, const TCHAR* Verb)
		{
			FFNSkillDef D;
			D.Id = Id; D.Name = Name; D.Short = Short; D.Tag = Tag; D.YarCost = Yar; D.Cooldown = Cd; D.Damage = Dmg;
			D.RadiusCm = R * 100.f; D.RangeCm = Range * 100.f; D.ArcDeg = Arc; D.Delay = Delay; D.Duration = Dur; D.Stun = Stun;
			D.AmmoCost = Ammo; D.IFrames = IF; D.RuneVerb = Verb;
			return D;
		};
		using T = EFNSkillTag;
		Defs = {
			Make(TEXT("thunder_strike"), TEXT("Громовой удар"), TEXT("Удар"), T::Strike, 20, 4, 1.8f, 4, 0, 120, 0, 0, 0, 0, 0, TEXT("площадь")),
			Make(TEXT("lightning_rod"), TEXT("Громоотвод"), TEXT("Отвод"), T::Spell, 30, 10, 45, 3, 8, 360, 1.5f, 0, 0.8f, 0, 0, TEXT("обездвиживание")),
			Make(TEXT("flare"), TEXT("Сполох"), TEXT("Сполох"), T::Spell, 25, 7, 15, 0, 6, 360, 0, 0, 0, 0, 0.25f, TEXT("рывок")),
			Make(TEXT("chain_spark"), TEXT("Цепная искра"), TEXT("Цепь"), T::Shot, 20, 3, 1.0f, 0, 40, 360, 0, 0, 0, 1, 0, TEXT("цепь")),
			Make(TEXT("storm_ward"), TEXT("Оберег грозы"), TEXT("Оберег"), T::Spell, 35, 18, 40, 4, 0, 360, 0, 5, 0, 0, 0, TEXT("щит")),
		};
	}

	// Minimal CSV line split with quotes ("a, b" stays one cell).
	TArray<FString> SplitCsv(const FString& Line)
	{
		TArray<FString> Out;
		FString Cur;
		bool bQuoted = false;
		for (int32 i = 0; i < Line.Len(); ++i)
		{
			const TCHAR C = Line[i];
			if (C == '"') { if (bQuoted && i + 1 < Line.Len() && Line[i + 1] == '"') { Cur.AppendChar('"'); ++i; } else { bQuoted = !bQuoted; } }
			else if (C == ',' && !bQuoted) { Out.Add(Cur); Cur.Reset(); }
			else { Cur.AppendChar(C); }
		}
		Out.Add(Cur);
		return Out;
	}
}

FString FNSkills::Reload()
{
	Defaults();
	bLoaded = true;
	// Dev: the docs table next to the project; packaged: a staged copy in the project's Data folder.
	const FString Candidates[] = {
		FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../../docs/systems/skills/C1-Yav/skills.csv")),
		FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Data/skills/C1-Yav/skills.csv")),
	};
	for (const FString& Path : Candidates)
	{
		TArray<FString> Lines;
		if (!FFileHelper::LoadFileToStringArray(Lines, *Path) || Lines.Num() < 2) { continue; }
		const TArray<FString> Head = SplitCsv(Lines[0]);
		auto Col = [&Head](const TCHAR* Name) { return Head.IndexOfByKey(FString(Name)); };
		const int32 CId = Col(TEXT("id")), CName = Col(TEXT("name_ru")), CTag = Col(TEXT("tag")), CYar = Col(TEXT("yar_cost")), CCd = Col(TEXT("cooldown_s")),
			CDmg = Col(TEXT("damage")), CR = Col(TEXT("radius_m")), CRange = Col(TEXT("range_m")), CArc = Col(TEXT("arc_deg")), CDelay = Col(TEXT("delay_s")),
			CDur = Col(TEXT("duration_s")), CStun = Col(TEXT("stun_s")), CAmmo = Col(TEXT("ammo_cost")), CIF = Col(TEXT("iframes_s")), CNotes = Col(TEXT("notes")), CVerb = Col(TEXT("rune_verb"));
		int32 Applied = 0;
		for (int32 L = 1; L < Lines.Num(); ++L)
		{
			const TArray<FString> Row = SplitCsv(Lines[L]);
			if (!Row.IsValidIndex(CId)) { continue; }
			FFNSkillDef* D = Defs.FindByPredicate([&](const FFNSkillDef& X) { return X.Id == Row[CId].TrimStartAndEnd(); });
			if (!D) { continue; }
			auto Num = [&Row](int32 C, float& Out, float Scale = 1.f) { if (Row.IsValidIndex(C) && !Row[C].TrimStartAndEnd().IsEmpty()) { Out = FCString::Atof(*Row[C]) * Scale; } };
			if (Row.IsValidIndex(CName) && !Row[CName].IsEmpty()) { D->Name = Row[CName]; }
			if (Row.IsValidIndex(CTag)) { const FString T = Row[CTag].TrimStartAndEnd(); D->Tag = T == TEXT("strike") ? EFNSkillTag::Strike : (T == TEXT("shot") ? EFNSkillTag::Shot : EFNSkillTag::Spell); }
			Num(CYar, D->YarCost); Num(CCd, D->Cooldown); Num(CDmg, D->Damage); Num(CR, D->RadiusCm, 100.f); Num(CRange, D->RangeCm, 100.f);
			Num(CArc, D->ArcDeg); Num(CDelay, D->Delay); Num(CDur, D->Duration); Num(CStun, D->Stun); Num(CIF, D->IFrames);
			float Ammo = static_cast<float>(D->AmmoCost); Num(CAmmo, Ammo); D->AmmoCost = FMath::RoundToInt(Ammo);
			if (Row.IsValidIndex(CNotes)) { D->Notes = Row[CNotes]; }
			if (Row.IsValidIndex(CVerb) && !Row[CVerb].IsEmpty()) { D->RuneVerb = Row[CVerb]; }
			++Applied;
		}
		UE_LOG(LogTemp, Log, TEXT("Skills: %d rows from %s"), Applied, *Path);
		return Path;
	}
	UE_LOG(LogTemp, Warning, TEXT("Skills: csv not found, using built-in defaults"));
	return FString();
}

const FFNSkillDef& FNSkills::Def(EFNSkillId Id)
{
	if (!bLoaded) { Reload(); }
	return Defs[FMath::Clamp(static_cast<int32>(Id), 0, Defs.Num() - 1)];
}
