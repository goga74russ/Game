#include "FNSkillTree.h"
#include "FNCsv.h"

namespace
{
	using K = EFNNodeKind;
	using St = EFNStat;

	// GDD §3 ring order, Perun on top, clockwise.
	const TCHAR* Gods[] = { TEXT("Перун"), TEXT("Велес"), TEXT("Мокошь"), TEXT("Ярило"), TEXT("Чернобог"), TEXT("Дажьбог"), TEXT("Стрибог"), TEXT("Сварог") };
	const TCHAR* Elements[] = { TEXT("Гром"), TEXT("Ртуть"), TEXT("Нити"), TEXT("Ярь-кровь"), TEXT("Порча"), TEXT("Солнце"), TEXT("Ветер"), TEXT("Жар") };
	const TCHAR* GodColors[] = { TEXT("8fa8ff"), TEXT("a9c2cf"), TEXT("d7a6e0"), TEXT("d0503a"), TEXT("86c45a"), TEXT("ffd36a"), TEXT("cfe8e0"), TEXT("ff8a4a") };
	// Icon per closed sector (HUD line art): drop, spindle, flame, skull, sun, wind, flame.
	const int32 GodIcons[] = { 1, 5, 8, 9, 10, 11, 12, 9 };

	// Content of a sector's nodes (names, effects, seals). The layout (branches, clusters) stays in Build().
	struct FContent
	{
		FString Name, Desc;
		TArray<TPair<St, float>> Effects;
		int32 Icon = 0;
		bool bSealed = false;
		int32 RuneOrder = -1;
	};
	struct FSectorContent { FContent Gate; TArray<FContent> Small, Notable, Keystone; FString Path; };

	St StatFromName(const FString& N)
	{
		static const TMap<FString, St> Map = {
			{ TEXT("ranged"), St::Ranged }, { TEXT("reserve"), St::Reserve }, { TEXT("fire_rate"), St::FireRate }, { TEXT("weak"), St::Weak },
			{ TEXT("reload"), St::Reload }, { TEXT("max_hp_flat"), St::MaxHPFlat }, { TEXT("melee"), St::Melee }, { TEXT("melee_heal"), St::MeleeHeal },
			{ TEXT("stamina_regen"), St::StaminaRegen }, { TEXT("damage_taken"), St::DamageTaken }, { TEXT("dodge_cost"), St::DodgeCost },
			{ TEXT("move"), St::Move }, { TEXT("weak_flinch"), St::WeakFlinch }, { TEXT("ranged_more"), St::RangedMore }, { TEXT("max_hp_inc"), St::MaxHPInc },
			{ TEXT("iframes"), St::IFrames }, { TEXT("damage_taken_more"), St::DamageTakenMore }, { TEXT("fire_rate_more"), St::FireRateMore },
		};
		const St* Found = Map.Find(N.ToLower());
		return Found ? *Found : St::None;
	}

