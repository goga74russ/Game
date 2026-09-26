#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "FNCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UStaticMeshComponent;
class UInputAction;
class UInputMappingContext;
class UFNHealthComponent;

// Tech-test hero ("Flesh" stage): over-the-shoulder camera, hitscan rifle, roll with i-frames, melee.
UCLASS()
class FADEDNAV_API AFNCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AFNCharacter();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// Returns false if reserve is full.
	bool AddReserveAmmo(int32 Amount);

	UFNHealthComponent* GetHealth() const { return Health; }
	int32 GetAmmo() const { return Ammo; }
	int32 GetReserve() const { return Reserve; }
	bool IsReloading() const { return bReloading; }
	float GetStaminaRatio() const { return Stamina / MaxStamina; }
	bool IsDead() const { return bDead; }
	bool IsAiming() const { return bAiming; }
	float GetTimeSinceHit() const;
	bool WasLastHitWeak() const { return bLastHitWeak; }

	// --- Tunables [D] = placeholder until playtest ---
	UPROPERTY(EditAnywhere, Category = "Weapon") float ShotDamage = 25.f;
	UPROPERTY(EditAnywhere, Category = "Weapon") float WeakPointMultiplier = 1.6f;
	UPROPERTY(EditAnywhere, Category = "Weapon") float FireInterval = 0.12f;
	UPROPERTY(EditAnywhere, Category = "Weapon") float HipSpreadDeg = 2.0f;
	UPROPERTY(EditAnywhere, Category = "Weapon") float AimSpreadDeg = 0.4f;
	UPROPERTY(EditAnywhere, Category = "Weapon") int32 MagazineSize = 24;
	UPROPERTY(EditAnywhere, Category = "Weapon") int32 MaxReserve = 120;
	UPROPERTY(EditAnywhere, Category = "Weapon") float ReloadTime = 1.6f;

	UPROPERTY(EditAnywhere, Category = "Stamina") float MaxStamina = 100.f;
	UPROPERTY(EditAnywhere, Category = "Stamina") float StaminaRegen = 35.f;
	UPROPERTY(EditAnywhere, Category = "Stamina") float RollCost = 25.f;
	UPROPERTY(EditAnywhere, Category = "Stamina") float MeleeCost = 20.f;

	UPROPERTY(EditAnywhere, Category = "Roll") float RollSpeed = 1500.f;
	UPROPERTY(EditAnywhere, Category = "Roll") float RollIFrames = 0.35f;
	UPROPERTY(EditAnywhere, Category = "Roll") float RollDuration = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Melee") float MeleeDamage = 40.f;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere) TObjectPtr<USpringArmComponent> Boom;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Body;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Head;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Gun;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UFNHealthComponent> Health;

	UPROPERTY() TObjectPtr<class UAnimMontage> FireMontage;
	UPROPERTY() TObjectPtr<class UAnimSequence> DeathAnim;

	// Input assets are built in code so the tech test needs no editor setup.
	UPROPERTY() TObjectPtr<UInputMappingContext> Mapping;
	UPROPERTY() TObjectPtr<UInputAction> MoveAction;
	UPROPERTY() TObjectPtr<UInputAction> LookAction;
	UPROPERTY() TObjectPtr<UInputAction> FireAction;
	UPROPERTY() TObjectPtr<UInputAction> AimAction;
	UPROPERTY() TObjectPtr<UInputAction> RollAction;
	UPROPERTY() TObjectPtr<UInputAction> ReloadAction;
	UPROPERTY() TObjectPtr<UInputAction> MeleeAction;
	UPROPERTY() TObjectPtr<UInputAction> RestartAction;

private:
	void BuildInput();

	void OnMove(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);
	void OnFireStarted() { bWantsFire = true; }
	void OnFireStopped() { bWantsFire = false; }
	void OnAimStarted() { bAiming = true; }
	void OnAimStopped() { bAiming = false; }
	void OnRoll();
	void OnReload();
	void OnMelee();
	void OnRestart();

	void FireShot();
	void FinishReload();

	UFUNCTION() void HandleDeath(AActor* Killer);

	int32 Ammo = 24;
	int32 Reserve = 72;
	bool bReloading = false;
	float ReloadRemaining = 0.f;
	bool bWantsFire = false;
	float FireCooldown = 0.f;
	bool bAiming = false;

	float Stamina = 100.f;
	float StaminaDelay = 0.f;

	bool bRolling = false;
	float RollRemaining = 0.f;
	float IFramesRemaining = 0.f;
	FVector RollDirection = FVector::ForwardVector;
	FVector LastMoveInput = FVector::ZeroVector;

	bool bDead = false;
	double LastHitTime = -100.0;
	bool bLastHitWeak = false;

	float DefaultWalkSpeed = 500.f;
};
