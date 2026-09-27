#pragma once

#include "CoreMinimal.h"

// Skill gems of the demo (docs/systems/skills_demo_v0.1.md, director 2026-09-27). Panel 1-3.
// Numbers come from the tables docs/systems/skills/<chapter>/skills.csv, read at startup — the table IS the tuning;
// the defaults below are only a fallback when the file is missing.
enum class EFNSkillTag : uint8 { Strike, Shot, Spell }; // удар / выстрел / чары
enum class EFNSkillId : uint8 { ThunderStrike, LightningRod, Flare, ChainSpark, StormWard, Count };

struct FFNSkillDef
{
	FString Id;          // csv id
	FString Name;
	FString Short;       // slot label
	EFNSkillTag Tag = EFNSkillTag::Spell;
	float YarCost = 20.f;
	float Cooldown = 5.f;
	float Damage = 0.f;  // multiplier of the weapon (weapon_* basis) or flat damage
	float RadiusCm = 0.f;
	float RangeCm = 0.f;
	float ArcDeg = 360.f;
	float Delay = 0.f;
	float Duration = 0.f;
	float Stun = 0.f;
	int32 AmmoCost = 0;
	float IFrames = 0.f;
	FString Notes;
	FString RuneVerb;    // property for the door rune: площадь, обездвиживание, рывок, цепь, щит
};

namespace FNSkills
{
	const FFNSkillDef& Def(EFNSkillId Id);
	// (Re)reads the csv tables; called lazily on first Def(). Returns the file actually used ("" = defaults).
	FString Reload();
}