	// Built-in fallback = the approved demo content (used only if the table is missing).
	FSectorContent PerunDefaults()
	{
		auto C = [](const TCHAR* Name, const TCHAR* Desc, std::initializer_list<TPair<St, float>> Fx, int32 Icon = 0, bool bSeal = false, int32 Rune = -1)
		{
			FContent X; X.Name = Name; X.Desc = Desc; X.Effects = Fx; X.Icon = Icon; X.bSealed = bSeal; X.RuneOrder = Rune; return X;
		};
		FSectorContent P;
		P.Gate = C(TEXT("Врата Грома"), TEXT("Вход в ветвь Перуна: +5% урона выстрелов."), { { St::Ranged, 0.05f } }, 1);
		P.Small = {
			C(TEXT("+8% урона выстрелов"), TEXT("Малый узел."), { { St::Ranged, 0.08f } }), C(TEXT("+15% запаса патронов"), TEXT("Малый узел."), { { St::Reserve, 0.15f } }),
			C(TEXT("+8% скорости перезарядки"), TEXT("Малый узел."), { { St::Reload, 0.08f } }), C(TEXT("+6% скорострельности"), TEXT("Малый узел."), { { St::FireRate, 0.06f } }),
			C(TEXT("+10 к здоровью"), TEXT("Малый узел."), { { St::MaxHPFlat, 10.f } }), C(TEXT("+12% урона удара"), TEXT("Малый узел."), { { St::Melee, 0.12f } }),
			C(TEXT("+12% восстановления выносливости"), TEXT("Малый узел."), { { St::StaminaRegen, 0.12f } }), C(TEXT("+5% скорости"), TEXT("Малый узел."), { { St::Move, 0.05f } }),
			C(TEXT("−10% цены уклонения"), TEXT("Малый узел."), { { St::DodgeCost, -0.10f } }),
		};
		P.Notable = {
			C(TEXT("Громовой Шлейф"), TEXT("+20% к скорострельности."), { { St::FireRate, 0.20f } }, 1, true, 0),
			C(TEXT("Заговор на Сталь"), TEXT("+30% урона по слабым местам."), { { St::Weak, 0.30f } }, 2, true, 3),
			C(TEXT("Закалка"), TEXT("Получаешь на 15% меньше урона."), { { St::DamageTaken, -0.15f } }, 3, true, 4),
			C(TEXT("Лёгкая Стопа"), TEXT("+15% скорости, уклонение на 25% дешевле."), { { St::Move, 0.15f }, { St::DodgeCost, -0.25f } }, 4, true, 2),
			C(TEXT("Ярь Крови"), TEXT("Каждый удар вблизи лечит на 6."), { { St::MeleeHeal, 6.f } }, 5, true, 1),
			C(TEXT("Раскат"), TEXT("Выстрел в уязвимое место заставляет тварь вздрогнуть."), { { St::WeakFlinch, 1.f } }, 6, true, 7),
		};
		P.Keystone = {
			C(TEXT("Шаровая Молния"), TEXT("КЛЮЧЕВОЙ: урон выстрелов в 1,3 раза БОЛЬШЕ, но −20% здоровья."), { { St::RangedMore, 1.3f }, { St::MaxHPInc, -0.2f } }, 1, true, 5),
			C(TEXT("Костяной Вал"), TEXT("КЛЮЧЕВОЙ: +0,15 с неуязвимости при уклонении, в 1,1 раза МЕНЬШЕ урона, −15% скорострельности."),
				{ { St::IFrames, 0.15f }, { St::DamageTakenMore, 0.9f }, { St::FireRateMore, 0.85f } }, 7, true, 6),
		};
		return P;
	}

	const FSectorContent& Perun()
	{
		static const FSectorContent Content = []
		{
			FSectorContent P = PerunDefaults();
			FFNCsv T;
			if (!T.Load(TEXT("tree/01-grom.csv"))) { UE_LOG(LogTemp, Warning, TEXT("Tree: 01-grom.csv not found, built-in defaults")); return P; }
			const int32 CKind = T.Col(TEXT("kind")), CName = T.Col(TEXT("name_ru")), CDesc = T.Col(TEXT("desc")), CIcon = T.Col(TEXT("icon")),
				CSeal = T.Col(TEXT("sealed")), CRune = T.Col(TEXT("rune_order"));
			const int32 CS[3] = { T.Col(TEXT("stat1")), T.Col(TEXT("stat2")), T.Col(TEXT("stat3")) };
			const int32 CV[3] = { T.Col(TEXT("value1")), T.Col(TEXT("value2")), T.Col(TEXT("value3")) };
			FSectorContent F;
			bool bGate = false;
			for (const TArray<FString>& R : T.Rows)
			{
				FContent X;
				X.Name = T.Str(R, CName); X.Desc = T.Str(R, CDesc); X.Icon = FMath::RoundToInt(T.Num(R, CIcon));
				X.bSealed = T.Num(R, CSeal) > 0.5f; X.RuneOrder = T.Has(R, CRune) ? FMath::RoundToInt(T.Num(R, CRune)) : -1;
				for (int32 k = 0; k < 3; ++k) { if (T.Has(R, CS[k])) { X.Effects.Add({ StatFromName(T.Str(R, CS[k])), T.Num(R, CV[k]) }); } }
				const FString Kind = T.Str(R, CKind).ToLower();
				if (Kind == TEXT("gate")) { F.Gate = X; bGate = true; }
				else if (Kind == TEXT("small")) { F.Small.Add(X); }
				else if (Kind == TEXT("notable")) { F.Notable.Add(X); }
				else if (Kind == TEXT("keystone")) { F.Keystone.Add(X); }
			}
			if (!bGate || F.Small.Num() == 0 || F.Notable.Num() == 0 || F.Keystone.Num() < 2) { UE_LOG(LogTemp, Warning, TEXT("Tree: %s incomplete, defaults"), *T.Path); return P; }
			F.Path = T.Path;
			UE_LOG(LogTemp, Log, TEXT("Tree: %d small, %d notable, %d keystone from %s"), F.Small.Num(), F.Notable.Num(), F.Keystone.Num(), *T.Path);
			return F;
		}();
		return Content;
	}

