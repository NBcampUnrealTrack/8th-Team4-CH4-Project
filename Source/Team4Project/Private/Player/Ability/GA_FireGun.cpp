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
	ActivationRequiredTags.AddTag(State::Weapon::EquipGun.GetTag());

	// 쿨다운/비용 GE 는 BP 에서 CooldownGameplayEffectClass / CostGameplayEffectClass 에 지정한다.
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

	if (Character->HasAuthority())
	{
		ABaseWeapon* Weapon = Character->GetCurrentWeapon();

		const FRotator AimRot = Character->GetBaseAimRotation();
		const FVector Start = Weapon ? Weapon->GetMuzzleLocation() : Character->GetActorLocation();
		const FVector End = Start + AimRot.Vector() * Range;

		FCollisionQueryParams Params;
		Params.AddIgnoredActor(Character);
		if (Weapon)
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
					Target->Die(Character);
				}
			}
		}

		// 머즐 + 명중 임팩트 이펙트 (서버 → 전 클라 재생). 발사가 성사된 경우에만 도달.
		if (Weapon)
		{
			Weapon->Multicast_PlayFireFX(
				bHit,
				bHit ? Hit.ImpactPoint : FVector::ZeroVector,
				bHit ? Hit.ImpactNormal : FVector::UpVector);
		}

		if (bDrawDebug)
		{
			DrawDebugLine(GetWorld(), Start, bHit ? Hit.ImpactPoint : End,
				FColor::Red, false, 1.0f, 0, 1.0f);
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
