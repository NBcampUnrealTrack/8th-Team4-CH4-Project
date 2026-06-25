// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/Ability/GA_FireGun.h"
#include "Player/BaseCharacter.h"
#include "Player/Weapon/BaseWeapon.h"
#include "Game/BaseGameplayTags.h"
#include "DrawDebugHelpers.h"

UGA_FireGun::UGA_FireGun()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	// 입력 연결 전 단계 + 권위적 명중 판정을 위해 서버 실행
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	FGameplayTagContainer Tags;
	Tags.AddTag(Abilities::Attack.GetTag());
	SetAssetTags(Tags);

	// 총기 장착 상태에서만 발동
	ActivationRequiredTags.AddTag(State::Weapon::GunEquipped.GetTag());
}

void UGA_FireGun::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// 쿨다운 + 탄약 비용(Cost GE) 커밋. 실패 시(쿨다운 중/탄약 없음) 종료.
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ABaseCharacter* Character = ActorInfo ? Cast<ABaseCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (!Character)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 명중 판정/사망 처리는 서버에서만
	if (Character->HasAuthority())
	{
		const FRotator AimRot = Character->GetBaseAimRotation();

		FVector Start = Character->GetActorLocation();
		if (ABaseWeapon* Weapon = Character->GetCurrentWeapon())
		{
			Start = Weapon->GetMuzzleLocation();
		}
		const FVector End = Start + AimRot.Vector() * Range;

		FCollisionQueryParams Params;
		Params.AddIgnoredActor(Character);
		if (ABaseWeapon* Weapon = Character->GetCurrentWeapon())
		{
			Params.AddIgnoredActor(Weapon);
		}

		FHitResult Hit;
		const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Pawn, Params);

		if (bHit)
		{
			if (ABaseCharacter* Target = Cast<ABaseCharacter>(Hit.GetActor()))
			{
				if (!Target->IsDead())
				{
					// 맞으면 즉시 사망 (데미지 없음 - 마피아 룰)
					Target->Die(Character);
				}
			}
		}

		if (bDrawDebug)
		{
			DrawDebugLine(GetWorld(), Start, bHit ? Hit.ImpactPoint : End,
				FColor::Red, false, 1.0f, 0, 1.0f);
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