	void Apply(FFNNode& N, const FContent& C)
	{
		N.Name = C.Name; N.Desc = C.Desc; N.Effects = C.Effects; N.bSealed = C.bSealed;
		if (C.Icon > 0) { N.Icon = C.Icon; }
	}

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
				if (bOpen) { Apply(S, Perun().Small[(SmallI++) % Perun().Small.Num()]); }
				else { S.Name = TEXT("???"); S.Desc = TEXT("Скрыто туманом."); }
				return Add(S);
			};

			FFNNode Gate; Gate.Pos = P(130.f, A0); Gate.Kind = K::Notable; Gate.God = G; Gate.Icon = GodIcons[G];
			if (bOpen) { Apply(Gate, Perun().Gate); }
			else { Gate.Name = FString::Printf(TEXT("Врата: %s"), Gods[G]); Gate.Desc = TEXT("Скрыто туманом."); }
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
				if (bOpen) { Apply(Nt, Perun().Notable[(NotableI++) % Perun().Notable.Num()]); }
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
				if (bOpen) { Apply(Ks, Perun().Keystone[Side < 0 ? 0 : 1]); }
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
	// Drop order of the rune-keys = column rune_order of the sector table.
	static const TArray<int32> Order = []
	{
		TArray<TPair<int32, int32>> Seq; // (order, node)
		const TArray<FFNNode>& N = Nodes();
		auto Find = [&N](const FString& Name) { return N.IndexOfByPredicate([&Name](const FFNNode& X) { return X.bSealed && X.God == 0 && X.Name == Name; }); };
		for (const FContent& C : Perun().Notable) { if (C.bSealed && C.RuneOrder >= 0) { Seq.Add({ C.RuneOrder, Find(C.Name) }); } }
		for (const FContent& C : Perun().Keystone) { if (C.bSealed && C.RuneOrder >= 0) { Seq.Add({ C.RuneOrder, Find(C.Name) }); } }
		Seq.Sort([](const TPair<int32, int32>& A, const TPair<int32, int32>& B) { return A.Key < B.Key; });
		TArray<int32> O;
		for (const TPair<int32, int32>& P : Seq) { if (P.Value != INDEX_NONE) { O.AddUnique(P.Value); } }
		return O;
	}();
	return Order;
}

FString UFNSkillTree::TablePath() { return Perun().Path; }

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
		for (const TPair<St, float>& E : Nodes()[Idx].Effects)
		{
			const float V = E.Value;
			switch (E.Key)
			{
			case St::Ranged: S.RangedInc += V; break;
			case St::Reserve: S.ReserveInc += V; break;
			case St::FireRate: S.FireRateInc += V; break;
			case St::Weak: S.WeakInc += V; break;
			case St::Reload: S.ReloadInc += V; break;
			case St::MaxHPFlat: S.MaxHPFlat += V; break;
			case St::Melee: S.MeleeInc += V; break;
			case St::MeleeHeal: S.MeleeHeal += V; break;
			case St::StaminaRegen: S.StaminaRegenInc += V; break;
			case St::DamageTaken: S.DamageTakenInc += V; break;
			case St::DodgeCost: S.DodgeCostInc += V; break;
			case St::Move: S.MoveInc += V; break;
			case St::WeakFlinch: S.bWeakFlinch = V > 0.f; break;
			case St::RangedMore: S.RangedMore *= V; break;
			case St::MaxHPInc: S.MaxHPInc += V; break;
			case St::IFrames: S.RollIFramesFlat += V; break;
			case St::DamageTakenMore: S.DamageTakenMore *= V; break;
			case St::FireRateMore: S.FireRateMore *= V; break;
			default: break;
			}
		}
	}
	return S;
}
