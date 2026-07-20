// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Net/VoiceConfig.h"
#include "VoipTalkerComponent.generated.h"

class UAudioComponent;
class APawn;

UCLASS()
class TEAM4PROJECT_API UVoipTalkerComponent : public UVOIPTalker
{
	GENERATED_BODY()

public:
	virtual void OnTalkingBegin(UAudioComponent* AudioComponent) override;
	virtual void OnTalkingEnd() override;

	// 발화 전에 OnPawnSet 을 미리 구독해 Settings(감쇠/부착 대상)를 항상 최신 상태로 유지한다.
	// 엔진은 발화 세션 시작 시점(OnTalkingBegin 직전)에 Settings 를 읽어 synth 에 감쇠를 적용하므로,
	// 그 전에 Settings 가 채워져 있어야 3D 감쇠가 실제로 걸린다.
	virtual void OnRegister() override;
	virtual void OnUnregister() override;

	void TeardownVoiceAudio();

	/**
	 * 발화자/청자의 생사 조합에 따라 볼륨을 재생 중인 컴포넌트에 적용(전 구간 2D).
	 *  - 생존 발화자: 전원에게 VoiceVolume 으로 들림(청자 생사·거리 무관).
	 *  - 사망 발화자: 죽은 청자에게만 VoiceVolume, 산 청자는 음소거(0).
	 * 호출 시점: OnTalkingBegin(발화 시작) + VoiceChannelSubsystem(생사 변화 이벤트).
	 * 폴링 없음 — 상태가 바뀌는 순간에만 재적용된다.
	 */
	void ApplyVoicePolicy();

	// 이 머신의 로컬 청자 입장에서 이 발화자가 내야 할 볼륨(0 = 음소거).
	// 오디빌리티 정책 전체가 여기 한 곳에 모여 있다(생존→전원, 사망→죽은 청자만).
	float ComputeListenerVolume() const;

	// 보이스 공통 기본 볼륨(초기값). 1.0 = 원음, 그 이상은 증폭.
	UPROPERTY(EditDefaultsOnly, Category = "Voice", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float VoiceVolume = 1.f;

	// 현재 생사 상태·빙의 Pawn 을 반영해 Settings.ComponentToAttachTo 만 갱신.
	// (재생 중이 아니어도 호출 가능 — 다음 발화 세션에 엔진이 이 값을 읽어 적용한다.)
	void RefreshVoiceSettings();

private:
	/** 발화 중 Pawn 이 늦게 붙는 경우(빙의 지연) attach/감쇠를 바로잡기 위한 콜백 */
	UFUNCTION()
	void HandleOwnerPawnSet(APlayerState* Player, APawn* NewPawn, APawn* OldPawn);

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
