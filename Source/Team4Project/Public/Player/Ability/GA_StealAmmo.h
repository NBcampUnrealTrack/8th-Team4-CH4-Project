// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_StealAmmo.generated.h"

/**
 * 무법자 — 도굴꾼 (사망자 탄약 탈취)
 *
 * TriggerEventData->OptionalObject : 시체 액터 (ABaseCharacter, IsDead() == true)
 * 쿨타임 없음.
 */
UCLASS()
class TEAM4PROJECT_API UGA_StealAmmo : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_StealAmmo();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
