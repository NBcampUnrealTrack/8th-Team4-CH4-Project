// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/Ability/GA_ToggleLantern.h"
#include "Player/BaseCharacter.h"
#include "Game/BaseGameplayTags.h"
#include "Sound/GameSoundTypes.h"

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

	Watchman->ToggleLantern();
	// 랜턴 빛이 본인에게만 보이듯, 토글음도 본인만 듣는다 (소리로 순찰자 노출 방지).
	Watchman->Client_PlayCharacterSound(SoundRows::AbilityLantern);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
