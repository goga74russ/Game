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

// Evolution stages of the hero in the Yav chapter (GDD §4): kills drive Spark -> Skeleton -> Flesh.
UENUM()
enum class EFNStage : uint8 { Spark, Skeleton, Flesh };

UENUM()
enum class EFNWeapon : uint8 { Plasma, Rifle, Scatter };

// Hero: Spark (plasma, blink) -> Skeleton (dash) -> Flesh (roll, full kit): over-the-shoulder camera, hitscan rifle, roll with i-frames, melee.
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

	EFNStage GetStage() const { return Stage; }
	EFNWeapon GetWeapon() const { return Weapon; }
	int32 GetScatterAmmo() const { return ScatterAmmo; }
	bool HasWeapon(EFNWeapon W) const { return W == EFNWeapon::Plasma || (W == EFNWeapon::Rifle && bHasRifle) || (W == EFNWeapon::Scatter && bHasScatter); }
	FLinearColor GetSparkColor() const { return SparkColor; }
	float GetArmor() const { return ArmorBonus; }
	// Ability slots 1-4 (GDD §4: Yav — 3 skill-gem slots; slot 4 = ultimate, opens in Nav). Empty in the demo for now.
	bool IsAbilitySlotOpen(int32 Slot) const { return Slot < 3; }
	int32 GetKills() const;
	void GiveWeapon(EFNWeapon NewWeapon);
	void GiveArmor(float Bonus);
	void RestAtTreba(const FVector& At);    // heal, refill, set respawn point
	bool IsExamDefeat() const { return bDead && bDiedInArena; }
	float GetRespawnRemaining() const { return RespawnTimer; }
	const FString& GetMessage() const { return Message; }
	float GetMessageAge() const;
	void ShowMessage(const FString& Text);

	// Passive tree (GDD §6).
	class UFNSkillTree* GetTree() const { return Tree; }
	bool IsTreeOpen() const { return bTreeOpen; }
	int32 GetSkillPoints() const;
	void TryAllocate(int32 Node);
	void ToggleTree();

	// Exam hooks.
	void ReviveAt(const FVector& At);
	void GiveTrace();              // cosmetic trace for the rare exam win (no power)
	void Flinch() { FlinchRemaining = 0.45f; } // startles at thunder
	void FindRune(int32 Node);

	// Satchel for rite items (secrets_v0.1: 6 slots [D], no weight, kept on death).
	bool HasItem(FName Item) const { return Satchel.Contains(Item); }
	bool IsSatchelFull() const { return Satchel.Num() >= SatchelSize; }
	void AddItem(FName Item) { if (!IsSatchelFull()) { Satchel.Add(Item); } }
	void RemoveItem(FName Item) { Satchel.RemoveSingle(Item); }
	const TArray<FName>& GetSatchel() const { return Satchel; }
	static constexpr int32 SatchelSize = 6;

	// NPC speech shown as a subtitle (speaker + line).
	void ShowSubtitle(const FString& Speaker, const FString& Text);
	bool HasSubtitle() const;
	const FString& GetSubSpeaker() const { return SubSpeaker; }
	const FString& GetSubText() const { return SubText; }

	// Rite reward: horse-skull helmet (cosmetic).
	void GiveHelmet();
	bool HasHelmet() const { return bHelmet; }

	// Interaction (E): nearest rite object in front of the hero.
	class AFNRiteObject* GetFocus() const { return Focus; }
	FString GetFocusPrompt(bool& bCan) const;
	void TryRefund(int32 Node);

	// Kills needed for each evolution step (GDD §4: ~5 per stage [D]).
	UPROPERTY(EditAnywhere, Category = "Evolution") int32 KillsToSkeleton = 5;
	UPROPERTY(EditAnywhere, Category = "Evolution") int32 KillsToFlesh = 10;

	// Colour of the Spark = element of the starting god/biome (Vysi / Perun: thunder blue). GDD §4.
	UPROPERTY(EditAnywhere, Category = "Evolution") FLinearColor SparkColor = FLinearColor(0.15f, 0.75f, 1.f);

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
	UPROPERTY(VisibleAnywhere) TObjectPtr<class UFNSkillTree> Tree;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> SparkOrb;
	UPROPERTY(VisibleAnywhere) TObjectPtr<class UPointLightComponent> SparkLight;
	// Skeleton stage body (Fab "Free Pack - Human Skeleton", Mixamo-rigged). Copies the hidden Wraith pose each frame.
	UPROPERTY(VisibleAnywhere) TObjectPtr<class UPoseableMeshComponent> SkeletonMesh;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> HelmetOnFlesh;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> HelmetOnBones;
	UPROPERTY() TObjectPtr<class UInputAction> InteractAction;
	UPROPERTY() TObjectPtr<class AFNRiteObject> Focus;

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
	UPROPERTY() TObjectPtr<UInputAction> Weapon1Action;
	UPROPERTY() TObjectPtr<UInputAction> Weapon2Action;
	UPROPERTY() TObjectPtr<UInputAction> Weapon3Action;
	UPROPERTY() TObjectPtr<UInputAction> AbilityActions[4];
	UPROPERTY() TObjectPtr<UInputAction> TreeAction;

