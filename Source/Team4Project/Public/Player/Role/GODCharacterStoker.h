// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Player/BaseCharacter.h"
#include "GODCharacterStoker.generated.h"

/**
 * 화부 (Stoker) — 엔지니어 진영
 * 방열복으로 증기 유출/과열 데미지 면역, 압력 밸브 강제 차단 가능.
 */
UCLASS()
class TEAM4PROJECT_API AGODCharacterStoker : public ABaseCharacter
{
	GENERATED_BODY()

public:
	AGODCharacterStoker();

	// 열/증기 데미지 면역 여부 (PressureValve, SteamPipe 대미지 판정에서 참조)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Stoker")
	bool IsHeatImmune() const { return true; }

	// 압력 밸브를 즉시 안전 수치(50)로 강제 차단 (GA에서 호출)
	UFUNCTION(BlueprintCallable, Category = "Stoker")
	void ForceClosePressureValve(AActor* PressureValveActor);

protected:
	// TakeDamage 재정의 — GameplayTag "Damage.Heat" 타입은 0으로 무효화
	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;
};
