// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BasePlayerController.generated.h"

/**
 * 
 */
UCLASS()
class TEAM4PROJECT_API ABasePlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	ABasePlayerController();
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void AcknowledgePossession(APawn* P) override;
	virtual void SetupInputComponent() override;

	virtual void PreClientTravel(const FString& PendingURL, ETravelType TravelType, bool bIsSeamlessTravel) override;

	// ============================================================
	// 보이스(VOIP)
	// ============================================================
	// 리스너를 Pawn 에 고정 + 오픈 마이크 시작. 로컬 컨트롤러 1회만. 호스트/클라 양쪽 진입점에서 호출.
	void StartLocalVoice(APawn* P);

	// seamless travel 직전 호출: 발화를 멈춰 오디오 스레드의 active VOIP sound 를 비운다.
	// 이게 없으면 월드가 파괴돼도 오디오 렌더 스레드가 죽은 synth 를 계속 처리하다 크래시한다.
	void StopLocalVoice();

	bool bVoiceStarted = false;
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnRep_PlayerState() override;

	virtual void PostSeamlessTravel() override;

	void HandlePostLoadMap(UWorld* LoadedWorld);
	
	FDelegateHandle PostLoadMapHandle;
	
public:
	// ============================================================
	// 온라인 닉네임(스팀 계정) — 소유 클라가 로컬 OnlineSubsystem 닉네임을 서버로 올려 PlayerState 에 복제.
	// ============================================================
	void TrySendNickname();

	UFUNCTION(Server, Reliable)
	void Server_SetNickname(const FString& Nick);

protected:
	bool bNicknameSent = false;
	
public:
	// ============================================================
	// 관전 시스템
	// ============================================================
	/** 서버에서 호출 — 관전 모드 시작 */
	UFUNCTION(Client, Reliable)
	void Client_StartSpectating();

	UFUNCTION(Server, Reliable)
	void Server_StartSpectating();

	/** 다음 플레이어 시점으로 전환 */
	UFUNCTION(Server, Reliable)
	void Server_SpectateNext();
	
	void OpenChat();
	void CloseChat();
	void SubmitChat(const FString& Message);
	
	UFUNCTION(BlueprintImplementableEvent)
	void OnChatOpened();

	UFUNCTION(BlueprintImplementableEvent)
	void OnChatClosed();

	bool bChatOpen = false;

protected:
	bool bIsSpectating = false;

	// 살아있는 플레이어 목록에서 다음 대상 찾기
	APawn* FindNextSpectateTarget() const;

	// 현재 관전 중인 Pawn
	UPROPERTY()
	TWeakObjectPtr<APawn> CurrentSpectateTarget;

	// 마우스 클릭으로 다음 관전 대상 전환
	void OnSpectateNextClicked();
};
