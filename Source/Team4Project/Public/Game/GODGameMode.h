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

	/** 플레이어 접속 시 인원 체크 → MaxPlayers 충족 시 카운트다운 시작 */
	virtual void PostLogin(APlayerController* NewPlayer) override;

	/** 플레이어 이탈 시 카운트다운/대기 중이면 취소 */
	virtual void Logout(AController* Exiting) override;

	/** 카운트다운 종료 후 자동 호출. BP에서 강제 시작용으로도 사용 가능 */
	UFUNCTION(BlueprintCallable, Category = "Game Logic")
	void StartGame();

	UFUNCTION(BlueprintCallable, Category = "Game Logic")
	void HandlePlayerDeath(class AGODPlayerState* KillerPS, class AGODPlayerState* VictimPS);

	/** 탈선 유발 — GODTrain 정지 후 MafiaWon 처리 */
	UFUNCTION(BlueprintCallable, Category = "Game Logic")
	void TriggerDerailment();

	// ============================================================
	// 설정
	// ============================================================
	/** 게임 시작에 필요한 플레이어 수 (기본 5인) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Logic")
	int32 MaxPlayers = 5;

	/** 인원 충족 후 출발까지 카운트다운 시간 (초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Logic")
	int32 CountdownDuration = 5;

	/** 게임 종료 UI 표시 후 메인 메뉴 복귀까지 대기 시간 (초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Logic")
	float EndGameDelay = 5.f;

	/** 게임 종료 후 복귀할 메인 메뉴 맵 경로 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Logic|Maps")
	FString MainMenuMapPath = TEXT("/Game/Level/TitleMap");

private:
	FTimerHandle GameTimerHandle;
	FTimerHandle CountdownTimerHandle;
	FTimerHandle MenuReturnTimerHandle;

	void StartCountdown();
	void UpdateCountdown();
	void UpdateGameTimer();
	void AssignRoles();
	void CheckWinConditions();
	void EndGame(EGamePhase WinningPhase);

	/** 타이머 만료 시 메인 메뉴로 복귀. */
	UFUNCTION()
	void ReturnToMainMenu();

	class AGODTrain* FindTrainActor() const;

	int32 TotalMatchTime = 600;
	int32 TimeElapsed = 0;
};
