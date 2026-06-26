// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_ToggleLantern.generated.h"

/**
 * 순찰자 — 랜턴 토글
 * 쿨타임/비용 없음. Character.Crew.Watchman 태그 필요.
 */
UCLASS()
class TEAM4PROJECT_API UGA_ToggleLantern : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_ToggleLantern();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
