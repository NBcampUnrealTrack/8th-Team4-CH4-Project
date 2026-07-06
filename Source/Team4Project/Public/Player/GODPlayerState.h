#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "GODPlayerState.generated.h"

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
	void OnRep_bIsAlive();

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "State")
	int32 AmmoCount;

	// 오염도 (0.0 ~ 1.0)
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "State")
	float SootLevel;
	
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SendChat(const FString& Message);
	
};
