// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_StopFakeDeath.generated.h"

/**
 * 무법자 — 가짜 시체 해제
 * 쿨타임 없음 (GA_StartFakeDeath 의 쿨타임과 분리).
 */
UCLASS()
class TEAM4PROJECT_API UGA_StopFakeDeath : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_StopFakeDeath();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
