#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FNGameMode.generated.h"

// Builds a grey-box arena in code (floor, pillars, sun, sky, Perun) so the tech test runs on the engine's empty map.
UCLASS()
class FADEDNAV_API AFNGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFNGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual AActor* FindPlayerStart_Implementation(AController* Player, const FString& IncomingName) override;

	// Kill counter for the Spark -> Skeleton -> Flesh evolution (GDD §4: ~5 kills per stage [D]).
	void NotifyMobKilled(class AFNMob* Mob);
	int32 GetKills() const { return Kills; }

private:
	void BuildArena();
	void SpawnChapterMobs();
	void SpawnChapterItems();

	int32 Kills = 0;
	int32 NextRune = 0;
	int32 RuneMisses = 0;

	UPROPERTY()
	TObjectPtr<AActor> SpawnPoint;

	bool bArenaTest = false;
};
