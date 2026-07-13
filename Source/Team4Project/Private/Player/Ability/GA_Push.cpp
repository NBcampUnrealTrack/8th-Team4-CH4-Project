// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/Ability/GA_Push.h"
#include "Player/BaseCharacter.h"
#include "Game/BaseGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "DrawDebugHelpers.h"

UGA_Push::UGA_Push()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	// 권위적 명중/넉백 판정을 위해 서버 실행 (GA_FireGun 과 동일).
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	FGameplayTagContainer Tags;
	Tags.AddTag(Abilities::Attack.GetTag());
	Tags.AddTag(Abilities::Push.GetTag());
	SetAssetTags(Tags);

	// 총을 들고 있으면 발사(GA_FireGun)만 나가야 하므로 밀치기는 차단.
	ActivationBlockedTags.AddTag(State::Weapon::EquipGun.GetTag());
	// 투명화 중에는 위치가 드러나므로 불가.
	ActivationBlockedTags.AddTag(Abilities::Invisible.GetTag());
	// 밀려서 비틀거리는 동안에는 반격 불가.
	ActivationBlockedTags.AddTag(State::Stumble.GetTag());
	// 긴급 회의 중 사용 불가.
	ActivationBlockedTags.AddTag(State::Meeting.GetTag());
}

void UGA_Push::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ABaseCharacter* Character = ActorInfo ? Cast<ABaseCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (!Character || !Character->HasAuthority() || Character->IsDead())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbilityCooldown(Handle, ActorInfo, ActivationInfo, /*ForceCooldown=*/false))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 밀치는 모션은 빗나가도 재생한다 (헛손질이 보여야 상대가 회피를 인지한다).
	Character->Multicast_PlayPushMontage();

	// 판정: 시야 방향으로 짧은 구체 스윕. 피치를 포함해야 계단 위/아래 상대도 잡힌다.
	const FVector Start = Character->GetActorLocation();
	const FVector AimDir = Character->GetBaseAimRotation().Vector();
	const FVector End = Start + AimDir * PushRange;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Character);

	FHitResult Hit;
	const bool bHit = GetWorld()->SweepSingleByChannel(
		Hit, Start, End, FQuat::Identity, ECC_Pawn,
		FCollisionShape::MakeSphere(PushRadius), Params);

	if (bDrawDebug)
	{
		DrawDebugLine(GetWorld(), Start, End, bHit ? FColor::Green : FColor::Silver, false, 1.f, 0, 1.f);
	}

	if (bHit)
	{
		ABaseCharacter* Target = Cast<ABaseCharacter>(Hit.GetActor());
		if (Target && Target != Character && !Target->IsDead())
		{
			// 수평 방향은 두 캐릭터의 위치차로 잡는다. 조준 벡터를 그대로 쓰면
			// 위를 보고 밀었을 때 상대가 수직으로만 떠올라 밖으로 나가지 않는다.
			FVector LaunchDir = (Target->GetActorLocation() - Start).GetSafeNormal2D();
			if (LaunchDir.IsNearlyZero())
			{
				LaunchDir = Character->GetActorForwardVector().GetSafeNormal2D();
			}

			const FVector LaunchVelocity = LaunchDir * PushStrength + FVector::UpVector * PushUpStrength;
			Target->ReceivePush(Character, LaunchVelocity, StumbleDuration);
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
