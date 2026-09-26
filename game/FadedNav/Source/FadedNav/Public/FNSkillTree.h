#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FNSkillTree.generated.h"

UENUM()
enum class EFNNodeKind : uint8 { Root, Small, Notable, Keystone };

// Derived modifiers from allocated nodes. Increased = additive, More = multiplicative (GDD §6).
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

	float Ranged() const { return (1.f + RangedInc) * RangedMore; }
	float FireRate() const { return (1.f + FireRateInc) * FireRateMore; }
	float DamageTaken() const { return FMath::Max(0.1f, 1.f + DamageTakenInc) * DamageTakenMore; }
};

struct FFNNode
{
	const TCHAR* Name;
	const TCHAR* Desc;
	float X, Y;               // screen layout, 0..1
	EFNNodeKind Kind;
	TArray<int32> Links;
};

// Demo piece of the passive tree (slice_v1: 20-30 nodes, two clearly different branches before the exam).
// Small nodes are open; notables and keystones stay locked until their rune-key is found (GDD §6: rune = key).
// Points: 1 per 2 kills [D]. Node data lives here for the grey-box; moves to a DataTable in stage 1.
UCLASS(ClassGroup = (FadedNav))
class FADEDNAV_API UFNSkillTree : public UActorComponent
{
	GENERATED_BODY()

public:
	UFNSkillTree();

	static const TArray<FFNNode>& Nodes();
	static const TArray<int32>& RuneOrder(); // notables/keystones in drop order

	bool IsAllocated(int32 Node) const { return Allocated.Contains(Node); }
	bool IsRuneFound(int32 Node) const { return FoundRunes.Contains(Node); }
	bool IsLockedByRune(int32 Node) const;
	bool CanAllocate(int32 Node, int32 Points) const;
	int32 GetSpent() const { return Allocated.Num() - 1; } // root is free

	bool Allocate(int32 Node, int32 Points);
	void FindRune(int32 Node) { FoundRunes.Add(Node); }
	int32 NumRunesFound() const { return FoundRunes.Num(); }

	FFNTreeStats ComputeStats() const;

private:
	TSet<int32> Allocated;
	TSet<int32> FoundRunes;
};
