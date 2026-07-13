#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Game/ChatTypes.h"   // 채팅을 위한 헤더 추가 (준수)
#include "Game/AnnouncementTypes.h"
#include "GODGameState.generated.h"

UENUM(BlueprintType)
enum class EGamePhase : uint8
{
	WaitingForPlayers UMETA(DisplayName = "대기 중"),
	Countdown         UMETA(DisplayName = "출발 카운트다운"),
	Playing           UMETA(DisplayName = "진행 중"),
	CitizensWon       UMETA(DisplayName = "보안관/시민 승리"),
	OutlawWon         UMETA(DisplayName = "무법자 승리"),
	MafiaWon          UMETA(DisplayName = "마피아 승리")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChatMessageReceived, const FChatMessage&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGamePhaseChanged, EGamePhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRemainingTimeChanged, int32, NewTime);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDistanceChanged, float, NewDistance);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPressureLevelChanged, float, NewPressure);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTrainFuelLevelChanged, float, NewFuel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGunsUnlocked);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnQuestProgressChanged, float, SpeedMultiplier, int32, CompletedCitizens, int32, TotalCitizens);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAnnouncement, const FText&, Message, EAnnouncementType, Type);

class UDataTable;

UCLASS()
class TEAM4PROJECT_API AGODGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AGODGameState();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(ReplicatedUsing = OnRep_GamePhase, BlueprintReadOnly, Category = "Game State")
	EGamePhase CurrentPhase;

	UPROPERTY(ReplicatedUsing = OnRep_RemainingTime, BlueprintReadOnly, Category = "Game State")
	int32 RemainingTime;

	UPROPERTY(ReplicatedUsing = OnRep_DistanceToDestination, BlueprintReadOnly, Category = "Game State")
	float DistanceToDestination;

	/** 압력 게이지 현재값 (0~100). GODTrain에서 매 Tick 동기화. */
	UPROPERTY(ReplicatedUsing = OnRep_PressureLevel, BlueprintReadOnly, Category = "Game State")
	float PressureLevel = 0.f;

	/** 연료 비율 (0~1). GODTrain Furnace에서 매 Tick 동기화. */
	UPROPERTY(ReplicatedUsing = OnRep_FuelLevel, BlueprintReadOnly, Category = "Game State")
	float FuelLevel = 0.f;

	/** 게임 시작 3분 후 true 로 전환 (발포 잠금 해제) */
	UPROPERTY(ReplicatedUsing = OnRep_bGunsUnlocked, BlueprintReadOnly, Category = "Game State")
	bool bGunsUnlocked;

	/**
	 * 퀘스트 진행에 따른 열차 속도 배율 (1.0 ~ 2.0).
	 * = 1.0 + (퀘스트를 전부 끝낸 시민 수) / (유효 시민 수).
	 * 유효 시민 = 살아있는 시민 + 이미 완료하고 죽은 시민. 미완료 상태로 죽으면 분모에서 빠지므로
	 * 이 값은 절대 감소하지 않는다 (마피아는 킬로 속도를 늦출 수 없다).
	 */
	UPROPERTY(ReplicatedUsing = OnRep_QuestProgress, BlueprintReadOnly, Category = "Quest")
	float QuestSpeedMultiplier = 1.f;

	/** 전체 진행 바 표시용. 마피아에게도 보인다(압박 장치). */
	UPROPERTY(ReplicatedUsing = OnRep_QuestProgress, BlueprintReadOnly, Category = "Quest")
	int32 QuestCompletedCitizens = 0;

	UPROPERTY(ReplicatedUsing = OnRep_QuestProgress, BlueprintReadOnly, Category = "Quest")
	int32 QuestTotalCitizens = 0;

	UPROPERTY(BlueprintAssignable, Category = "Quest")
	FOnQuestProgressChanged OnQuestProgressChanged;

	UFUNCTION()
	void OnRep_QuestProgress();

	/**
	 * 인원 충족 후 출발까지 남은 카운트다운 (초).
	 * HUD 에서 숫자 표시용으로 사용. Countdown 페이즈 동안만 유효.
	 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game State")
	int32 LobbyCountdown;

	/**
	 * 페이즈 변경 시 서버와 클라이언트 모두에서 브로드캐스트.
	 * 출발 연출, 승리/패배 UI 등 위젯에서 이 델리게이트를 바인딩.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Game State|Events")
	FOnGamePhaseChanged OnGamePhaseChanged;

	/** 남은 시간 변경 시 브로드캐스트 (1초마다). HUD 타이머 위젯에서 바인딩. */
	UPROPERTY(BlueprintAssignable, Category = "Game State|Events")
	FOnRemainingTimeChanged OnRemainingTimeChanged;

	/** 열차 목적지까지 거리 변경 시 브로드캐스트. 진행도 프로그레스 바에서 바인딩. */
	UPROPERTY(BlueprintAssignable, Category = "Game State|Events")
	FOnDistanceChanged OnDistanceToDestinationChanged;

	/** 압력 수치 변경 시 브로드캐스트 (0~100). 압력 게이지 위젯에서 바인딩. */
	UPROPERTY(BlueprintAssignable, Category = "Game State|Events")
	FOnPressureLevelChanged OnPressureLevelChanged;

	/** 연료 비율 변경 시 브로드캐스트 (0~1). 연료 게이지 위젯에서 바인딩. */
	UPROPERTY(BlueprintAssignable, Category = "Game State|Events")
	FOnTrainFuelLevelChanged OnFuelLevelChanged;

	/** 3분 발포 잠금 해제 시 브로드캐스트. HUD "총기 제한 해제" 알림에서 바인딩. */
	UPROPERTY(BlueprintAssignable, Category = "Game State|Events")
	FOnGunsUnlocked OnGunsUnlocked;

	UFUNCTION()
	void OnRep_bGunsUnlocked();

	UFUNCTION()
	void OnRep_GamePhase();

	UFUNCTION()
	void OnRep_RemainingTime();

	UFUNCTION()
	void OnRep_DistanceToDestination();

	UFUNCTION()
	void OnRep_PressureLevel();

	UFUNCTION()
	void OnRep_FuelLevel();
	
	
	
	// 채팅 관련 코드 추가했습니당 주석은 나중에 지울게용(준수)
	UPROPERTY(ReplicatedUsing = OnRep_ChatHistory)
	TArray<FChatMessage> ChatHistory;

	UPROPERTY(BlueprintAssignable)
	FOnChatMessageReceived OnChatMessageReceived;

	void AddChatMessage(const FChatMessage& Msg);

	// ── 알림 방송 ──
	// 서버에서 호출. Playing 페이즈에만 나가며, 전 클라의 HUD 배너에 표시된다.
	// (라운드 초기화 중 ForceReassemble 등이 방송을 쏘지 않도록 페이즈로 게이트)
	void Announce(const FText& Message, EAnnouncementType Type = EAnnouncementType::Info);

	UPROPERTY(BlueprintAssignable, Category = "Announcement")
	FOnAnnouncement OnAnnouncement;

