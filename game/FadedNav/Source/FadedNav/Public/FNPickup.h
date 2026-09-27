#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FNPickup.generated.h"

class UStaticMeshComponent;

UENUM()
enum class EFNPickupType : uint8 { Rifle, Scatter, Armor, Rune, Skill };

// Chapter loot on the ground (slice_v1: weapon 1 at the Oath Stone, weapon 2 + armour in the shed).
// Readability channel 3: wax gold with a vertical column (style_v0.1 §2).
UCLASS()
class FADEDNAV_API AFNPickup : public AActor
{
	GENERATED_BODY()

public:
	AFNPickup();

	void InitType(EFNPickupType InType) { Type = InType; }
	void InitRune(int32 Node) { Type = EFNPickupType::Rune; RuneNode = Node; }
	void InitSkill(int32 Skill) { Type = EFNPickupType::Skill; RuneNode = Skill; }
	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Item;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Column;

private:
	EFNPickupType Type = EFNPickupType::Rifle;
	int32 RuneNode = -1;
	FVector BaseLocation;
	float Age = 0.f;
};
