// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/Ability/GA_FootprintVision.h"
#include "Player/BaseCharacter.h"
#include "Game/BaseGameplayTags.h"
#include "Sound/GameSoundTypes.h"

UGA_FootprintVision::UGA_FootprintVision()
{
	InstancingPolicy  = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	ActivationRequiredTags.AddTag(Character::Crew::Watchman.GetTag());
}

void UGA_FootprintVision::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ABaseCharacter* Watchman = ActorInfo
		? Cast<ABaseCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;

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

	Watchman->ActivateFootprintVision();
	Watchman->Client_PlayCharacterSound(SoundRows::AbilityFootprintVision);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
