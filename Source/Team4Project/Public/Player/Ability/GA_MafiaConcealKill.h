// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_MafiaConcealKill.generated.h"

/**
 * 마피아 시체 은폐 (패시브). 처치 이벤트(Event.Kill)로 트리거된다.
 * 오너가 마피아면 방금 죽인 대상(피해자)의 시체를 ConcealDuration 초 동안 숨긴다.
 * 마피아 캐릭터에게만 부여(DataTable JobAbilities)하면 되고, 안전을 위해 코드에서도 마피아 여부를 검사한다.
 */
UCLASS()
class TEAM4PROJECT_API UGA_MafiaConcealKill : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_MafiaConcealKill();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	// 시체를 숨기는 시간(초)
	UPROPERTY(EditDefaultsOnly, Category = "Mafia")
	float ConcealDuration = 30.f;
};
