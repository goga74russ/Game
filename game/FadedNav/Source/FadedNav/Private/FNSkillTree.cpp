#include "FNSkillTree.h"

namespace
{
	using K = EFNNodeKind;
	using St = EFNStat;

	// GDD §3 ring order, Perun on top, clockwise.
	const TCHAR* Gods[] = { TEXT("Перун"), TEXT("Велес"), TEXT("Мокошь"), TEXT("Ярило"), TEXT("Чернобог"), TEXT("Дажьбог"), TEXT("Стрибог"), TEXT("Сварог") };
	const TCHAR* Elements[] = { TEXT("Гром"), TEXT("Ртуть"), TEXT("Нити"), TEXT("Ярь-кровь"), TEXT("Порча"), TEXT("Солнце"), TEXT("Ветер"), TEXT("Жар") };
	const TCHAR* GodColors[] = { TEXT("6fdcff"), TEXT("a9c2cf"), TEXT("d7a6e0"), TEXT("d0503a"), TEXT("86c45a"), TEXT("ffd36a"), TEXT("cfe8e0"), TEXT("ff8a4a") };
	// Icon per closed sector (HUD line art): drop, spindle, flame, skull, sun, wind, flame.
	const int32 GodIcons[] = { 1, 5, 8, 9, 10, 11, 12, 9 };

	struct FSmall { const TCHAR* Name; St Stat; float Value; };
	// Perun's small nodes: milder copies of the demo effects, cycled along the branch.
	const FSmall PerunSmall[] = {
		{ TEXT("+8% урона выстрелов"), St::Ranged, 0.08f },
		{ TEXT("+15% запаса патронов"), St::Reserve, 0.15f },
		{ TEXT("+8% скорости перезарядки"), St::Reload, 0.08f },
		{ TEXT("+6% скорострельности"), St::FireRate, 0.06f },
		{ TEXT("+10 к здоровью"), St::MaxHPFlat, 10.f },
		{ TEXT("+12% урона удара"), St::Melee, 0.12f },
		{ TEXT("+12% восстановления выносливости"), St::StaminaRegen, 0.12f },
		{ TEXT("+5% скорости"), St::Move, 0.05f },
		{ TEXT("−10% цены уклонения"), St::DodgeCost, -0.10f },
	};

	struct FNotable { const TCHAR* Name; const TCHAR* Desc; St Stat; float Value; int32 Icon; };
	// The demo notables (director-approved names), one per cluster; "Раскат" added with tree mockup v1.
	const FNotable PerunNotable[] = {
		{ TEXT("Громовой Шлейф"), TEXT("+20% к скорострельности."), St::FireRate, 0.20f, 1 },
		{ TEXT("Заговор на Сталь"), TEXT("+30% урона по слабым местам."), St::Weak, 0.30f, 2 },
		{ TEXT("Закалка"), TEXT("Получаешь на 15% меньше урона."), St::DamageTaken, -0.15f, 3 },
		{ TEXT("Лёгкая Стопа"), TEXT("+15% скорости, уклонение на 25% дешевле."), St::Move, 0.15f, 4 },
		{ TEXT("Ярь Крови"), TEXT("Каждый удар вблизи лечит на 6."), St::MeleeHeal, 6.f, 5 },
		{ TEXT("Раскат"), TEXT("Выстрел в уязвимое место заставляет тварь вздрогнуть."), St::WeakFlinch, 1.f, 6 },
	};

