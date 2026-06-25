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
	
	void StartLocalVoice(APawn* P);
	
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
	void TrySendNickname();

	UFUNCTION(Server, Reliable)
	void Server_SetNickname(const FString& Nick);

protected:
	bool bNicknameSent = false;
	
public:
	UFUNCTION(Client, Reliable)
	void Client_StartSpectating();

	UFUNCTION(Server, Reliable)
	void Server_StartSpectating();
	
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
