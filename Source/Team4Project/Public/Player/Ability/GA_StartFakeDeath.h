// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_StartFakeDeath.generated.h"

/**
 * 무법자 — 가짜 시체 시작
 *
 * 쿨타임 30초: BP에서 CooldownGameplayEffectClass 에 GE_Cooldown_FakeDeath 지정
 * 가짜 시체 해제는 GA_StopFakeDeath 로 분리 (쿨타임 소비 없음).
 */
UCLASS()
class TEAM4PROJECT_API UGA_StartFakeDeath : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_StartFakeDeath();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
