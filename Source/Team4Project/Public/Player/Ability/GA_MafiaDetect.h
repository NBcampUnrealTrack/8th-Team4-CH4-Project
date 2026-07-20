// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_MafiaDetect.generated.h"

class UNiagaraSystem;

/**
 * 마피아 감별 사격. 라인트레이스로 맞춘 대상이 마피아(Character.Crew.Mafia)면 대상 위치에 이펙트를 띄운다.
 * 마피아가 아니거나 빗나가면 아무 일도 없다.
 */
UCLASS()
class TEAM4PROJECT_API UGA_MafiaDetect : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_MafiaDetect();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	// 근접 수색 반경(cm). 1/2/R 슬롯 발동 시 이 반경 안의 최근접 인원을 감별한다.
	// InteractComponent 가 있으면 그쪽 InteractRadius 를 우선 사용한다.
	UPROPERTY(EditDefaultsOnly, Category = "Detect")
	float SearchRadius = 300.f;

	// 디버그 표시 (개발용)
	UPROPERTY(EditDefaultsOnly, Category = "Detect")
	bool bDrawDebug = false;

	// 대상이 마피아일 때 띄울 이펙트
	UPROPERTY(EditDefaultsOnly, Category = "Detect|FX")
	TObjectPtr<UNiagaraSystem> DetectEffect;
};
