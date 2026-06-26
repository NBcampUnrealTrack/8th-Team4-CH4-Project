// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_UnlockDoor.generated.h"

/**
 * 보안관 — 문 따기 (마피아가 잠근 문 해제)
 *
 * TriggerEventData->OptionalObject : "Door.Locked" 태그를 가진 문 액터
 * 쿨타임 없음.
 */
UCLASS()
class TEAM4PROJECT_API UGA_UnlockDoor : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_UnlockDoor();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
