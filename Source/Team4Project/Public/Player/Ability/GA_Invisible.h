// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_Invisible.generated.h"

/**
 * 투명화 어빌리티. 발동 시 일정 시간(InvisibleDuration) 동안 본인 메시(+무기/아이템)를 숨긴다.
 * 시간이 지나면 캐릭터가 자동으로 복구한다(타이머는 캐릭터가 관리).
 */
UCLASS()
class TEAM4PROJECT_API UGA_Invisible : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Invisible();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// 종료 시 지속 타이머를 정리(조기 취소 대비).
	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	// 투명화 지속 시간(초)
	UPROPERTY(EditDefaultsOnly, Category = "Invisible")
	float InvisibleDuration = 10.f;

private:
	// 지속시간 동안 어빌리티를 활성 유지(ActivationOwnedTags 로 공격 차단) → 만료 시 종료.
	FTimerHandle DurationTimerHandle;
	void OnDurationEnd();
};
