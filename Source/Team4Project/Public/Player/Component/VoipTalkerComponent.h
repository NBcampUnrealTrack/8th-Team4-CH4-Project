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

	/**
	 * 재생 컴포넌트를 죽는 월드에서 완전히 떼어낸다(Stop + Detach + Unregister) + Settings/구독 정리.
	 * travel 직전(ABasePlayerState::StopVoicePlayback) 및 Pawn 파괴 시 호출. 언제 불러도 안전.
	 */
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
};
