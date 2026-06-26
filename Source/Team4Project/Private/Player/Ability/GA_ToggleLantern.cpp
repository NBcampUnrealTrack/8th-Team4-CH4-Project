// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/Ability/GA_ToggleLantern.h"
#include "Player/Role/GODCharacterWatchman.h"
#include "Game/BaseGameplayTags.h"

UGA_ToggleLantern::UGA_ToggleLantern()
{
	InstancingPolicy  = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	ActivationRequiredTags.AddTag(Character::Crew::Watchman.GetTag());
}

void UGA_ToggleLantern::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	AGODCharacterWatchman* Watchman = ActorInfo
		? Cast<AGODCharacterWatchman>(ActorInfo->AvatarActor.Get()) : nullptr;

	if (!Watchman)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Watchman->ToggleLantern();
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
