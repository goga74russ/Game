#include "FNRite.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "FNCharacter.h"
#include "FNGameMode.h"
#include "FNPickup.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

double AFNRiteObject::LastThunderTime = -100.0;

AFNRiteObject::AFNRiteObject()
{
	PrimaryActorTick.bCanEverTick = false;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
}

FString AFNRiteObject::ItemName(FName Item)
{
	if (Item == TEXT("Milk")) { return TEXT("Крынка молока"); }
	if (Item == TEXT("SkyArrow")) { return TEXT("Стрела с неба"); }
	return Item.ToString();
}

bool AFNRiteObject::IsThunderWindow(const UWorld* World)
{
	// "Свой час": a few seconds after the oak is struck (every 12 s) [D].
	return World && World->GetTimeSeconds() - LastThunderTime < 3.5;
}

UStaticMeshComponent* AFNRiteObject::Part(const TCHAR* Shape, const FVector& Offset, const FVector& Scale, const FLinearColor& Color)
{
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
	C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"), Shape, Shape)));
	C->SetupAttachment(Root);
	C->SetRelativeLocation(Offset);
	C->SetRelativeScale3D(Scale);
	C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	C->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	C->RegisterComponent();
	if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
	{
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, this);
		MID->SetVectorParameterValue(TEXT("Color"), Color);
		C->SetMaterial(0, MID);
	}
	return C;
}

void AFNRiteObject::Init(const FFNRiteStep& InStep, EFNRiteLook InLook)
{
	Step = InStep;
	Look = InLook;
	// Grey-box looks (primitives are 100 cm; scale 1 = 1 m). Palette obeys the colour channels: no gold, no silver here.
	switch (Look)
	{
	case EFNRiteLook::Goat:
		Part(TEXT("Cube"), FVector(0, 0, 55), FVector(0.9f, 0.4f, 0.45f), FLinearColor(0.35f, 0.3f, 0.24f));        // body
		Part(TEXT("Cube"), FVector(55, 0, 85), FVector(0.3f, 0.22f, 0.25f), FLinearColor(0.3f, 0.26f, 0.2f));       // head
		for (const FVector& L : { FVector(30, 15, 15), FVector(30, -15, 15), FVector(-30, 15, 15), FVector(-30, -15, 15) })
		{
			Part(TEXT("Cylinder"), L, FVector(0.08f, 0.08f, 0.35f), FLinearColor(0.2f, 0.17f, 0.13f));
		}
		break;
	case EFNRiteLook::SkullPole:
		Part(TEXT("Cylinder"), FVector(0, 0, 90), FVector(0.1f, 0.1f, 1.8f), FLinearColor(0.25f, 0.2f, 0.15f));      // pole
		Part(TEXT("Cube"), FVector(12, 0, 190), FVector(0.55f, 0.2f, 0.25f), FLinearColor(0.82f, 0.79f, 0.7f));     // horse skull
		Part(TEXT("Sphere"), FVector(0, 0, 165), FVector(0.1f, 0.1f, 0.1f), FLinearColor(0.45f, 0.28f, 0.16f));     // copper bell
		break;
	case EFNRiteLook::SkyArrow:
		Part(TEXT("Cylinder"), FVector(0, 0, 40), FVector(0.06f, 0.06f, 0.8f), FLinearColor(0.12f, 0.12f, 0.13f));  // arrow
		Part(TEXT("Sphere"), FVector(0, 0, 82), FVector(0.1f, 0.1f, 0.12f), FLinearColor(0.6f, 0.22f, 0.08f));      // fused tip
		break;
	case EFNRiteLook::Beam:
		Part(TEXT("Cube"), FVector(0, 0, 250), FVector(0.4f, 5.2f, 0.3f), FLinearColor(0.03f, 0.03f, 0.03f));       // blackened beam
		break;
	case EFNRiteLook::Elder:
		Part(TEXT("Cylinder"), FVector(0, 0, 60), FVector(0.45f, 0.45f, 1.2f), FLinearColor(0.78f, 0.76f, 0.7f));    // faded body
		Part(TEXT("Sphere"), FVector(0, 0, 135), FVector(0.3f, 0.3f, 0.32f), FLinearColor(0.8f, 0.78f, 0.72f));     // head
		break;
	}
}

bool AFNRiteObject::CanStage(const AFNCharacter* Hero, FString* OutRefusal) const
{
	const EFNStage S = Hero->GetStage();
	const uint8 Bit = S == EFNStage::Spark ? FNStage::Spark : (S == EFNStage::Skeleton ? FNStage::Skeleton : FNStage::Flesh);
	if (Step.Stages & Bit) { return true; }
	if (OutRefusal)
	{
		*OutRefusal = S == EFNStage::Spark ? Step.RefuseSpark : (S == EFNStage::Skeleton ? Step.RefuseSkeleton : Step.RefuseFlesh);
	}
	return false;
}

