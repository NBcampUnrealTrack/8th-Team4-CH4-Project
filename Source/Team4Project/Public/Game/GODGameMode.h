#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GODGameState.h"
#include "GameplayTagContainer.h"
#include "GODGameMode.generated.h"

class ABaseCharacter;
class AGODPlayerState;
class AQuestStation;

UCLASS()
class TEAM4PROJECT_API AGODGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGODGameMode();

	void StartCountdown();
	virtual void BeginPlay() override;

	/** 접속 승인 전 비밀번호 방 검증 (서버 권위 — 클라 조작으로 우회 불가) */
	virtual void PreLogin(const FString& Options, const FString& Address,
		const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

	/** 플레이어 접속 시 인원 체크 → MaxPlayers 충족 시 카운트다운 시작 */
	virtual void PostLogin(APlayerController* NewPlayer) override;

	/** 플레이어 이탈 시 카운트다운/대기 중이면 취소 */
	virtual void Logout(AController* Exiting) override;

	/** 카운트다운 종료 후 자동 호출. BP에서 강제 시작용으로도 사용 가능 */
	UFUNCTION(BlueprintCallable, Category = "Game Logic")
	void StartGame();

	/**모든 플레이어 같은 PlayerStart에서 시작(하나만 배치)*/
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	UFUNCTION(BlueprintCallable, Category = "Game Logic")
	void HandlePlayerDeath(class AGODPlayerState* KillerPS, class AGODPlayerState* VictimPS);

	// ============================================================
	// 퀘스트
	// ============================================================

	/** 완료 보고 처리. 배정된 스테이션일 때만 진행도 반영 + 특수직 탄약 보상. */
	void HandleQuestCompleted(AGODPlayerState* PS, AQuestStation* Station);

	/** 시민 완료 인원 / 유효 시민 수로 GameState 의 속도 배율을 다시 계산한다. */
	void RecalculateQuestSpeedMultiplier();

	/** 플레이어당 배정할 퀘스트 개수. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quest")
	int32 NumQuestsPerPlayer = 3;

	/** 탈선 유발 — GODTrain 정지 후 MafiaWon 처리 */
	UFUNCTION(BlueprintCallable, Category = "Game Logic")
	void TriggerDerailment();

	// ============================================================
	// 긴급 소집 (Meeting)
	// ============================================================

	/** 소집 벨에서 호출 (서버). 조건 충족 시 회의 시작. 성공 여부 반환. */
	UFUNCTION(BlueprintCallable, Category = "Meeting")
	bool TryStartMeeting();

	/** 회의 종료 → Playing 복귀 (타이머 만료 시 자동 호출. BP 강제 종료용으로도 사용 가능). */
	UFUNCTION(BlueprintCallable, Category = "Meeting")
	void EndMeeting();

	/** 지금 벨을 누를 수 있는가 (서버 전용 — 클라 프롬프트는 GameState.bMeetingBellReady 사용). */
	bool CanStartMeeting() const;

	/** 회의 중 열차 밖으로 떨어진 플레이어를 회의실 좌석으로 복귀 (BaseCharacter 낙하 판정에서 호출). */
	void RescueToMeeting(ABaseCharacter* Character);

	/** 회의(토론) 지속 시간 (초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meeting")
	int32 MeetingDuration = 60;

	/** 게임 시작 후 벨이 활성화되기까지의 시간 (초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meeting")
	int32 MeetingUnlockDelay = 60;

	/** 회의 종료 후 다시 벨을 누를 수 있기까지의 쿨다운 (초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meeting")
	int32 MeetingCooldownAfterEnd = 60;

	// ============================================================
	// 설정
	// ============================================================
	/** 게임 시작에 필요한 플레이어 수 (기본 8인, 세션 정원과 동일하게 유지) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Logic")
	int32 MaxPlayers = 8;

	/** 인원 충족 후 출발까지 카운트다운 시간 (초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Logic")
	int32 CountdownDuration = 5;

	/** 게임 종료 UI 표시 후 메인 메뉴 복귀까지 대기 시간 (초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Logic")
	float EndGameDelay = 5.f;

	/** 게임 시작 후 발포 잠금이 해제되기까지의 시간 (초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Logic")
	int32 GunUnlockDelay = 60;

	/** 남은 시간이 이 값에 도달하면 "시간 임박" 방송 (초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Logic")
	int32 TimeWarningSeconds = 60;

	/** 게임 종료 후 복귀할 메인 메뉴 맵 경로 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Logic|Maps")
	FString MainMenuMapPath = TEXT("/Game/Level/TitleMap");

	// ============================================================
	// 역할별 캐릭터 BP 클래스 — BP_GODGameMode 에서 지정
	// ============================================================
	UPROPERTY(EditDefaultsOnly, Category = "Spawning")
	TSubclassOf<ABaseCharacter> MafiaClass;

	UPROPERTY(EditDefaultsOnly, Category = "Spawning")
	TSubclassOf<ABaseCharacter> SheriffClass;

	UPROPERTY(EditDefaultsOnly, Category = "Spawning")
	TSubclassOf<ABaseCharacter> OutlawClass;

	UPROPERTY(EditDefaultsOnly, Category = "Spawning")
	TSubclassOf<ABaseCharacter> MechanicClass;

	UPROPERTY(EditDefaultsOnly, Category = "Spawning")
	TSubclassOf<ABaseCharacter> WatchmanClass;

	UPROPERTY(EditDefaultsOnly, Category = "Spawning")
	TSubclassOf<ABaseCharacter> StokerClass;

	UPROPERTY(EditDefaultsOnly, Category = "Spawning")
	TSubclassOf<ABaseCharacter> PorterClass;

private:
	FTimerHandle GameTimerHandle;
	FTimerHandle CountdownTimerHandle;
	FTimerHandle MenuReturnTimerHandle;
	FTimerHandle MeetingTimerHandle;

	// ── 긴급 소집 (서버 전용 상태) ──
	// 직전 회의가 끝난 서버 시각. 음수면 아직 회의가 없었다.
	double LastMeetingEndTime = -1.0;
	// 회의 직전 열차가 달리고 있었는지. 기어 전멸 정지 등과 겹칠 때 잘못 재출발하지 않기 위함.
	bool bTrainWasRunningBeforeMeeting = false;

	void UpdateMeetingTimer();
	void SetMeetingTagOnAllCharacters(bool bEnable);
	class AMeetingRoom* FindMeetingRoom() const;

	void UpdateCountdown();
	void UpdateGameTimer();
	void AssignRoles();

	/** 레벨의 QuestStation 을 모아 플레이어마다 겹치지 않게 랜덤 배정한다. */
	void AssignQuests();

	void CheckWinConditions();
	void EndGame(EGamePhase WinningPhase);

	FGameplayTag GetTagForRole(AGODPlayerState* PS) const;
	TSubclassOf<ABaseCharacter> GetClassForRole(AGODPlayerState* PS) const;
	void RespawnPlayerAsRole(APlayerController* PC, TSubclassOf<ABaseCharacter> CharClass);
	void SetupWatchmanTracking();

	/** 타이머 만료 시 메인 메뉴로 복귀. */
	UFUNCTION()
	void ReturnToMainMenu();

	/** 압력 100% 폭발 시 호출 → 탈선 처리 + 마피아 승리. */
	UFUNCTION()
	void HandlePressureExplosion();

	class AGODTrain* FindTrainActor() const;

	int32 TotalMatchTime = 600;
	int32 TimeElapsed = 0;
};
