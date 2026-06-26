// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_ForceClosePressureValve.generated.h"

/**
 * 화부 — 압력 밸브 강제 차단
 *
 * TriggerEventData->OptionalObject : 압력 밸브 액터
 * 쿨타임 없음.
 */
UCLASS()
class TEAM4PROJECT_API UGA_ForceClosePressureValve : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_ForceClosePressureValve();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