FString AFNRiteObject::GetPrompt(const AFNCharacter* Hero, bool& bCan) const
{
	bCan = false;
	if (bSpent) { return FString(); }
	const AFNGameMode* GM = GetWorld()->GetAuthGameMode<AFNGameMode>();
	if (Step.Verb == EFNVerb::Talk) { bCan = true; return FString::Printf(TEXT("%s — %s"), *Step.Name, *Step.Action); }
	if (!Step.NeedFlag.IsNone() && (!GM || !GM->HasFlag(Step.NeedFlag))) { return Step.Name; }
	FString Refusal;
	if (!CanStage(Hero, &Refusal)) { return FString::Printf(TEXT("%s  ·  %s"), *Step.Name, *Refusal); }
	if (!Step.NeedItem.IsNone() && !Hero->HasItem(Step.NeedItem)) { return Step.Name; } // no hint about what it wants
	if (!Step.GiveItem.IsNone() && Hero->HasItem(Step.GiveItem)) { return FString::Printf(TEXT("%s  ·  уже есть"), *Step.Name); }
	if (!Step.GiveItem.IsNone() && Hero->IsSatchelFull()) { return FString::Printf(TEXT("%s  ·  сума полна"), *Step.Name); }
	if (Step.bThunder && !IsThunderWindow(GetWorld())) { return FString::Printf(TEXT("%s  ·  %s"), *Step.Name, *Step.WaitText); }
	bCan = true;
	return FString::Printf(TEXT("%s — %s"), *Step.Name, *Step.Action);
}

void AFNRiteObject::Use(AFNCharacter* Hero)
{
	bool bCan = false;
	GetPrompt(Hero, bCan);
	if (!bCan) { return; }
	AFNGameMode* GM = GetWorld()->GetAuthGameMode<AFNGameMode>();

	if (Step.Verb == EFNVerb::Talk)
	{
		const FFNRiteLine* Pick = nullptr;
		for (const FFNRiteLine& L : Step.Lines)
		{
			if ((!L.NeedFlag.IsNone() && (!GM || !GM->HasFlag(L.NeedFlag))) || (!L.NeedItem.IsNone() && !Hero->HasItem(L.NeedItem))) { continue; }
			Pick = &L;
		}
		if (Pick)
		{
			Hero->ShowSubtitle(Step.Speaker, Pick->Text);
			if (GM && !Pick->SetFlag.IsNone()) { GM->SetFlag(Pick->SetFlag); }
		}
		return;
	}

	if (!Step.NeedItem.IsNone() && Step.bConsumeItem) { Hero->RemoveItem(Step.NeedItem); }
	if (!Step.GiveItem.IsNone()) { Hero->AddItem(Step.GiveItem); }
	if (GM && !Step.SetFlag.IsNone()) { GM->SetFlag(Step.SetFlag); }
	if (!Step.DoneText.IsEmpty()) { Hero->ShowMessage(Step.DoneText); }
	GiveReward(Hero);
	if (Step.bOneShot)
	{
		bSpent = true;
		if (Look == EFNRiteLook::SkyArrow) { SetActorHiddenInGame(true); SetActorEnableCollision(false); }
	}
}

void AFNRiteObject::OnShot(AFNCharacter* Hero)
{
	if (!bSpent && !Step.ShotText.IsEmpty() && Hero) { Hero->ShowMessage(Step.ShotText); }
}

void AFNRiteObject::GiveReward(AFNCharacter* Hero)
{
	switch (Step.Reward)
	{
	case EFNRiteReward::HorseHelmet:
		// The skull leaves the pole and goes with the hero (cosmetic, GDD: no mandatory power).
		for (UActorComponent* C : GetComponents())
		{
			if (UStaticMeshComponent* M = Cast<UStaticMeshComponent>(C); M && M->GetRelativeLocation().Z > 150.f) { M->SetVisibility(false); }
		}
		Hero->GiveHelmet();
		break;
	case EFNRiteReward::Satchel:
		Hero->GiveSatchel();
		break;
	case EFNRiteReward::StrelokopCache:
	{
		// The cellar opens: the dead arrow-digger's thing (wax gold = loot channel).
		const FTransform At(GetActorLocation() + GetActorForwardVector() * 250.f + FVector(0.f, 0.f, 90.f));
		if (AFNPickup* P = GetWorld()->SpawnActorDeferred<AFNPickup>(AFNPickup::StaticClass(), At, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
		{
			P->InitType(EFNPickupType::Armor);
			P->FinishSpawning(At);
		}
		break;
	}
	default:
		break;
	}
}
