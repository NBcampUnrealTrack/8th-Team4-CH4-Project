// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_FireGun.generated.h"

/**
 * 총기 발사 어빌리티 (공통). State.Weapon.GunEquipped 상태에서만 발동.
 * 라인트레이스로 명중 판정 후, 맞은 캐릭터를 즉시 사망 처리(마피아 게임 룰).
 * 연사 방지는 Cooldown GE(긴 지속시간) / 탄약 소모는 Cost GE 로 BP 에서 설정.
 */
UCLASS()
class TEAM4PROJECT_API UGA_FireGun : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_FireGun();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	// 사거리(언리얼 단위, cm)
	UPROPERTY(EditDefaultsOnly, Category = "Fire")
	float Range = 10000.f;

	// 디버그 라인 표시 (개발용)
	UPROPERTY(EditDefaultsOnly, Category = "Fire")
	bool bDrawDebug = false;
};
