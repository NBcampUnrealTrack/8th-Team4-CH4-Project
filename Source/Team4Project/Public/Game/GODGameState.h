#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "GODGameState.generated.h"

UENUM(BlueprintType)
enum class EGamePhase : uint8
{
	WaitingForPlayers UMETA(DisplayName = "대기 중"),
	Playing UMETA(DisplayName = "진행 중"),
	CitizensWon UMETA(DisplayName = "보안관/시민 승리"),
	OutlawWon UMETA(DisplayName = "무법자 승리"),
	MafiaWon UMETA(DisplayName = "마피아 승리")
};

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

	// 총기 사용 가능 여부 (게임 시작 3분 후 True)
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game State")
	bool bGunsUnlocked;

	UFUNCTION()
	void OnRep_GamePhase();
};
