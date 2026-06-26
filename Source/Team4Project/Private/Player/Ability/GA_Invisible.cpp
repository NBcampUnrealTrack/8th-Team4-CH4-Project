// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/Ability/GA_Invisible.h"
#include "Player/BaseCharacter.h"
#include "Game/BaseGameplayTags.h"
#include "TimerManager.h"
#include "Engine/World.h"

UGA_Invisible::UGA_Invisible()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	// 가시성 변경은 서버 권위로 처리(복제로 클라 반영).
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	// 활성 동안 Ability.Invisible 태그 부여 → 공격 등이 이 태그를 ActivationBlockedTags 로 차단.
	ActivationOwnedTags.AddTag(Abilities::Invisible.GetTag());
}

void UGA_Invisible::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// 쿨다운/비용 GE 가 BP 에 지정돼 있으면 함께 커밋.
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ABaseCharacter* Character = ActorInfo ? Cast<ABaseCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (!Character || !Character->HasAuthority())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Character->SetInvisibleForDuration(InvisibleDuration);

	// 지속시간 동안 어빌리티를 활성 유지(이 동안 Ability.Invisible 보유 → 공격 차단), 만료 시 종료.
	UWorld* World = GetWorld();
	if (World && InvisibleDuration > 0.f)
	{
		World->GetTimerManager().SetTimer(
			DurationTimerHandle, this, &UGA_Invisible::OnDurationEnd, InvisibleDuration, false);
	}
	else
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

void UGA_Invisible::OnDurationEnd()
{
	// 지속 종료 → 어빌리티 종료(ActivationOwnedTags 해제 → 공격 다시 가능).
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_Invisible::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DurationTimerHandle);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
