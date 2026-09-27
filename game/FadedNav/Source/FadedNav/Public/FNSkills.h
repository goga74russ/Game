#pragma once

#include "CoreMinimal.h"

class AFNCharacter;

// Skill gems of the demo (docs/systems/skills_demo_v0.1.md, director 2026-09-27). Panel 1-3; numbers [D].
enum class EFNSkillTag : uint8 { Strike, Shot, Spell }; // удар / выстрел / чары
enum class EFNSkillId : uint8 { ThunderStrike, LightningRod, Flare, ChainSpark, StormWard, Count };

struct FFNSkillDef
{
	const TCHAR* Name;
	const TCHAR* Short;   // slot label
	EFNSkillTag Tag;
	float YarCost;
	float Cooldown;
	const TCHAR* Desc;
	const TCHAR* RuneVerb; // property for the door rune: площадь, обездвиживание, рывок, цепь, щит
};

namespace FNSkills
{
	const FFNSkillDef& Def(EFNSkillId Id);
}