private:
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_Announce(const FText& Message, EAnnouncementType Type);

public:

	UFUNCTION()
	void OnRep_ChatHistory();

	// ============================================================
	// 게임 전체 사운드 — 게임 사운드 DT (경고/출발/승패. 행 이름은 SoundRows 참조)
	// ============================================================

	/** 게임 전체 사운드 DT. GameState BP 디폴트에서 이것 하나만 지정. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<UDataTable> GameSoundTable;

	/** 압력 경고음이 울리기 시작하는 압력값 (0~100) */
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	float PressureWarningThreshold = 80.f;

	/** 연료 부족음이 울리기 시작하는 연료 비율 (0~1) */
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	float FuelLowThreshold = 0.2f;

	/** 압력이 폭발 임계값에 닿는 지점 (PressureComponent.ExplosionThreshold 와 맞출 것) */
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	float PressureCriticalLevel = 100.f;

	/** 경고 임계값에서의 비프 간격 (초). 비프 한 번 길이보다 길게 둘 것 — 짧으면 재생이 겹친다. */
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	float PressureWarningIntervalMax = 1.5f;

	/** 폭발 직전의 비프 간격 (초). 압력이 오를수록 Max 에서 여기까지 좁혀진다. */
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	float PressureWarningIntervalMin = 0.4f;

	/** 연료 부족음 반복 간격 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	float FuelWarningInterval = 3.f;

protected:
	/**
	 * 0.25초 주기로 페이즈/압력/연료를 감시해 각 머신 로컬에서 사운드 재생.
	 * OnRep 대신 폴링인 이유: 리슨 서버(호스트)는 OnRep 이 호출되지 않고,
	 * 압력/연료는 매 틱 갱신이라 OnRep 기반 트리거는 스팸이 되기 때문.
	 */
	void SoundMonitorTick();
	void PlayPhaseSound(EGamePhase NewPhase);

	/** 현재 압력에 따른 비프 간격. 폭발 임계값에 가까울수록 짧아진다. */
	float GetPressureWarningInterval() const;

	/** 로컬 플레이어의 MainRole 기준 승리 여부 (승리/패배 사운드 분기). */
	bool IsLocalPlayerWinner(EGamePhase WinPhase) const;

	FTimerHandle SoundMonitorTimer;
	EGamePhase LastSoundPhase = EGamePhase::WaitingForPlayers;
	double LastPressureWarningTime = -1e9;
	double LastFuelWarningTime = -1e9;

	// 연료가 한 번이라도 임계값 위로 올라온 뒤에만 부족 경고 (시작 직후 0 초기값 오경보 방지).
	bool bFuelInitialized = false;
};
