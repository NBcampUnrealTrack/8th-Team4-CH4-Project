// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GODPlayerState.h"
#include "VoicePlayerState.generated.h"

class UVoipTalkerComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerNameChanged, const FString&, NewName);

UCLASS()
class TEAM4PROJECT_API AVoicePlayerState : public AGODPlayerState
{
	GENERATED_BODY()

public:
	AVoicePlayerState();
	
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	virtual void OnSetUniqueId() override;

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void CopyProperties(APlayerState* PlayerState) override;

	// 생사 변화 시 발화 중인 Talker 전체 정책 재적용(Super) + 이 플레이어 자신의 Talker
	// Settings(2D/3D) baseline 도 즉시 갱신 → 조용히 있다가 부활해도 첫 발화가 3D 로 시작.
	virtual void OnRep_bIsAlive() override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Voice")
	TObjectPtr<UVoipTalkerComponent> VoipTalker;

	void ReregisterVoiceTalker();

	void HandlePostLoadMap(UWorld* LoadedWorld);
	FDelegateHandle VoicePostLoadMapHandle;

public:
	// travel 직전 호출(BasePlayerController): 이 플레이어의 VOIP 재생 컴포넌트를 죽는 월드에서 떼어낸다(크래시 방지).
	void StopVoicePlayback();

	// ============================================================
	// 게임 NickName 데이터
	// ============================================================
public:
	UFUNCTION(BlueprintCallable)
	FString GetPlayerInfoString();
	
	UPROPERTY(ReplicatedUsing = OnRep_PlayerNameString, BlueprintReadOnly)
	FString PlayerNameString;

	UFUNCTION()
	void OnRep_PlayerNameString();
	
	UPROPERTY(BlueprintAssignable, Category = "Player")
	FOnPlayerNameChanged OnPlayerNameChanged;
	
	void SetPlayerNameString(const FString& InName);
	
	virtual FString GetDisplayName() const override;
	

	FString GetPersistentPlayerID() const;
};
