// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/Ability/GA_StartFakeDeath.h"
#include "Player/Role/GODCharacterOutlaw.h"
#include "Game/BaseGameplayTags.h"

UGA_StartFakeDeath::UGA_StartFakeDeath()
{
	InstancingPolicy  = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	ActivationRequiredTags.AddTag(Character::Special::Outlaw.GetTag());
	// BP 에서 CooldownGameplayEffectClass = GE_Cooldown_FakeDeath (Duration 30s) 지정
}

void UGA_StartFakeDeath::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	AGODCharacterOutlaw* Outlaw = ActorInfo
		? Cast<AGODCharacterOutlaw>(ActorInfo->AvatarActor.Get()) : nullptr;

	// 이미 죽은 척 중이거나 실제 사망 상태면 발동 불가
	if (!Outlaw || Outlaw->IsFakeDead() || Outlaw->IsDead())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Outlaw->StartFakeDeath();
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
