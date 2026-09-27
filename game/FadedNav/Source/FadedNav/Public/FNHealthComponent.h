#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FNHealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFNOnDeath, AActor*, Killer);

// Tech-test health. Will be replaced by GAS attributes in stage 1 (see docs/tech/architecture_v1.md).
UCLASS(ClassGroup = (FadedNav), meta = (BlueprintSpawnableComponent))
class FADEDNAV_API UFNHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFNHealthComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health")
	float MaxHealth = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	float Health = 100.f;

	UPROPERTY(BlueprintAssignable)
	FFNOnDeath OnDeath;

	bool bInvulnerable = false;

	// Called when an attack that would have hit was avoided by invulnerability (perfect-dodge hook).
	TFunction<void()> OnAvoided;

	// Temporary shield (skill "Оберег грозы"): absorbs damage first.
	float Shield = 0.f;
	TFunction<void()> OnShieldBroken;

	// Damage taken multiplier from passives (Increased/More, GDD §6).
	float IncomingMultiplier = 1.f;

	// Returns damage actually applied.
	float ApplyDamage(float Amount, AActor* Instigator);

	bool IsDead() const { return Health <= 0.f; }
	float GetRatio() const { return MaxHealth > 0.f ? Health / MaxHealth : 0.f; }

protected:
	virtual void BeginPlay() override;
};
