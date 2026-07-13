// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/Ability/GA_StopFakeDeath.h"
#include "Player/BaseCharacter.h"
#include "Game/BaseGameplayTags.h"

UGA_StopFakeDeath::UGA_StopFakeDeath()
{
	InstancingPolicy  = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	ActivationRequiredTags.AddTag(Character::Special::Outlaw.GetTag());
	// 긴급 회의 중 사용 불가
	ActivationBlockedTags.AddTag(State::Meeting.GetTag());
}

void UGA_StopFakeDeath::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ABaseCharacter* Outlaw = ActorInfo
		? Cast<ABaseCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;

	if (!Outlaw || !Outlaw->IsFakeDead())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 해제는 쿨타임 소비 없음: CommitAbility 생략
	Outlaw->StopFakeDeath();
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
