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

UCLASS()
class TEAM4PROJECT_API AGODGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AGODGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(ReplicatedUsing = OnRep_GamePhase, BlueprintReadOnly, Category = "Game State")
	EGamePhase CurrentPhase;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game State")
	int32 RemainingTime;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game State")
	float DistanceToDestination;

	/** 게임 시작 3분 후 true 로 전환 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game State")
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

	UFUNCTION()
	void OnRep_GamePhase();
	
	
	
	// 채팅 관련 코드 추가했습니당 주석은 나중에 지울게용(준수)
	UPROPERTY(ReplicatedUsing = OnRep_ChatHistory)
	TArray<FChatMessage> ChatHistory;

	UPROPERTY(BlueprintAssignable)
	FOnChatMessageReceived OnChatMessageReceived; 
	
	void AddChatMessage(const FChatMessage& Msg);

	UFUNCTION()
	void OnRep_ChatHistory();
};
