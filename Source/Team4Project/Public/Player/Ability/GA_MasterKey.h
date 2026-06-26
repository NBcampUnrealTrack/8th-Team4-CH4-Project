// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_MasterKey.generated.h"

/**
 * 마피아 — 마스터키 (문 잠금)
 *
 * TriggerEventData->OptionalObject : 잠글 문 액터
 * 쿨타임 1분: BP에서 CooldownGameplayEffectClass 에 GE_Cooldown_MasterKey 지정
 */
UCLASS()
class TEAM4PROJECT_API UGA_MasterKey : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_MasterKey();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
