#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FNSkillTree.generated.h"

UENUM()
enum class EFNNodeKind : uint8 { Root, Small, Notable, Keystone };

// What a node changes (GDD §6: Increased = additive, More = multiplicative; keystones use Special).
// Table names (docs/systems/tree/*.csv, column stat1..3): ranged, reserve, fire_rate, weak, reload, max_hp_flat, melee, melee_heal,
// stamina_regen, damage_taken, dodge_cost, move, weak_flinch, ranged_more, max_hp_inc, iframes, damage_taken_more, fire_rate_more.
enum class EFNStat : uint8 { None, Ranged, Reserve, FireRate, Weak, Reload, MaxHPFlat, Melee, MeleeHeal, StaminaRegen, DamageTaken, DodgeCost, Move, WeakFlinch,
	RangedMore, MaxHPInc, IFrames, DamageTakenMore, FireRateMore };

// Derived modifiers from allocated nodes.
struct FFNTreeStats
{
	float RangedInc = 0.f, RangedMore = 1.f;
	float FireRateInc = 0.f, FireRateMore = 1.f;
	float ReserveInc = 0.f;
	float WeakInc = 0.f;
	float ReloadInc = 0.f;
	float MaxHPFlat = 0.f, MaxHPInc = 0.f;
	float MeleeInc = 0.f;
	float MeleeHeal = 0.f;
	float StaminaRegenInc = 0.f;
	float DamageTakenInc = 0.f, DamageTakenMore = 1.f;
	float DodgeCostInc = 0.f;
	float MoveInc = 0.f;
	float RollIFramesFlat = 0.f;
	bool bWeakFlinch = false;

	float Ranged() const { return (1.f + RangedInc) * RangedMore; }
	float FireRate() const { return (1.f + FireRateInc) * FireRateMore; }
	float DamageTaken() const { return FMath::Max(0.1f, 1.f + DamageTakenInc) * DamageTakenMore; }
};

struct FFNNode
{
	FString Name;
	FString Desc;
	FVector2D Pos;            // tree space: Spark at 0,0, y down, Perun's branch points up, radius ~1000
	EFNNodeKind Kind = EFNNodeKind::Small;
	int32 God = -1;           // sector 0..7 in the GDD ring order (0 = Perun), -1 = boundary node between sectors
	bool bSealed = false;     // needs its rune-key (GDD §6: rune = key)
	TArray<TPair<EFNStat, float>> Effects; // up to 3, from the sector table
	int32 Icon = 0;           // line-art icon id for the HUD
	TArray<int32> Links;
};

// The passive tree (GDD §6): 8 god sectors around the Spark, grown like an oak, in fog until the god returns.
// Demo: only Perun's sector is open. Pathing like PoE: a node connects to the learned set; a whole path can be bought.
// Points: 1 per 2 kills [D]. Node data is generated here for now; moves to a DataTable in stage 1.
UCLASS(ClassGroup = (FadedNav))
class FADEDNAV_API UFNSkillTree : public UActorComponent
{
	GENERATED_BODY()

public:
	UFNSkillTree();

	static const TArray<FFNNode>& Nodes();
	static const TArray<int32>& RuneOrder(); // sealed nodes in drop order (column rune_order)
	static FString TablePath();              // which csv filled the open sector ("" = built-in defaults)
	static const TCHAR* GodName(int32 God);
	static const TCHAR* GodElement(int32 God);
	static FLinearColor GodColor(int32 God);
	static bool IsSectorOpen(int32 God) { return God == 0; } // demo: Perun only

	bool IsAllocated(int32 Node) const { return Allocated.Contains(Node); }
	bool IsRuneFound(int32 Node) const { return FoundRunes.Contains(Node); }
	bool IsLockedByRune(int32 Node) const;
	bool IsPassable(int32 Node) const; // open sector and not sealed
	bool CanAllocate(int32 Node, int32 Points) const;
	int32 GetSpent() const { return Allocated.Num() - 1; } // root is free

	// Shortest path of unlearned passable nodes from the learned set to Node (empty if learned, false if unreachable).
	bool FindPath(int32 Node, TArray<int32>& OutPath) const;
	bool Allocate(int32 Node, int32 Points);      // buys the whole path if affordable
	int32 Refund(int32 Node);                     // returns points given back (node + anything cut off)
	void FindRune(int32 Node) { FoundRunes.Add(Node); }
	int32 NumRunesFound() const { return FoundRunes.Num(); }

	FFNTreeStats ComputeStats() const;

private:
	TSet<int32> Allocated;
	TSet<int32> FoundRunes;
};