	TArray<FFNNode> Build()
	{
		TArray<FFNNode> N;
		auto Add = [&N](FFNNode Node) { N.Add(MoveTemp(Node)); return N.Num() - 1; };
		auto Link = [&N](int32 A, int32 B) { N[A].Links.Add(B); N[B].Links.Add(A); };
		auto P = [](float R, float A) { return FVector2D(R * FMath::Cos(A), R * FMath::Sin(A)); };

		FFNNode Root;
		Root.Name = TEXT("Искра"); Root.Desc = TEXT("Сердце древа. С неё начинается любой путь.");
		Root.Kind = K::Root; Root.God = 0; Root.Pos = FVector2D::ZeroVector;
		const int32 RootIdx = Add(Root);

		TArray<int32> Gates;
		for (int32 G = 0; G < 8; ++G)
		{
			const bool bOpen = UFNSkillTree::IsSectorOpen(G);
			const float A0 = -PI * 0.5f + G * PI * 0.25f;
			int32 SmallI = 0, NotableI = 0;
			auto MakeSmall = [&](FVector2D At)
			{
				FFNNode S; S.Pos = At; S.Kind = K::Small; S.God = G;
				if (bOpen)
				{
					const FSmall& D = PerunSmall[(SmallI++) % UE_ARRAY_COUNT(PerunSmall)];
					S.Name = D.Name; S.Desc = TEXT("Малый узел."); S.Stat = D.Stat; S.Value = D.Value;
				}
				else { S.Name = TEXT("???"); S.Desc = TEXT("Скрыто туманом."); }
				return Add(S);
			};

			FFNNode Gate; Gate.Pos = P(130.f, A0); Gate.Kind = K::Notable; Gate.God = G; Gate.Icon = GodIcons[G];
			Gate.Name = bOpen ? TEXT("Врата Грома") : FString::Printf(TEXT("Врата: %s"), Gods[G]);
			Gate.Desc = bOpen ? TEXT("Вход в ветвь Перуна: +5% урона выстрелов.") : TEXT("Скрыто туманом.");
			if (bOpen) { Gate.Stat = St::Ranged; Gate.Value = 0.05f; }
			const int32 GateIdx = Add(Gate);
			Gates.Add(GateIdx);
			Link(RootIdx, GateIdx);

			// The limb: five small nodes outward (created right after the gate, so Gate+k = limb node k).
			TArray<int32> Limb = { GateIdx };
			for (int32 k = 1; k <= 5; ++k)
			{
				const int32 Id = MakeSmall(P(130.f + k * 100.f, A0 + FMath::Sin(k * 1.7f + G) * 0.05f));
				Link(Limb.Last(), Id);
				Limb.Add(Id);
			}

			// Clusters off the limb, each ending in a notable.
			static const int32 Clusters[6][2] = { { 1, -1 }, { 2, 1 }, { 3, -1 }, { 4, 1 }, { 5, -1 }, { 5, 1 } };
			for (int32 C = 0; C < 6; ++C)
			{
				const FVector2D Base = N[Limb[Clusters[C][0]]].Pos;
				const float Side = Clusters[C][1];
				const float BR = Base.Size();
				const float Ang = FMath::Atan2(Base.Y, Base.X) + Side * (0.13f + 0.05f * C / 6.f);
				int32 Prev = Limb[Clusters[C][0]];
				const int32 Len = 3 + (C % 2);
				for (int32 k = 1; k <= Len; ++k)
				{
					const int32 Id = MakeSmall(P(BR + k * 38.f, Ang + Side * k * 0.055f));
					Link(Prev, Id);
					Prev = Id;
				}
				FFNNode Nt; Nt.Pos = P(BR + Len * 38.f + 44.f, Ang + Side * (Len * 0.055f + 0.05f)); Nt.Kind = K::Notable; Nt.God = G;
				if (bOpen)
				{
					const FNotable& D = PerunNotable[(NotableI++) % UE_ARRAY_COUNT(PerunNotable)];
					Nt.Name = D.Name; Nt.Desc = D.Desc; Nt.Stat = D.Stat; Nt.Value = D.Value; Nt.Icon = D.Icon; Nt.bSealed = true;
				}
				else { Nt.Name = TEXT("???"); Nt.Desc = TEXT("Скрыто туманом."); Nt.Icon = GodIcons[G]; }
				Link(Prev, Add(Nt));
			}

			// Two keystones at the tip.
			for (int32 Side = -1; Side <= 1; Side += 2)
			{
				const float A = A0 + Side * 0.07f;
				const int32 Pre = MakeSmall(P(730.f, A));
				Link(Limb[5], Pre);
				FFNNode Ks; Ks.Pos = P(830.f, A + Side * 0.02f); Ks.Kind = K::Keystone; Ks.God = G;
				if (bOpen && Side < 0)
				{
					Ks.Name = TEXT("Шаровая Молния"); Ks.Desc = TEXT("КЛЮЧЕВОЙ: урон выстрелов в 1,3 раза БОЛЬШЕ, но −20% здоровья."); Ks.Special = 1; Ks.Icon = 1; Ks.bSealed = true;
				}
				else if (bOpen)
				{
					Ks.Name = TEXT("Костяной Вал"); Ks.Desc = TEXT("КЛЮЧЕВОЙ: +0,15 с неуязвимости при уклонении, в 1,1 раза МЕНЬШЕ урона, −15% скорострельности."); Ks.Special = 2; Ks.Icon = 7; Ks.bSealed = true;
				}
				else { Ks.Name = TEXT("???"); Ks.Desc = TEXT("Скрыто туманом."); Ks.Icon = GodIcons[G]; }
				Link(Pre, Add(Ks));
			}
		}

		// Boundary nodes between neighbouring sectors (PoE-style wheel). Closed in the demo.
		for (int32 G = 0; G < 8; ++G)
		{
			FFNNode B; B.Pos = P(470.f, -PI * 0.5f + G * PI * 0.25f + PI / 8.f); B.Kind = K::Small; B.God = -1;
			B.Name = TEXT("Межа"); B.Desc = TEXT("Стык двух ветвей. Откроется, когда вернутся оба бога.");
			const int32 Id = Add(B);
			Link(Id, Gates[G] + 3);
			Link(Id, Gates[(G + 1) % 8] + 3);
		}
		return N;
	}
}

