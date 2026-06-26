// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/Ability/GA_EquipGun.h"
#include "Player/BaseCharacter.h"
#include "Player/Weapon/BaseWeapon.h"
#include "Game/BaseGameplayTags.h"
#include "AbilitySystemComponent.h"

UGA_EquipGun::UGA_EquipGun()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	FGameplayTagContainer Tags;
	Tags.AddTag(Abilities::Equip.GetTag());
	SetAssetTags(Tags);

	// 투명화(Ability.Invisible) 중에는 장착 불가. 투명화가 풀리면 다시 장착 가능.
	ActivationBlockedTags.AddTag(Abilities::Invisible.GetTag());
}

void UGA_EquipGun::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ABaseCharacter* Character = ActorInfo ? Cast<ABaseCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;

	if (!Character || !Character->HasAuthority() || !ASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const FGameplayTag EquippedTag = State::Weapon::EquipGun.GetTag();

	// 토글: 이미 장착돼 있으면 -> 장착 해제 (무기 제거 + 태그 제거)
	if (ASC->HasMatchingGameplayTag(EquippedTag))
	{
		if (ABaseWeapon* EquippedWeapon = Character->GetCurrentWeapon())
		{
			EquippedWeapon->Destroy();
		}
		Character->SetCurrentWeapon(nullptr);

		ASC->RemoveLooseGameplayTag(EquippedTag);
		ASC->RemoveReplicatedLooseGameplayTag(EquippedTag);

		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// 여기부터 장착 처리 (WeaponClass 필요)
	if (!WeaponClass)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 단일 슬롯: 총을 들기 전에 들고 있던 물리 아이템(석탄/기어)을 떨군다.
	// (이 분기는 총이 아직 미장착 상태이므로 ClearEquipSlot 의 총 해제 경로는 동작하지 않음.)
	Character->ClearEquipSlot();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Character;
	SpawnParams.Instigator = Character;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ABaseWeapon* Weapon = GetWorld()->SpawnActor<ABaseWeapon>(WeaponClass, Character->GetActorTransform(), SpawnParams);
	if (Weapon)
	{
		Weapon->AttachToComponent(
			Character->GetMesh(),
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			AttachSocketName);

		Character->SetCurrentWeapon(Weapon);

		// 서버 OwnedTag 즉시 반영 (ServerOnly 어빌리티 조건 검사용 - 서버에서 검사함)
		ASC->AddLooseGameplayTag(EquippedTag);
		// 클라이언트까지 복제 (HUD 등에서 장착 상태 확인용)
		ASC->AddReplicatedLooseGameplayTag(EquippedTag);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
