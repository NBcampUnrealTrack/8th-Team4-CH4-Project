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

	virtual void Shutdown() override;

	UFUNCTION(BlueprintCallable)
	void LoadMenuWidget();

	UFUNCTION(BlueprintCallable)
	void InGameLoadMenu();

	UFUNCTION(BlueprintCallable)
	void LobbyLoadMenu();

	UFUNCTION(Exec)
	virtual void Host(FString ServerName, FString Password) override;

	UFUNCTION(Exec)
	virtual void Join(uint32 Index, FString Password) override;

	// 빠른 방찾기: 검색 후 자리 있고 비밀번호 없는 방 중 최적을 자동 조인
	UFUNCTION(Exec)
	virtual void QuickJoin() override;

	virtual void LoadMainMenu() override;

	virtual void RefreshServerList() override;

	// 호스트가 설정한 세션 비밀번호 (빈 문자열 = 공개 방).
	// GameMode::PreLogin의 서버 권위 검증에서 사용한다. 세션에 광고되지 않는다.
	const FString& GetHostSessionPassword() const { return HostSessionPassword; }

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

	// ============================================================
	// 스킨 선택
	// ============================================================

	// 메인 메뉴에서 고른 스킨 인덱스 (BaseCharacter.SkinOptions 배열 인덱스).
	// 클라 로컬 값 — 캐릭터 스폰 후 Server_SetSkin 으로 서버에 전달된다.
	UPROPERTY(BlueprintReadWrite, Category = "Skin")
	int32 SelectedSkinIndex = 0;

	// ============================================================
	// 세션 설정
	// ============================================================

	// 방 최대 인원 (세션 정원). BP_PlayerGameInstance에서 조절 가능.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Session")
	int32 MaxSessionPlayers = 8;

	// ── 빠른 방찾기 판정 가중치 ──
	// 점수 = 현재인원 × PlayersWeight − 핑(ms) × PingWeight. 점수가 가장 높은 방에 참여.
	// 기본값(100:1)은 "핑 100ms 차이 = 인원 1명 차이"로 취급한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Session|QuickJoin")
	float QuickJoinPlayersWeight = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Session|QuickJoin")
	float QuickJoinPingWeight = 1.f;

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

	// 세션 델리게이트 핸들 — Shutdown에서 해제하기 위해 보관.
	FDelegateHandle CreateSessionCompleteDelegateHandle;
	FDelegateHandle DestroySessionCompleteDelegateHandle;
	FDelegateHandle FindSessionsCompleteDelegateHandle;
	FDelegateHandle JoinSessionCompleteDelegateHandle;
	FDelegateHandle NetworkFailureDelegateHandle;

	void OnCreateSessionComplete(FName Sessionname, bool Success);
	void OnDestorySessionComplete(FName Sessionname, bool Success);
	void OnFindSessionComplete(bool Success);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnNetworkFailure(UWorld* world, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);

	FString DesiredServerName;

	// Host() 입력 비밀번호 (세션 생성 대기 중 임시 보관)
	FString DesiredPassword;

	// 생성된 세션의 비밀번호 (호스트 머신 전용 — PreLogin 검증 소스)
	FString HostSessionPassword;

	// Join() 입력 비밀번호 — 접속 URL에 ?SessionPassword= 옵션으로 전달
	FString PendingJoinPassword;

	// QuickJoin() 진행 중 플래그 — 검색 완료 시 최적 방 자동 조인
	bool bQuickJoinPending = false;

	void CreateSession();

	void MoveToTitleMap();
};