UFNSkillTree::UFNSkillTree()
{
	PrimaryComponentTick.bCanEverTick = false;
	Allocated.Add(0); // the Spark itself
}

const TArray<FFNNode>& UFNSkillTree::Nodes()
{
	static const TArray<FFNNode> Data = Build();
	return Data;
}

const TArray<int32>& UFNSkillTree::RuneOrder()
{
	// Same drop order as the demo tree, found by name; "Раскат" last.
	static const TArray<int32> Order = []
	{
		static const TCHAR* Names[] = { TEXT("Громовой Шлейф"), TEXT("Ярь Крови"), TEXT("Лёгкая Стопа"), TEXT("Заговор на Сталь"), TEXT("Закалка"), TEXT("Шаровая Молния"), TEXT("Костяной Вал"), TEXT("Раскат") };
		TArray<int32> O;
		for (const TCHAR* Name : Names)
		{
			O.Add(Nodes().IndexOfByPredicate([Name](const FFNNode& N) { return N.bSealed && N.Name == Name; }));
		}
		return O;
	}();
	return Order;
}

const TCHAR* UFNSkillTree::GodName(int32 God) { return God >= 0 && God < 8 ? Gods[God] : TEXT("межа"); }
const TCHAR* UFNSkillTree::GodElement(int32 God) { return God >= 0 && God < 8 ? Elements[God] : TEXT(""); }
FLinearColor UFNSkillTree::GodColor(int32 God) { return FLinearColor(FColor::FromHex(GodColors[God >= 0 && God < 8 ? God : 0])); }

bool UFNSkillTree::IsLockedByRune(int32 Node) const
{
	return Nodes().IsValidIndex(Node) && Nodes()[Node].bSealed && !FoundRunes.Contains(Node);
}

