#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FNPerunBoss.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UFNHealthComponent;
class AFNCharacter;
class AFNTelegraph;

UENUM()
enum class EFNBossState : uint8 { Idle, Intro, Chase, Windup, Recover, Stagger, Transition, Stopped, Beaten, Epilogue };

UENUM()
enum class EFNBossAttack : uint8 { Slam, Bolts, Judgment };

// Perun the mentor: the Yav exam (GDD §9, bosses.md §2, foundation_perun §6).
//  Phases by HP (100-66 / 66-33 / 33-0): attacks get faster, Judgment from phase 2, more bolts in phase 3.
//  Phase gates: short invulnerability, a line, and summoned Otrosts that drop ammo (GDD §5).
//  Parry: during Slam/Judgment wind-ups a glowing point appears; shooting it breaks the attack and staggers him.
//  Outcome A: the player "falls" in the arena -> he stops the fight (line depends on how far the player got).
//  Outcome B: his health reaches 0 -> he cracks ("Годно..."), the player gets a cosmetic trace, no power.
//  Both outcomes lead to the ridge epilogue at the portal.
UCLASS()
class FADEDNAV_API AFNPerunBoss : public ACharacter
{
	GENERATED_BODY()

public:
	AFNPerunBoss();

	virtual void Tick(float DeltaSeconds) override;

	UFNHealthComponent* GetHealth() const { return Health; }
	bool IsDead() const { return State == EFNBossState::Beaten; }
	bool IsFightActive() const;
	int32 GetPhase() const { return Phase; }

	// Called by the hero.
	void OnPlayerFell(AFNCharacter* Player);
	void TryParry();

	// Subtitles and end card for the HUD.
	const FString& GetSubtitleSpeaker() const { return SubSpeaker; }
	const FString& GetSubtitle() const { return SubText; }
	bool HasSubtitle() const;
	bool ShowEndCard() const { return bEndCard; }

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
	UPROPERTY(EditAnywhere, Category = "Perun") float StaggerTime = 1.8f;
	UPROPERTY(EditAnywhere, Category = "Perun") float ActivationRadius = 2200.f;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Body;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Head;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> ParryPoint;
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
	AFNTelegraph* SpawnTelegraph(const FVector& Location, float Radius, float Delay, float Damage, bool bDeadly, const FLinearColor& Color);
	void CancelAttack();
	void SpawnAmmo(int32 Count);
	void SpawnAdds(int32 Count);
	void SetTint(const FLinearColor& Color);
	void SetPose(bool bRaised);
	void SetOverlay(const FLinearColor* Color);
	void SetParryWindow(bool bOpen);
	void PlayMontage(class UAnimMontage* Montage, float Duration);
	void Say(const FString& Speaker, const FString& Text, float Duration);
	void EnterState(EFNBossState NewState, float Duration);
	void BeginEpilogue();
	float CooldownFor() const;
	bool bHasSkeletalVisual = false;

	UFUNCTION() void HandleDeath(AActor* Killer);

	EFNBossState State = EFNBossState::Idle;
	EFNBossAttack CurrentAttack = EFNBossAttack::Slam;
	float StateTime = 0.f;
	float StateDuration = 0.f;
	float Cooldown = 2.f;
	int32 AttackCount = 0;
	int32 Phase = 1;
	bool bDamagedInFinalPhase = false;
	float LastHealth = 0.f;
	float ImpulseRemaining = 0.f;
	bool bParryWindow = false;
	TArray<FTimerHandle> BoltTimers;
	TArray<TWeakObjectPtr<AFNTelegraph>> ActiveTelegraphs;
	FVector HomeLocation = FVector::ZeroVector;
	TWeakObjectPtr<AFNCharacter> FallenPlayer;

	// Epilogue dialogue at the portal.
	int32 EpilogueStep = 0;
	float EpilogueClock = 0.f;
	bool bEndCard = false;

	FString SubSpeaker;
	FString SubText;
	double SubUntil = -1.0;
};
