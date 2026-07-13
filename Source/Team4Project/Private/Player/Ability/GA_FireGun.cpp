// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/Ability/GA_FireGun.h"
#include "Player/BaseCharacter.h"
#include "Player/GODPlayerState.h"
#include "Player/Weapon/BaseWeapon.h"
#include "Player/Component/BaseAttributeSet.h"
#include "Game/BaseGameplayTags.h"
#include "Game/GODGameMode.h"
#include "Game/GODGameState.h"
#include "AbilitySystemComponent.h"
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

	// 투명화(Ability.Invisible) 중에는 발사 불가. 투명화가 풀리면 태그가 사라져 다시 발사 가능.
	ActivationBlockedTags.AddTag(Abilities::Invisible.GetTag());

	// 긴급 회의 중 사용 불가.
	ActivationBlockedTags.AddTag(State::Meeting.GetTag());

	// 쿨다운/비용 GE 는 BP 에서 CooldownGameplayEffectClass / CostGameplayEffectClass 에 지정한다.
}

void UGA_FireGun::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// 3분 발포 잠금 (기획: 게임 시작 3분 후 해제). 쿨다운 커밋 전에 검사해 쿨다운 낭비 방지.
	const AGODGameState* GS = GetWorld() ? GetWorld()->GetGameState<AGODGameState>() : nullptr;
	if (!GS || !GS->bGunsUnlocked)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 쿨다운만 커밋. 탄약은 아래에서 CurrentAmmo 어트리뷰트로 직접 검사/소모한다
	// (PlayerState.AmmoCount 와 이원화돼 있던 탄약을 어트리뷰트로 단일화).
	if (!CommitAbilityCooldown(Handle, ActorInfo, ActivationInfo, /*ForceCooldown=*/false))
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
		// 탄약 검사/소모 (서버 권위).
		UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
		const float Ammo = ASC ? ASC->GetNumericAttribute(UBaseAttributeSet::GetCurrentAmmoAttribute()) : 0.f;
		if (Ammo < 1.f)
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}
		ASC->ApplyModToAttribute(UBaseAttributeSet::GetCurrentAmmoAttribute(), EGameplayModOp::Additive, -1.f);

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
					// Die() 직접 호출이 아니라 게임모드 공통 사망 경로를 태운다
					// (승리 조건 체크 + 특수직 사살 시 탄약 리필까지 처리).
					AGODGameMode* GM = GetWorld()->GetAuthGameMode<AGODGameMode>();
					AGODPlayerState* VictimPS = Target->GetPlayerState<AGODPlayerState>();
					if (GM && VictimPS)
					{
						GM->HandlePlayerDeath(Character->GetPlayerState<AGODPlayerState>(), VictimPS);
					}
					else
					{
						Target->Die(Character);
					}
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
