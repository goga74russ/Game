#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FNMob.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UFNHealthComponent;
class AFNCharacter;

// Mob types of the demo chapter (docs/level/vysi_layout_v0.1.md §2).
UENUM()
enum class EFNMobType : uint8
{
	Otrost,    // vein offshoot: crawls to movement, short melee strike. Main enemy and ammo source.
	Strelnik,  // thunder-arrow stuck in the ground: stationary, shoots a slow spark at 15 m. Teaches strafing.
	Ryhlets    // digger: burrows, surfaces under the player after a crack-ring telegraph.
};

UENUM()
enum class EFNMobState : uint8 { Idle, Chase, Windup, Recover, Burrowed, Dead };

// Grey-box mob: primitives + a simple state machine in C++ (no GAS/StateTree yet, same rule as the tech test).
// Readability (style_v0.1 §2, channel 2): dark mass darker than the ground + rot-light points (#7FE8C0) on the weak point.
UCLASS()
class FADEDNAV_API AFNMob : public ACharacter
{
	GENERATED_BODY()

public:
	AFNMob();

	// Call right after spawning, before the first Tick.
	void InitType(EFNMobType InType);

	virtual void Tick(float DeltaSeconds) override;

	EFNMobType GetMobType() const { return Type; }
	bool IsDead() const { return State == EFNMobState::Dead; }
	// Flinch (tree notable "Раскат"): a winding-up attack is interrupted.
	void Flinch() { if (State == EFNMobState::Windup && Type != EFNMobType::Ryhlets) { EnterState(EFNMobState::Recover, 0.6f); } }

	// --- Tunables [D] ---
	UPROPERTY(EditAnywhere, Category = "Mob") float AggroRadius = 2500.f;
	UPROPERTY(EditAnywhere, Category = "Mob") float LeashRadius = 4500.f;

	// Otrost
	UPROPERTY(EditAnywhere, Category = "Mob|Otrost") float OtrostHealth = 60.f;
	UPROPERTY(EditAnywhere, Category = "Mob|Otrost") float OtrostSpeed = 380.f;
	UPROPERTY(EditAnywhere, Category = "Mob|Otrost") float OtrostReach = 220.f;
	UPROPERTY(EditAnywhere, Category = "Mob|Otrost") float OtrostWindup = 0.6f;
	UPROPERTY(EditAnywhere, Category = "Mob|Otrost") float OtrostDamage = 12.f;
	UPROPERTY(EditAnywhere, Category = "Mob|Otrost") float OtrostAmmoChance = 0.6f;

	// Strelnik
	UPROPERTY(EditAnywhere, Category = "Mob|Strelnik") float StrelnikHealth = 40.f;
	UPROPERTY(EditAnywhere, Category = "Mob|Strelnik") float StrelnikRange = 1500.f;
	UPROPERTY(EditAnywhere, Category = "Mob|Strelnik") float StrelnikWindup = 0.8f;
	UPROPERTY(EditAnywhere, Category = "Mob|Strelnik") float StrelnikCooldown = 1.6f;
	UPROPERTY(EditAnywhere, Category = "Mob|Strelnik") float StrelnikDamage = 10.f;
	UPROPERTY(EditAnywhere, Category = "Mob|Strelnik") float SparkSpeed = 1400.f;

	// Ryhlets
	UPROPERTY(EditAnywhere, Category = "Mob|Ryhlets") float RyhletsHealth = 80.f;
	UPROPERTY(EditAnywhere, Category = "Mob|Ryhlets") float RyhletsBurrowTime = 1.2f;
	UPROPERTY(EditAnywhere, Category = "Mob|Ryhlets") float RyhletsTelegraph = 1.0f;
	UPROPERTY(EditAnywhere, Category = "Mob|Ryhlets") float RyhletsRadius = 250.f;
	UPROPERTY(EditAnywhere, Category = "Mob|Ryhlets") float RyhletsDamage = 20.f;
	UPROPERTY(EditAnywhere, Category = "Mob|Ryhlets") float RyhletsExposed = 2.5f;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Body;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Eye;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UFNHealthComponent> Health;

	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> BodyMID;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> EyeMID;

private:
	AFNCharacter* FindTarget() const;
	void EnterState(EFNMobState NewState, float Duration);
	void TickOtrost(AFNCharacter* Target, float DeltaSeconds);
	void TickStrelnik(AFNCharacter* Target, float DeltaSeconds);
	void TickRyhlets(AFNCharacter* Target, float DeltaSeconds);
	void SetBurrowed(bool bBurrowed);
	void SetEyeGlow(float Strength);
	void SpawnTelegraph(const FVector& Location, float Radius, float Delay, float Damage, const FLinearColor& Color);
	void FireSpark(const AFNCharacter* Target);
	void DropAmmo();

	UFUNCTION() void HandleDeath(AActor* Killer);

	EFNMobType Type = EFNMobType::Otrost;
	EFNMobState State = EFNMobState::Idle;
	float StateTime = 0.f;
	float StateDuration = 0.f;
	FVector HomeLocation = FVector::ZeroVector;
	FVector BurrowTarget = FVector::ZeroVector;
};
