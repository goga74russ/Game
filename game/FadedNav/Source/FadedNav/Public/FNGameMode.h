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

private:
	void BuildArena();

	UPROPERTY()
	TObjectPtr<AActor> SpawnPoint;
};
