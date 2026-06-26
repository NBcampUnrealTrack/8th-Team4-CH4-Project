// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_WireCutter.generated.h"

/**
 * 마피아 — 절단기 (기어 즉시 파괴)
 *
 * TriggerEventData->OptionalObject : 파괴할 기어 액터
 * 쿨타임 2분: BP에서 CooldownGameplayEffectClass 에 GE_Cooldown_WireCutter 지정
 */
UCLASS()
class TEAM4PROJECT_API UGA_WireCutter : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_WireCutter();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