private:
	void OnInteract();
	void UpdateFocus();
	void UpdateHelmet();
	TArray<FName> Satchel;
	FString SubSpeaker, SubText;
	double SubTime = -100.0;
	bool bHelmet = false;
	// Per-bone retarget Wraith (Epic names) -> Mixamo skeleton, in world space, aligning rest-pose bone directions.
	struct FRetargetBone { FName Src, Dst; int32 SrcIdx = INDEX_NONE, DstIdx = INDEX_NONE; FQuat Align = FQuat::Identity; };
	TArray<FRetargetBone> RetargetBones;
	void InitRetarget();
	void UpdateRetarget();

	void BuildInput();

	void OnMove(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);
	// LMB: melee by default; ranged only while aiming with RMB (director, 2026-09-26).
	void OnFireStarted() { if (bAiming) { bWantsFire = true; } else { OnMelee(); } }
	void OnFireStopped() { bWantsFire = false; }
	void OnAimStarted() { bAiming = true; }
	void OnAimStopped() { bAiming = false; bWantsFire = false; }
	void OnRoll();
	void OnReload();
	void OnMelee();
	void OnRestart();

	void FireShot();
	void FireTrace(float Damage, float SpreadDeg, float Range, const FColor& Tracer);
	void SelectWeapon(EFNWeapon W);
	void OnWeapon1() { CycleWeapon(+1); }
	void OnWeapon2() { CycleWeapon(-1); }
	void OnWeapon3() { CycleWeapon(+1); }
	void CycleWeapon(int32 Dir);
	void OnAbility(int32 Slot);
	void SetStage(EFNStage NewStage, bool bAnnounce);
	void Revive();
	void ApplyStats();
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
	FVector LastSafeLocation = FVector::ZeroVector;
	float SafeTimer = 0.f;

	EFNStage Stage = EFNStage::Spark;
	EFNWeapon Weapon = EFNWeapon::Plasma;
	bool bHasRifle = false;
	bool bHasScatter = false;
	int32 ScatterAmmo = 6;
	float ArmorBonus = 0.f;
	float CurRollSpeed = 1500.f;
	float CurRollDuration = 0.5f;
	float CurRollIFrames = 0.35f;
	FVector Checkpoint = FVector::ZeroVector;
	bool bDiedInArena = false;
	float RespawnTimer = -1.f;
	FString Message;
	double MessageTime = -100.0;

	bool bTreeOpen = false;
	bool bHasTrace = false;
	float FlinchRemaining = 0.f;
	float MeleeCooldown = 0.f;
	float MeleeFlash = 0.f;
	float StageBaseHealth = 100.f;
	float StageBaseSpeed = 500.f;
	struct FFNTreeCache { float Ranged = 1.f, FireRate = 1.f, Weak = 0.f, Reload = 0.f, Reserve = 0.f, Melee = 0.f, MeleeHeal = 0.f, Stamina = 0.f, Dodge = 0.f, IFrames = 0.f; bool bWeakFlinch = false; } TreeMods;
};
