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

	// ============================================================
	// 게임 Voice 데이터
	// ============================================================
	// SetUniqueId() / OnRep_UniqueId() 양쪽에서 호출됨.
	// 클라에선 BeginPlay 시점에 UniqueId 복제가 아직 안 와 Talker 등록이 조용히 실패할 수 있어
	// UniqueId 가 유효해지는 이 시점에 재등록한다.
	virtual void OnSetUniqueId() override;

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	// seamless travel 시 신원(닉네임/스팀 ID)을 새 PlayerState 로 직접 넘긴다.
	// (엔진 기본 CopyProperties 는 커스텀 필드를 복사하지 않아 트래블 후 Player{N} 으로 되돌아갔음)
	virtual void CopyProperties(APlayerState* PlayerState) override;

protected:
	// 원격 음성 재생/공간화. 모든 인스턴스에서 생성되고 각자 로컬 VoiceTalkerMap 에 등록된다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Voice")
	TObjectPtr<UVoipTalkerComponent> VoipTalker;

	// 기존 등록을 지우고 현재 Talker 로 다시 등록한다. seamless travel 후 무음 방지용.
	void ReregisterVoiceTalker();

	// 맵 로드 완료마다 재등록(트래블 직후 옛 Talker 의 unregister 가 새 등록을 덮어쓰는 레이스 회피).
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

	// 온라인 닉네임(소유 클라가 Server_SetNickname 으로 올림) 또는 폴백("Player{N}"). 모든 클라에 복제.
	// 이 값이 곧 영속 저장 키(GetPersistentPlayerID)로도 재사용된다.
	UPROPERTY(ReplicatedUsing = OnRep_PlayerNameString, BlueprintReadOnly)
	FString PlayerNameString;

	UFUNCTION()
	void OnRep_PlayerNameString();

	// 이름 변경 알림 — 위젯 컴포넌트가 폴링 없이 바인딩해 갱신(닉네임이 RPC 로 늦게 도착해도 반영).
	UPROPERTY(BlueprintAssignable, Category = "Player")
	FOnPlayerNameChanged OnPlayerNameChanged;

	/** 서버: 이름 설정 + 복제/브로드캐스트(호스트 수동, 클라는 OnRep). */
	void SetPlayerNameString(const FString& InName);
	

	FString GetPersistentPlayerID() const;
};
