// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GameplayTagContainer.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "UI/MenuSystem/MenuInterface.h"
#include "UI/MenuSystem/InGameMenu.h"

#include "Game/GODGameState.h"
#include "PlayerGameInstance.generated.h"

class UItemDefinition;

UENUM(BlueprintType)
enum class EMatchState : uint8
{
	Lobby      UMETA(DisplayName = "Lobby"),
	Transition UMETA(DisplayName = "Transition"),
	InGame     UMETA(DisplayName = "InGame"),
	Result     UMETA(DisplayName = "Result"),
	GameOver   UMETA(DisplayName = "GameOver")
};

USTRUCT(BlueprintType)
struct FPlayerPersistentData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	int32 Credits = 0;
};

UCLASS()
class TEAM4PROJECT_API UPlayerGameInstance : public UGameInstance, public IMenuInterface
{
	GENERATED_BODY()

public:
	UPlayerGameInstance(const FObjectInitializer& ObjectInitializer);

	virtual void Init() override;

	UFUNCTION(BlueprintCallable)
	void LoadMenuWidget();

	UFUNCTION(BlueprintCallable)
	void InGameLoadMenu();

	UFUNCTION(BlueprintCallable)
	void LobbyLoadMenu();

	UFUNCTION(Exec)
	virtual void Host(FString ServerName) override;

	UFUNCTION(Exec)
	virtual void Join(uint32 Index) override;

	virtual void LoadMainMenu() override;

	virtual void RefreshServerList() override;

	// ============================================================
	// 플레이어 데이터 영속화
	// ============================================================

	// 트래블 스냅샷 저장/복원 (per-player 런타임 권위는 ABasePlayerState. 여기는 ServerTravel 간 보존용 저장소).
	UFUNCTION(BlueprintCallable, Category = "PlayerData")
	void SavePlayerData(const FString& PlayerID, const FPlayerPersistentData& Data);

	UFUNCTION(BlueprintCallable, Category = "PlayerData")
	bool LoadPlayerData(const FString& PlayerID, FPlayerPersistentData& OutData) const;

	// 스냅샷 저장소의 모든 플레이어 스크랩을 0으로 — 스테이지 전환 시 스크랩만 리셋(크레딧은 유지)하는 용도.
	//UFUNCTION(BlueprintCallable, Category = "PlayerData")
	//void ResetScrapForAllPlayers();

	// ============================================================
	// ServerTravel 간 매치 상태 보존
	// ============================================================

	/** Travel 직전에 호출 — GameState 데이터를 여기에 보관 */
	UFUNCTION(BlueprintCallable, Category = "Match")
	void SaveMatchState(EMatchState InMatchState, int32 InStage, float InDifficulty);

	/** Travel 직후 BeginPlay에서 호출 — 보관된 데이터를 GameState에 복원 */
	UFUNCTION(BlueprintCallable, Category = "Match")
	void RestoreMatchState(EMatchState& OutMatchState, int32& OutStage, float& OutDifficulty);

	/** 보존된 매치 상태가 유효한지 */
	UPROPERTY(BlueprintReadOnly, Category = "Match")
	bool bHasPendingMatchState = false;

	/** 보존된 매치 상태 */
	UPROPERTY(BlueprintReadOnly, Category = "Match")
	EMatchState PendingMatchState = EMatchState::Lobby;

	/** 보존된 스테이지 */
	UPROPERTY(BlueprintReadOnly, Category = "Match")
	int32 PendingStage = 0;

	/** 보존된 난이도 */
	UPROPERTY(BlueprintReadOnly, Category = "Match")
	float PendingDifficulty = 1.0f;

private:
	int32 PendingSharedCredits = 0;

	TMap<FString, FPlayerPersistentData> PlayerDataMap;

	TSubclassOf<class UUserWidget> MenuClass;
	TSubclassOf<class UUserWidget> InGameMenuClass;
	TSubclassOf<class UUserWidget> LobbyMenuClass;

	UPROPERTY()
	TObjectPtr<class UMainMenu> Menu;

	UPROPERTY()
	TObjectPtr<UMenuWidget> InGameMenuInstance;

	IOnlineSessionPtr SessionInterface;
	TSharedPtr<class FOnlineSessionSearch> SessionSearch;

	void OnCreateSessionComplete(FName Sessionname, bool Success);
	void OnDestorySessionComplete(FName Sessionname, bool Success);
	void OnFindSessionComplete(bool Success);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnNetworkFailure(UWorld* world, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);

	FString DesiredServerName;

	void CreateSession();

	void MoveToTitleMap();
};
