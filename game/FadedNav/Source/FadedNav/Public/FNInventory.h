#pragma once

#include "CoreMinimal.h"

class AFNCharacter;
class UCanvas;
class UFont;
class UTexture2D;

// A read-only view of existing gameplay state, not a second item store.
struct FADEDNAV_API FFNInventorySnapshot
{
	TArray<int32> OwnedSkills;
	TArray<int32> Panel = { -1, -1, -1 };
	TArray<FString> Equipment;
	TArray<FString> Satchel;
	bool bCanEdit = false;
	bool bSpark = false;
	static FFNInventorySnapshot Capture(const AFNCharacter& Hero, bool bAtTreba);
};

namespace FNInventory
{
	// Pure transaction. Failure leaves OutPanel unchanged. Re-equipping an
	// already slotted gem swaps slots instead of duplicating it.
	FADEDNAV_API bool PlanAssignment(const FFNInventorySnapshot& State, int32 Skill, int32 Slot,
		TArray<int32>& OutPanel, FString& OutError);
}

// Canvas matches the existing HUD; no new UMG/Slate module dependency.
// Host supplies fonts/icons (kept alive by UPROPERTY in the HUD), draws while
// open, and routes clicks. Host alone owns pause/input and actual panel writes.
class FADEDNAV_API FFNInventoryScreen
{
public:
	void Draw(UCanvas* Canvas, const FFNInventorySnapshot& State, UFont* TitleFont, UFont* TextFont,
		const TMap<int32, UTexture2D*>& Icons);
	// Apply receives the complete proposed panel. It MUST revalidate current
	// ownership/rest eligibility/cooldowns before mutating gameplay.
	bool Click(const FVector2D& ScreenPoint, const FFNInventorySnapshot& State,
		TFunctionRef<bool(const TArray<int32>&, FString&)> Apply);
	void Reset() { SelectedSkill = -1; Notice.Reset(); }
	int32 GetSelectedSkill() const { return SelectedSkill; }
	const FString& GetNotice() const { return Notice; }
private:
	struct FHit { FBox2D Rect; int32 Skill = -1; int32 Slot = -1; };
	TArray<FHit> Hits;
	int32 SelectedSkill = -1;
	FString Notice;
};