bool UFNSkillTree::IsPassable(int32 Node) const
{
	return Nodes().IsValidIndex(Node) && IsSectorOpen(Nodes()[Node].God) && !IsLockedByRune(Node);
}

bool UFNSkillTree::FindPath(int32 Node, TArray<int32>& OutPath) const
{
	OutPath.Reset();
	if (Allocated.Contains(Node)) { return true; }
	if (!IsPassable(Node)) { return false; }
	const TArray<FFNNode>& N = Nodes();
	TMap<int32, int32> Prev;
	TArray<int32> Queue;
	for (int32 A : Allocated) { Prev.Add(A, INDEX_NONE); Queue.Add(A); }
	for (int32 Head = 0; Head < Queue.Num(); ++Head)
	{
		for (int32 L : N[Queue[Head]].Links)
		{
			if (Prev.Contains(L) || !IsPassable(L)) { continue; }
			Prev.Add(L, Queue[Head]);
			if (L == Node)
			{
				for (int32 X = Node; !Allocated.Contains(X); X = Prev[X]) { OutPath.Insert(X, 0); }
				return true;
			}
			Queue.Add(L);
		}
	}
	return false;
}

bool UFNSkillTree::CanAllocate(int32 Node, int32 Points) const
{
	TArray<int32> Path;
	return FindPath(Node, Path) && Path.Num() > 0 && Path.Num() <= Points;
}

bool UFNSkillTree::Allocate(int32 Node, int32 Points)
{
	TArray<int32> Path;
	if (!FindPath(Node, Path) || Path.Num() == 0 || Path.Num() > Points) { return false; }
	Allocated.Append(Path);
	return true;
}

int32 UFNSkillTree::Refund(int32 Node)
{
	if (Node == 0 || !Allocated.Contains(Node)) { return 0; }
	Allocated.Remove(Node);
	// Keep only what still connects to the Spark.
	TSet<int32> Keep = { 0 };
	TArray<int32> Queue = { 0 };
	for (int32 Head = 0; Head < Queue.Num(); ++Head)
	{
		for (int32 L : Nodes()[Queue[Head]].Links)
		{
			if (Allocated.Contains(L) && !Keep.Contains(L)) { Keep.Add(L); Queue.Add(L); }
		}
	}
	const int32 Back = 1 + Allocated.Num() - Keep.Num();
	Allocated = Keep;
	return Back;
}

FFNTreeStats UFNSkillTree::ComputeStats() const
{
	FFNTreeStats S;
	for (int32 Idx : Allocated)
	{
		const FFNNode& N = Nodes()[Idx];
		switch (N.Stat)
		{
		case St::Ranged: S.RangedInc += N.Value; break;
		case St::Reserve: S.ReserveInc += N.Value; break;
		case St::FireRate: S.FireRateInc += N.Value; break;
		case St::Weak: S.WeakInc += N.Value; break;
		case St::Reload: S.ReloadInc += N.Value; break;
		case St::MaxHPFlat: S.MaxHPFlat += N.Value; break;
		case St::Melee: S.MeleeInc += N.Value; break;
		case St::MeleeHeal: S.MeleeHeal += N.Value; break;
		case St::StaminaRegen: S.StaminaRegenInc += N.Value; break;
		case St::DamageTaken: S.DamageTakenInc += N.Value; break;
		case St::DodgeCost: S.DodgeCostInc += N.Value; break;
		case St::Move: S.MoveInc += N.Value; if (N.Name == TEXT("Лёгкая Стопа")) { S.DodgeCostInc -= 0.25f; } break;
		case St::WeakFlinch: S.bWeakFlinch = true; break;
		default: break;
		}
		if (N.Special == 1) { S.RangedMore *= 1.3f; S.MaxHPInc -= 0.20f; }
		if (N.Special == 2) { S.RollIFramesFlat += 0.15f; S.DamageTakenMore *= 0.9f; S.FireRateMore *= 0.85f; }
	}
	return S;
}
