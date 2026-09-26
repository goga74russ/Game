#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FNPerunBoss.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UFNHealthComponent;
class AFNCharacter;

UENUM()
enum class EFNBossState : uint8 { Idle, Chase, Windup, Recover, Dead };

UENUM()
enum class EFNBossAttack : uint8 { Slam, Bolts, Judgment };

// Tech-test Perun (mentor exam). Three attacks with telegraphs:
//  Slam      — close ring, normal signal.
//  Bolts     — three strikes at the player's position, normal signal.
//  Judgment  — wide deadly strike: pose + one silhouette impulse (red/white), GDD §9.
UCLASS()
class FADEDNAV_API AFNPerunBoss : public ACharacter
{
	GENERATED_BODY()

public:
	AFNPerunBoss();

	virtual void Tick(float DeltaSeconds) override;

	UFNHealthComponent* GetHealth() const { return Health; }
	bool IsDead() const { return State == EFNBossState::Dead; }

	// --- Tunables [D] ---
	UPROPERTY(EditAnywhere, Category = "Perun") float SlamRadius = 450.f;
	UPROPERTY(EditAnywhere, Category = "Perun") float SlamDelay = 0.9f;
	UPROPERTY(EditAnywhere, Category = "Perun") float SlamDamage = 30.f;
	UPROPERTY(EditAnywhere, Category = "Perun") float BoltRadius = 250.f;
	UPROPERTY(EditAnywhere, Category = "Perun") float BoltDelay = 1.0f;
	UPROPERTY(EditAnywhere, Category = "Perun") float BoltDamage = 25.f;
	UPROPERTY(EditAnywhere, Category = "Perun") float JudgmentRadius = 1000.f;
	UPROPERTY(EditAnywhere, Category = "Perun") float JudgmentDelay = 1.8f;
	UPROPERTY(EditAnywhere, Category = "Perun") float JudgmentDamage = 80.f;
	UPROPERTY(EditAnywhere, Category = "Perun") float RecoverTime = 0.9f;
	UPROPERTY(EditAnywhere, Category = "Perun") float MinCooldown = 1.0f;
	UPROPERTY(EditAnywhere, Category = "Perun") float MaxCooldown = 2.0f;
	UPROPERTY(EditAnywhere, Category = "Perun") float ActivationRadius = 2200.f;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Body;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Head;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UFNHealthComponent> Health;

	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> BodyMID;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> HeadMID;
	UPROPERTY() TObjectPtr<class UAnimMontage> SlamMontage;
	UPROPERTY() TObjectPtr<class UAnimMontage> BoltsMontage;
	UPROPERTY() TObjectPtr<class UAnimMontage> JudgmentMontage;
	UPROPERTY() TObjectPtr<class UAnimSequence> DeathAnim;

private:
	AFNCharacter* FindTarget() const;
	void StartAttack(AFNCharacter* Target);
	void SpawnTelegraph(const FVector& Location, float Radius, float Delay, float Damage, bool bDeadly, const FLinearColor& Color);
	void SpawnAmmo(int32 Count);
	void SetTint(const FLinearColor& Color);
	void SetPose(bool bRaised);
	void SetOverlay(const FLinearColor* Color);
	void PlayMontage(class UAnimMontage* Montage, float Duration);
	bool bHasSkeletalVisual = false;

	UFUNCTION() void HandleDeath(AActor* Killer);

	EFNBossState State = EFNBossState::Idle;
	EFNBossAttack CurrentAttack = EFNBossAttack::Slam;
	float StateTime = 0.f;
	float StateDuration = 0.f;
	float Cooldown = 2.f;
	int32 AttackCount = 0;
	float ImpulseRemaining = 0.f;
	TArray<FTimerHandle> BoltTimers;
	FVector HomeLocation = FVector::ZeroVector;
};
