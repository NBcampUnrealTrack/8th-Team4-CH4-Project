// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Player/BaseCharacter.h"
#include "GODCharacterMechanic.generated.h"

/**
 * 정비공 (Mechanic) — 엔지니어 진영
 * 수리 속도 2배 패시브, 튕겨나간 기어를 핀 1회 삽입으로 즉시 고정하는 특수 상호작용.
 */
UCLASS()
class TEAM4PROJECT_API AGODCharacterMechanic : public ABaseCharacter
{
	GENERATED_BODY()

public:
	AGODCharacterMechanic();

	// 이 캐릭터가 기어를 축에 삽입할 때 핀 삽입 횟수를 1회로 단축하는지 여부
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mechanic")
	bool CanInstantFixGear() const { return true; }

	// 수리 인터랙션 코드에서 참조 (배율 기본값 2배)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mechanic")
	float GetRepairSpeedMultiplier() const { return RepairSpeedMultiplier; }

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mechanic")
	float RepairSpeedMultiplier = 2.0f;
};
