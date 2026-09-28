#include "FNCharacter.h"
#include "FNHealthComponent.h"
#include "FNTreba.h"
#include "FNRite.h"
#include "FNMob.h"
#include "FNPerunBoss.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraSpriteRendererProperties.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "HighResScreenshot.h"
#include "Misc/Paths.h"

void AFNCharacter::TickInventorySmokeTest()
{
    const double Now = GetWorld()->GetRealTimeSeconds();
    if (Now < 3.0 || Now < InventoryTestNextTime) { return; }
    InventoryTestNextTime = Now + 0.7;
    auto Check = [this](bool OK, const TCHAR* Name) {
        if (!OK) { ++InventoryTestFailures; }
        UE_LOG(LogTemp, Display, TEXT("[InventoryTest] %s %s"), OK ? TEXT("PASS") : TEXT("FAIL"), Name);
    };
    FString Error;
    switch (InventoryTestStep++)
    {
    case 0:
        for (TActorIterator<AFNMob> It(GetWorld()); It; ++It) { It->SetActorTickEnabled(false); }
        for (TActorIterator<AFNPerunBoss> It(GetWorld()); It; ++It) { It->SetActorTickEnabled(false); }
        Health->bInvulnerable = true;
        GiveSkill(0); GiveSkill(1); GiveSkill(2); GiveSkill(3); GiveSkill(4);
        SetStage(EFNStage::Spark, false);
        Check(SparkFX && SparkFX->GetAsset() && SparkFX->IsActive(), TEXT("Spark asset active"));
        if (SparkFX && SparkFX->GetAsset()) {
            for (const auto& Handle : SparkFX->GetAsset()->GetEmitterHandles()) {
                if (auto* Data = Handle.GetInstance().GetEmitterData()) {
                    for (auto* Renderer : Data->GetRenderers()) {
                        if (auto* Sprite = Cast<UNiagaraSpriteRendererProperties>(Renderer)) {
                            UE_LOG(LogTemp, Display, TEXT("[SparkRenderer] %s material=%s"), *Sprite->GetPathName(), *GetPathNameSafe(Sprite->Material));
                        }
                    }
                }
            }
        }
        ToggleInventory();
        Check(!bInventoryOpen && !HasSatchel(), TEXT("Spark has no inventory"));
        GiveSatchel();
        Check(!bHasSatchel, TEXT("Spark cannot receive bag"));
        SetStage(EFNStage::Skeleton, false);
        ToggleInventory();
        Check(!bInventoryOpen && !HasSatchel(), TEXT("Skeleton without bag has no inventory"));
        for (TActorIterator<AFNRiteObject> It(GetWorld()); It; ++It) {
            bool Can = false;
            if (It->GetPrompt(this, Can).Contains(TEXT("принять пояс"))) { It->Use(this); break; }
        }
        Check(HasSatchel(), TEXT("Belt interaction gives bag"));
        ToggleInventory();
        Check(bInventoryOpen, TEXT("Open inventory after bag"));
        Check(!CanEditLoadout(), TEXT("Away from treba"));
        Check(!ApplyInventoryPanel({4, 3, 2}, Error), TEXT("Reject away from treba"));
        OnFireStarted();
        Check(!bWantsFire, TEXT("Click cannot fire in inventory"));
        ToggleMap(); ToggleTree();
        Check(!bMapOpen && !bTreeOpen, TEXT("Menus do not overlap"));
        break;
    case 1:
        CloseMenus();
        Check(!bInventoryOpen && UGameplayStatics::GetGlobalTimeDilation(this) == 1.f, TEXT("Close restores time"));
        for (TActorIterator<AFNTreba> It(GetWorld()); It; ++It) {
            SetActorLocation(It->GetActorLocation() + FVector(0, 0, 100), false, nullptr, ETeleportType::TeleportPhysics);
            LastSafeLocation = GetActorLocation(); break;
        }
        GetCharacterMovement()->StopMovementImmediately();
        ToggleInventory();
        Check(CanEditLoadout(), TEXT("Treba allows editing"));
        SkillCooldown[4] = 12.f;
        Check(ApplyInventoryPanel({4, 3, 2}, Error), TEXT("Equip owned skills"));
        Check(Panel[0] == 4 && Panel[1] == 3 && SkillCooldown[4] > 11.f, TEXT("Cooldown preserved by skill ID"));
        Check(!ApplyInventoryPanel({4, 4, 2}, Error), TEXT("Reject duplicate"));
        Check(Panel[1] == 3, TEXT("Failed transaction unchanged"));
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/Inventory/live.png"), false, false);
        break;
    case 2:
        CloseMenus();
        SetStage(EFNStage::Skeleton, false);
        Check(SparkFX && !SparkFX->IsActive(), TEXT("Skeleton disables Spark"));
        ToggleInventory();
        SetStage(EFNStage::Spark, false);
        Check(!bInventoryOpen && !HasSatchel(), TEXT("Spark transition closes and hides inventory"));
        Check(bHasSatchel, TEXT("Owned bag survives stage change"));
        Check(SparkFX && SparkFX->IsActive(), TEXT("Spark return activates FX"));
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/Inventory/spark.png"), false, false);
        break;
    case 3:
        SetStage(EFNStage::Flesh, false);
        Check(HasSatchel(), TEXT("Flesh keeps received bag"));
        ToggleInventory();
        HandleDeath(nullptr);
        Check(!bInventoryOpen && UGameplayStatics::GetGlobalTimeDilation(this) == 1.f, TEXT("Death closes menu and restores time"));
        UE_LOG(LogTemp, Display, TEXT("[InventoryTest] COMPLETE failures=%d"), InventoryTestFailures);
        FPlatformMisc::RequestExitWithStatus(false, InventoryTestFailures ? 1 : 0);
        break;
    }
}
