#include "FNSkillTree.h"

namespace
{
	using K = EFNNodeKind;
}

UFNSkillTree::UFNSkillTree()
{
	PrimaryComponentTick.bCanEverTick = false;
	Allocated.Add(0); // the Spark itself
}

const TArray<FFNNode>& UFNSkillTree::Nodes()
{
	// Names in English until the UI font with Cyrillic arrives (stage 1). Russian names in comments.
	static const TArray<FFNNode> Data = {
		/* 0 */ { TEXT("Spark"), TEXT("Where every path begins."), 0.50f, 0.62f, K::Root, { 1, 8, 15, 16 } },
		// --- Thunder branch (ranged): Гром ---
		/* 1 */ { TEXT("+10% ranged damage"), TEXT("Shots hit harder."), 0.41f, 0.56f, K::Small, { 0, 2 } },
		/* 2 */ { TEXT("+25% ammo reserve"), TEXT("Carry more rounds."), 0.34f, 0.48f, K::Small, { 1, 3, 4 } },
		/* 3 */ { TEXT("Thunder Trail"), TEXT("+20% fire rate. (Громовой Шлейф)"), 0.26f, 0.40f, K::Notable, { 2, 6, 18 } },
		/* 4 */ { TEXT("+10% ranged damage"), TEXT("Shots hit harder."), 0.38f, 0.39f, K::Small, { 2, 5 } },
		/* 5 */ { TEXT("Steel Charm"), TEXT("+30% weak point damage. (Заговор на Сталь)"), 0.34f, 0.29f, K::Notable, { 4, 7 } },
		/* 6 */ { TEXT("+15% reload speed"), TEXT("Faster reloads."), 0.20f, 0.29f, K::Small, { 3, 7 } },
		/* 7 */ { TEXT("Ball Lightning"), TEXT("KEYSTONE: 30% MORE ranged damage, -20% max health. (Шаровая Молния)"), 0.27f, 0.18f, K::Keystone, { 5, 6 } },
		// --- Bone branch (melee / survival): Кость ---
		/* 8 */ { TEXT("+15 max health"), TEXT("Tougher."), 0.59f, 0.56f, K::Small, { 0, 9 } },
		/* 9 */ { TEXT("+25% melee damage"), TEXT("Heavier blows."), 0.66f, 0.48f, K::Small, { 8, 10, 11 } },
		/* 10 */ { TEXT("Blood Yar"), TEXT("Melee hits heal 6 health. (Ярь Крови)"), 0.74f, 0.40f, K::Notable, { 9, 13, 19 } },
		/* 11 */ { TEXT("+20% stamina regen"), TEXT("Recover faster."), 0.62f, 0.39f, K::Small, { 9, 12 } },
		/* 12 */ { TEXT("Tempering"), TEXT("15% less damage taken. (Закалка)"), 0.66f, 0.29f, K::Notable, { 11, 14 } },
		/* 13 */ { TEXT("+15 max health"), TEXT("Tougher."), 0.80f, 0.29f, K::Small, { 10, 14 } },
		/* 14 */ { TEXT("Bone Rampart"), TEXT("KEYSTONE: +0.15 s dodge immunity, 10% less damage taken (MORE), -15% fire rate. (Костяной Вал)"), 0.73f, 0.18f, K::Keystone, { 12, 13 } },
		// --- Path branch (mobility): Путь ---
		/* 15 */ { TEXT("-20% dodge cost"), TEXT("Dodge more often."), 0.44f, 0.72f, K::Small, { 0, 17, 20 } },
		/* 16 */ { TEXT("+8% move speed"), TEXT("Move faster."), 0.56f, 0.72f, K::Small, { 0, 17 } },
		/* 17 */ { TEXT("Light Step"), TEXT("+15% move speed, -25% dodge cost. (Лёгкая Стопа)"), 0.50f, 0.82f, K::Notable, { 15, 16, 20 } },
		// --- extra smalls ---
		/* 18 */ { TEXT("+10% fire rate"), TEXT("Faster shots."), 0.19f, 0.46f, K::Small, { 3 } },
		/* 19 */ { TEXT("+15% melee damage"), TEXT("Heavier blows."), 0.81f, 0.46f, K::Small, { 10 } },
		/* 20 */ { TEXT("+15% reload speed"), TEXT("Faster reloads."), 0.40f, 0.84f, K::Small, { 15, 17 } },
	};
	return Data;
}

const TArray<int32>& UFNSkillTree::RuneOrder()
{
	static const TArray<int32> Order = { 3, 10, 17, 5, 12, 7, 14 };
	return Order;
}

bool UFNSkillTree::IsLockedByRune(int32 Node) const
{
	const EFNNodeKind Kind = Nodes()[Node].Kind;
	return (Kind == K::Notable || Kind == K::Keystone) && !FoundRunes.Contains(Node);
}

bool UFNSkillTree::CanAllocate(int32 Node, int32 Points) const
{
	if (!Nodes().IsValidIndex(Node) || Allocated.Contains(Node) || Points <= 0 || IsLockedByRune(Node))
	{
		return false;
	}
	for (int32 L : Nodes()[Node].Links)
	{
		if (Allocated.Contains(L))
		{
			return true; // pathing: must connect to an allocated node
		}
	}
	return false;
}

bool UFNSkillTree::Allocate(int32 Node, int32 Points)
{
	if (!CanAllocate(Node, Points))
	{
		return false;
	}
	Allocated.Add(Node);
	return true;
}

FFNTreeStats UFNSkillTree::ComputeStats() const
{
	FFNTreeStats S;
	for (int32 N : Allocated)
	{
		switch (N)
		{
		case 1: case 4: S.RangedInc += 0.10f; break;
		case 2: S.ReserveInc += 0.25f; break;
		case 3: S.FireRateInc += 0.20f; break;
		case 5: S.WeakInc += 0.30f; break;
		case 6: case 20: S.ReloadInc += 0.15f; break;
		case 7: S.RangedMore *= 1.3f; S.MaxHPInc -= 0.20f; break;
		case 8: case 13: S.MaxHPFlat += 15.f; break;
		case 9: S.MeleeInc += 0.25f; break;
		case 10: S.MeleeHeal += 6.f; break;
		case 11: S.StaminaRegenInc += 0.20f; break;
		case 12: S.DamageTakenInc -= 0.15f; break;
		case 14: S.RollIFramesFlat += 0.15f; S.DamageTakenMore *= 0.9f; S.FireRateMore *= 0.85f; break;
		case 15: S.DodgeCostInc -= 0.20f; break;
		case 16: S.MoveInc += 0.08f; break;
		case 17: S.MoveInc += 0.15f; S.DodgeCostInc -= 0.25f; break;
		case 18: S.FireRateInc += 0.10f; break;
		case 19: S.MeleeInc += 0.15f; break;
		default: break;
		}
	}
	return S;
}
