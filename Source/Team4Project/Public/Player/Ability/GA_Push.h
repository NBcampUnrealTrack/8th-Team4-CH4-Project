// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_Push.generated.h"

/**
 * 밀치기 어빌리티 (공통). 총을 장착하지 않은 상태의 좌클릭.
 *
 * 애셋 태그에 Ability.Attack 을 함께 달아 두었으므로, 좌클릭(IA_Attack)이 이미 호출하는
 * TryActivateAbilitiesByTag(Ability.Attack) 하나로 총기 발사와 자동으로 갈린다:
 *   - 총 장착 중  → GA_FireGun 만 발동 (GA_Push 는 State.Equip.Gun 이 Blocked)
 *   - 총 미장착   → GA_Push 만 발동   (GA_FireGun 은 State.Equip.Gun 이 Required)
 *
 * 맞은 상대는 비틀거림 몽타주 + 넉백을 받고, 그 동안 이동 입력이 잠긴다.
 * 넉백에 밀려 열차 밖으로 떨어지면 기존 낙사 로직이 사망 처리하며,
 * 민 사람이 킬러로 기록된다(ABaseCharacter::PushKillCreditWindow 안일 때).
 */
UCLASS()
class TEAM4PROJECT_API UGA_Push : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Push();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	// 밀치기 사거리 (cm)
	UPROPERTY(EditDefaultsOnly, Category = "Push")
	float PushRange = 200.f;

	// 판정 구체 반지름 (cm). 조준이 조금 빗나가도 잡히게 한다.
	UPROPERTY(EditDefaultsOnly, Category = "Push")
	float PushRadius = 45.f;

	// 수평 넉백 세기 (cm/s)
	UPROPERTY(EditDefaultsOnly, Category = "Push")
	float PushStrength = 900.f;

	// 수직 넉백 세기 (cm/s). 살짝 띄워야 난간 너머로 넘어간다.
	UPROPERTY(EditDefaultsOnly, Category = "Push")
	float PushUpStrength = 300.f;

	// 밀린 쪽이 비틀거리며 이동 입력을 잃는 시간 (초)
	UPROPERTY(EditDefaultsOnly, Category = "Push")
	float StumbleDuration = 1.2f;

	UPROPERTY(EditDefaultsOnly, Category = "Push")
	bool bDrawDebug = false;
};
