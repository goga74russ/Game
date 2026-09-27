#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FNRite.generated.h"

class AFNCharacter;
class UStaticMeshComponent;

// "Rite" engine (docs/systems/secrets_v0.1.md): a chain is a set of world objects, each answering one verb.
// A step = verb on an object (+ an item from the Satchel) under at most one extra condition (hero stage / oak thunder).
// Hidden chains, the tutorial quest and super-quests all run on this; they differ only in presentation.
UENUM()
enum class EFNVerb : uint8 { Take, Milk, Pour, Place, Talk };

UENUM()
enum class EFNRiteReward : uint8 { None, HorseHelmet, StrelokopCache };

// Stage bits for FFNRiteStep::Stages.
namespace FNStage { constexpr uint8 Spark = 1, Skeleton = 2, Flesh = 4, Any = 7; }

// One NPC line; the last matching line wins (later = more specific).
struct FFNRiteLine
{
	FName NeedFlag;   // world flag required (None = always)
	FName NeedItem;   // Satchel item required (None = any)
	FString Text;
	FName SetFlag;    // flag set when the line is spoken
};

struct FFNRiteStep
{
	FString Name;                 // shown in the prompt: "Коза", "Конский череп"
	FString Action;               // verb label: "подоить", "полить молоком"
	EFNVerb Verb = EFNVerb::Take;
	uint8 Stages = FNStage::Any;
	FString RefuseSpark, RefuseSkeleton, RefuseFlesh; // why this stage cannot do it
	FName NeedItem;               // must be in the Satchel
	bool bConsumeItem = true;
	FName GiveItem;               // put into the Satchel
	FName NeedFlag;               // world flag required to be usable
	FName SetFlag;                // world flag set on success
	bool bThunder = false;        // only right after the oak is struck ("свой час")
	FString WaitText;             // shown when the thunder window is not open
	FString DoneText;
	FString ShotText;             // reaction when shot/hit (near-miss feedback)
	bool bOneShot = true;
	EFNRiteReward Reward = EFNRiteReward::None;
	FString Speaker;              // Talk: who speaks
	TArray<FFNRiteLine> Lines;    // Talk
};

UENUM()
enum class EFNRiteLook : uint8 { Goat, SkullPole, SkyArrow, Beam, Elder };

UCLASS()
class FADEDNAV_API AFNRiteObject : public AActor
{
	GENERATED_BODY()

public:
	AFNRiteObject();

	void Init(const FFNRiteStep& InStep, EFNRiteLook InLook);

	// What the prompt says for this hero right now; bCan = pressing E will work.
	FString GetPrompt(const AFNCharacter* Hero, bool& bCan) const;
	void Use(AFNCharacter* Hero);
	void OnShot(AFNCharacter* Hero);
	bool IsSpent() const { return bSpent; }

	static FString ItemName(FName Item);
	static double LastThunderTime; // set by the grey-box oak strike
	static bool IsThunderWindow(const UWorld* World);

private:
	bool CanStage(const AFNCharacter* Hero, FString* OutRefusal) const;
	void GiveReward(AFNCharacter* Hero);
	UStaticMeshComponent* Part(const TCHAR* Shape, const FVector& Offset, const FVector& Scale, const FLinearColor& Color);

	UPROPERTY() TObjectPtr<USceneComponent> Root;
	FFNRiteStep Step;
	EFNRiteLook Look = EFNRiteLook::Goat;
	bool bSpent = false;
};
