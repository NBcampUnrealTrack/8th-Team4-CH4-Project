// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Net/VoiceConfig.h"
#include "VoipTalkerComponent.generated.h"

class USoundAttenuation;
class UAudioComponent;
class APawn;

UCLASS()
class TEAM4PROJECT_API UVoipTalkerComponent : public UVOIPTalker
{
	GENERATED_BODY()

public:
	virtual void OnTalkingBegin(UAudioComponent* AudioComponent) override;
	virtual void OnTalkingEnd() override;
	
	void TeardownVoiceAudio();
	
	UPROPERTY(EditDefaultsOnly, Category = "Voice")
	TObjectPtr<USoundAttenuation> VoiceAttenuationOverride;

	// 근접 보이스 최대 볼륨 반경(cm). 기본 감쇠 생성에 사용.
	UPROPERTY(EditDefaultsOnly, Category = "Voice")
	float InnerRadius = 400.f;

	// 반경 밖에서 0 까지 감쇠하는 거리(cm)
	UPROPERTY(EditDefaultsOnly, Category = "Voice")
	float FalloffDistance = 1600.f;

private:
	// 에셋이 없을 때 한 번만 만들어 재사용하는 런타임 감쇠.
	UPROPERTY(Transient)
	TObjectPtr<USoundAttenuation> RuntimeAttenuation;

	/** 현재 재생 중인 synth 의 AudioComponent. teardown 대상. */
	TWeakObjectPtr<UAudioComponent> PlayingAudioComponent;

	/** OnEndPlay 를 구독 중인 발화자 Pawn. 파괴되면 즉시 teardown. */
	TWeakObjectPtr<APawn> BoundPawn;

	/** 발화 중 캐릭터가 파괴(seamless travel 포함)되는 순간 호출 → teardown. */
	UFUNCTION()
	void HandleOwnerPawnEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason);

	/** OwnerPawn 의 OnEndPlay 구독 보장(중복 방지). */
	void BindOwnerPawnEndPlay(APawn* OwnerPawn);

	/** 현재 구독 해제. */
	void UnbindOwnerPawnEndPlay();

	USoundAttenuation* ResolveAttenuation();

	/**
	 * 발화자(이 Talker 의 오너 PlayerState)가 사망 상태인지.
	 * PlayerState->bIsAlive 와 빙의 중인 캐릭터의 IsDead() 둘 다 확인한다
	 * (현재 사망 경로가 둘 중 한쪽만 갱신할 수 있어 OR 로 판정).
	 */
	bool IsSpeakerDead() const;

	/**
	 * 이 머신의 로컬 청자(내 플레이어)가 사망 상태인지.
	 * 죽은 사람 목소리를 들을 자격이 있는지 판단하는 데 쓴다(죽은 사람만 들림).
	 */
	bool IsLocalListenerDead() const;
};
