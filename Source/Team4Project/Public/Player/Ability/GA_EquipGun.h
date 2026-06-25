// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_EquipGun.generated.h"

class ABaseWeapon;

/**
 * 총기 장착 어빌리티 (공통). 서버에서 무기 액터를 스폰해 캐릭터에 부착하고
 * State.Weapon.GunEquipped 태그를 부여한다. -> Fire 어빌리티 발동 조건 충족.
 */
UCLASS()
class TEAM4PROJECT_API UGA_EquipGun : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_EquipGun();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	// 스폰할 무기 클래스 (BP_BaseWeapon 등)
	UPROPERTY(EditDefaultsOnly, Category = "Equip")
	TSubclassOf<ABaseWeapon> WeaponClass;

	// 캐릭터 메시에서 무기를 붙일 소켓
	UPROPERTY(EditDefaultsOnly, Category = "Equip")
	FName AttachSocketName = TEXT("WeaponSocket");
};
