// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Player/BaseCharacter.h"
#include "GODCharacterOutlaw.generated.h"

/**
 * 무법자 (Outlaw) — 특수 진영
 *
 * 가짜 시체: GA_StartFakeDeath → StartFakeDeath / GA_StopFakeDeath → StopFakeDeath
 * 도굴꾼: GA_StealAmmo → StealAmmo (사망자 탄약 탈취)
 */
UCLASS()
class TEAM4PROJECT_API AGODCharacterOutlaw : public ABaseCharacter
{
	GENERATED_BODY()

public:
	AGODCharacterOutlaw();

	virtual void BeginPlay() override;

	// ── 가짜 시체 (GA_StartFakeDeath / GA_StopFakeDeath 에서 호출) ──

	void StartFakeDeath();
	void StopFakeDeath();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Outlaw")
	bool IsFakeDead() const { return bIsFakeDead; }

	// ── 도굴꾼 (GA_StealAmmo 에서 호출) ──

	void StealAmmo(ABaseCharacter* DeadCharacter);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UPROPERTY(ReplicatedUsing = OnRep_IsFakeDead)
	bool bIsFakeDead = false;

	UFUNCTION()
	void OnRep_IsFakeDead();

	FVector DefaultMeshRelativeLocation = FVector::ZeroVector;
	FRotator DefaultMeshRelativeRotation = FRotator::ZeroRotator;

	void ApplyFakeDeathPhysics(bool bActivate);
};
