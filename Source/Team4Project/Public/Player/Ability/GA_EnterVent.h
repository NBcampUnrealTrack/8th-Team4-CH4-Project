// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_EnterVent.generated.h"

/**
 * 마피아 — 환풍구 진입/퇴장 토글
 *
 * 진입: TriggerEventData->OptionalObject 에 환풍구 액터가 있어야 함.
 *       CommitAbility 로 비용/쿨타임 소비 (쿨타임이 있다면 BP에서 GE 지정).
 * 퇴장: 이미 vent 상태면 바로 ExitVent 호출 (CommitAbility 없음).
 */
UCLASS()
class TEAM4PROJECT_API UGA_EnterVent : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_EnterVent();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
