// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Player/BaseCharacter.h"
#include "GODCharacterPorter.generated.h"

/**
 * 짐꾼 (Porter) — 엔지니어 진영
 * 괴력 패시브: 50kg 이상의 무거운 기어를 들어도 이동 속도 감소 패널티 없음.
 */
UCLASS()
class TEAM4PROJECT_API AGODCharacterPorter : public ABaseCharacter
{
	GENERATED_BODY()

public:
	AGODCharacterPorter();

	// 무거운 물체 소지 시 이동 속도 감소 비율 반환 (Porter는 항상 0)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Porter")
	float GetHeavyCarrySpeedPenalty() const { return 0.f; }

	// 들어올릴 수 있는 최대 무게 (kg) — 물리 핸들 판정에서 참조
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Porter")
	float GetMaxCarryWeight() const { return MaxCarryWeight; }

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Porter")
	float MaxCarryWeight = 200.f;
};
