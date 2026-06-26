// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_FootprintVision.generated.h"

/**
 * 순찰자 — 발자국 시각화 활성화
 * 마지막 30초 이동 경로를 바닥 데칼로 표시 (소유 클라이언트에만).
 * 쿨타임은 BP에서 CooldownGameplayEffectClass 로 지정.
 */
UCLASS()
class TEAM4PROJECT_API UGA_FootprintVision : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_FootprintVision();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
