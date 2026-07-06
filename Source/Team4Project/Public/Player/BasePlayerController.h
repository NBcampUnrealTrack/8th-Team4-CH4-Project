// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BasePlayerController.generated.h"

class UScrollBox;      
class UEditableText;

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
	// HUD 커서 모드: true 면 커서 표시 + UI 클릭 가능(GameAndUI), false 면 게임 전용(마우스 시야 전환).
	// Alt 홀드(아래 바인딩)와 가짜 시체(해제 버튼 클릭용) 등에서 사용. 관전/채팅 중에는 무시된다.
	void SetHUDCursorMode(bool bEnable);

	UFUNCTION(Client, Reliable)
	void Client_StartSpectating();

	UFUNCTION(Server, Reliable)
	void Server_StartSpectating();
	
	UFUNCTION(Server, Reliable)
	void Server_SpectateNext();
	
	// BasePlayerController.h
	UPROPERTY()
	TObjectPtr<UUserWidget> ChatBoxWidget;

	UPROPERTY(EditDefaultsOnly, Category = "Chat")
	TSubclassOf<UUserWidget> ChatBoxWidgetClass;   // BP에서 WBP_ChatBox 할당

	UPROPERTY(EditDefaultsOnly, Category = "Chat")
	TSubclassOf<UUserWidget> ChatEntryWidgetClass; // BP에서 WBP_ChatEntry 할당

	// UI 사운드 DT (채팅 수신음 UI.ChatReceive 행). 컨트롤러 BP 디폴트에서 지정.
	UPROPERTY(EditDefaultsOnly, Category = "Chat")
	TObjectPtr<class UDataTable> UISoundTable;

	UFUNCTION()
	void HandleChatMessage(const FChatMessage& Msg);

	// 접속 직후 히스토리 백필 중에는 수신음을 내지 않기 위한 플래그
	bool bChatBackfillInProgress = false;
	
	// 위젯에서 named slot/named widget 접근용
	UScrollBox*     GetChatScrollBox()  const;
	UEditableText*  GetChatInputWidget() const;
	void OnChatToggle();

	void OpenChat();
	
	UFUNCTION()
	void OnChatTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);
	void CloseChat();
	void SubmitChat(const FString& Message);

protected:
	bool bIsSpectating = false;
	bool bChatOpen = false; 

	// 살아있는 플레이어 목록에서 다음 대상 찾기
	APawn* FindNextSpectateTarget() const;

	// 현재 관전 중인 Pawn
	UPROPERTY()
	TWeakObjectPtr<APawn> CurrentSpectateTarget;

	// 마우스 클릭으로 다음 관전 대상 전환
	void OnSpectateNextClicked();

	// Alt 홀드 → HUD 커서 모드 토글
	void OnHUDCursorPressed();
	void OnHUDCursorReleased();

	// 두 소스(Alt 홀드 / 가짜 시체 등 강제)를 합성해 실제 입력 모드에 반영
	void ApplyHUDCursorMode();
	bool bCursorHeldByKey = false;
	bool bCursorForced = false;
	
};
