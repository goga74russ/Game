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
	// Russian names: the Canvas font renders Cyrillic.
	static const TArray<FFNNode> Data = {
		/* 0 */ { TEXT("Искра"), TEXT("С неё начинается любой путь."), 0.50f, 0.62f, K::Root, { 1, 8, 15, 16 } },
		// --- Thunder branch (ranged): Гром ---
		/* 1 */ { TEXT("+10% урона выстрелов"), TEXT("Выстрелы бьют сильнее."), 0.41f, 0.56f, K::Small, { 0, 2 } },
		/* 2 */ { TEXT("+25% запаса патронов"), TEXT("Больше патронов с собой."), 0.34f, 0.48f, K::Small, { 1, 3, 4 } },
		/* 3 */ { TEXT("Громовой Шлейф"), TEXT("+20% к скорострельности."), 0.26f, 0.40f, K::Notable, { 2, 6, 18 } },
		/* 4 */ { TEXT("+10% урона выстрелов"), TEXT("Выстрелы бьют сильнее."), 0.38f, 0.39f, K::Small, { 2, 5 } },
		/* 5 */ { TEXT("Заговор на Сталь"), TEXT("+30% урона по слабым местам."), 0.34f, 0.29f, K::Notable, { 4, 7 } },
		/* 6 */ { TEXT("+15% скорости перезарядки"), TEXT("Перезарядка быстрее."), 0.20f, 0.29f, K::Small, { 3, 7 } },
		/* 7 */ { TEXT("Шаровая Молния"), TEXT("КЛЮЧЕВОЙ: урон выстрелов в 1,3 раза БОЛЬШЕ, но −20% здоровья."), 0.27f, 0.18f, K::Keystone, { 5, 6 } },
		// --- Bone branch (melee / survival): Кость ---
		/* 8 */ { TEXT("+15 к здоровью"), TEXT("Крепче."), 0.59f, 0.56f, K::Small, { 0, 9 } },
		/* 9 */ { TEXT("+25% урона удара"), TEXT("Удары тяжелее."), 0.66f, 0.48f, K::Small, { 8, 10, 11 } },
		/* 10 */ { TEXT("Ярь Крови"), TEXT("Каждый удар вблизи лечит на 6."), 0.74f, 0.40f, K::Notable, { 9, 13, 19 } },
		/* 11 */ { TEXT("+20% восстановления выносливости"), TEXT("Быстрее переводишь дух."), 0.62f, 0.39f, K::Small, { 9, 12 } },
		/* 12 */ { TEXT("Закалка"), TEXT("Получаешь на 15% меньше урона."), 0.66f, 0.29f, K::Notable, { 11, 14 } },
		/* 13 */ { TEXT("+15 к здоровью"), TEXT("Крепче."), 0.80f, 0.29f, K::Small, { 10, 14 } },
		/* 14 */ { TEXT("Костяной Вал"), TEXT("КЛЮЧЕВОЙ: +0,15 с неуязвимости при уклонении, в 1,1 раза МЕНЬШЕ урона, −15% скорострельности."), 0.73f, 0.18f, K::Keystone, { 12, 13 } },
		// --- Path branch (mobility): Путь ---
		/* 15 */ { TEXT("−20% цены уклонения"), TEXT("Уклоняешься чаще."), 0.44f, 0.72f, K::Small, { 0, 17, 20 } },
		/* 16 */ { TEXT("+8% скорости"), TEXT("Быстрее ходишь."), 0.56f, 0.72f, K::Small, { 0, 17 } },
		/* 17 */ { TEXT("Лёгкая Стопа"), TEXT("+15% скорости, уклонение на 25% дешевле."), 0.50f, 0.82f, K::Notable, { 15, 16, 20 } },
		// --- extra smalls ---
		/* 18 */ { TEXT("+10% скорострельности"), TEXT("Стреляешь чаще."), 0.19f, 0.46f, K::Small, { 3 } },
		/* 19 */ { TEXT("+15% урона удара"), TEXT("Удары тяжелее."), 0.81f, 0.46f, K::Small, { 10 } },
		/* 20 */ { TEXT("+15% скорости перезарядки"), TEXT("Перезарядка быстрее."), 0.40f, 0.84f, K::Small, { 15, 17 } },
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
