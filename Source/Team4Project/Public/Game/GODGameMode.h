#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GODGameState.h"
#include "GODGameMode.generated.h"

UCLASS()
class TEAM4PROJECT_API AGODGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:

	AGODGameMode();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Game Logic")
	void StartGame();

	UFUNCTION(BlueprintCallable, Category = "Game Logic")
	void StartSeamlessTravelToGame();

	UFUNCTION(BlueprintCallable, Category = "Game Logic")
	void HandlePlayerDeath(class AGODPlayerState* KillerPS, class AGODPlayerState* VictimPS);

	UFUNCTION(BlueprintCallable, Category = "Game Logic")
	void TriggerDerailment();

private:
	FTimerHandle GameTimerHandle;

	void UpdateGameTimer();
	void AssignRoles();
	void CheckWinConditions();
	void EndGame(enum EGamePhase WinningPhase);

	int32 TotalMatchTime = 600; // 10분 (600초)
	int32 TimeElapsed = 0;      // 진행 시간 추적용

};
