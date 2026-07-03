#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Game/ChatTypes.h"   // 채팅을 위한 헤더 추가 (준수)
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

UCLASS()
class TEAM4PROJECT_API AGODGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AGODGameState();

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

	UFUNCTION()
	void OnRep_ChatHistory();
};
