#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Quest/QuestTypes.h"
#include "GODPlayerState.generated.h"

class AQuestStation;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAssignedQuestsChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTurnedToMafia);

// 최대 글자수
static constexpr int32 MaxChatLength = 100;

// 메인 역할군 (3파전)
UENUM(BlueprintType)
enum class EMainRole : uint8
{
	Citizen UMETA(DisplayName = "시민"),
	Sheriff UMETA(DisplayName = "보안관"),
	Outlaw UMETA(DisplayName = "무법자"),
	Mafia UMETA(DisplayName = "마피아")
};

// 시민 전용 세부 직업 클래스
UENUM(BlueprintType)
enum class ECitizenClass : uint8
{
	None, // 시민이 아닐 경우
	Mechanic UMETA(DisplayName = "정비공"),
	Watchman UMETA(DisplayName = "순찰자"),
	Stoker UMETA(DisplayName = "화부"),
	Porter UMETA(DisplayName = "짐꾼")
};

UCLASS()
class TEAM4PROJECT_API AGODPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
	AGODPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 로비 준비 상태
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Lobby")
	bool bIsReady;

	// 역할군 및 직업
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Role")
	EMainRole MainRole;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Role")
	ECitizenClass CitizenClass;

	// 총기 및 생존 상태.
	// 쓰기는 반드시 SetIsAlive()로 — 보이스 채널(생사별 격리) 갱신이 걸려 있다.
	UPROPERTY(ReplicatedUsing = OnRep_bIsAlive, BlueprintReadOnly, Category = "State")
	bool bIsAlive;

	// 서버 전용 생사 설정. 변경 시 이 머신(리슨 호스트)의 보이스 정책을 즉시 갱신하고,
	// 클라이언트들은 OnRep_bIsAlive 복제 도착 시점에 각자 갱신한다.
	void SetIsAlive(bool bNewAlive);

	UFUNCTION()
	virtual void OnRep_bIsAlive();

	// 오염도 (0.0 ~ 1.0)
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "State")
	float SootLevel;
	
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SendChat(const FString& Message);

	// ============================================================
	// 퀘스트
	// ============================================================

	// 이 플레이어에게 배정된 퀘스트 (스테이션 인스턴스 단위).
	UPROPERTY(ReplicatedUsing = OnRep_AssignedQuests, BlueprintReadOnly, Category = "Quest")
	TArray<FAssignedQuest> AssignedQuests;

	UFUNCTION()
	void OnRep_AssignedQuests();

	// 배정 목록이 바뀔 때(초기 배정 / 완료) 발화. 좌상단 목록 위젯이 바인딩한다.
	UPROPERTY(BlueprintAssignable, Category = "Quest")
	FOnAssignedQuestsChanged OnAssignedQuestsChanged;

	// 서버 전용. 해당 스테이션이 배정돼 있고 아직 미완료면 완료로 표시하고 true 를 반환한다.
	bool TryMarkQuestCompleted(AQuestStation* Station);

	UFUNCTION(BlueprintPure, Category = "Quest")
	bool AreAllQuestsCompleted() const;

	UFUNCTION(BlueprintPure, Category = "Quest")
	int32 GetCompletedQuestCount() const;

	// 속도 배율에 기여하는가
	// 전원 완료 시 배율 = 1 + N/N = 2.0 (열차 속도 2배). 마피아/무법자는 배정 자체가 없다.
	bool ContributesToQuestSpeed() const
	{
		return MainRole == EMainRole::Citizen || MainRole == EMainRole::Sheriff;
	}

	// ============================================================
	// 무법자 (이중스파이)
	// ============================================================

	/**
	 * 5분 전향 여부. 게임 시작 후 OutlawTurnDelaySeconds 에 GameMode 가 true 로 전환.
	 * 승리 사이드 판정에 사용: 전향 전 = 시민 사이드, 전향 후 = 마피아 사이드.
	 * 역할 노출 방지를 위해 소유자에게만 복제.
	 */
	UPROPERTY(ReplicatedUsing = OnRep_bTurnedToMafia, BlueprintReadOnly, Category = "Outlaw")
	bool bTurnedToMafia = false;

	UFUNCTION()
	void OnRep_bTurnedToMafia();

	// 전향 시 발화 (서버/소유 클라). HUD 전향 연출("마피아에 합류")이 바인딩한다.
	UPROPERTY(BlueprintAssignable, Category = "Outlaw")
	FOnTurnedToMafia OnTurnedToMafia;
};
