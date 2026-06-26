// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/Ability/GA_StealAmmo.h"
#include "Player/Role/GODCharacterOutlaw.h"
#include "Player/BaseCharacter.h"
#include "Game/BaseGameplayTags.h"

UGA_StealAmmo::UGA_StealAmmo()
{
	InstancingPolicy  = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	ActivationRequiredTags.AddTag(Character::Special::Outlaw.GetTag());
}

void UGA_StealAmmo::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	AGODCharacterOutlaw* Outlaw = ActorInfo
		? Cast<AGODCharacterOutlaw>(ActorInfo->AvatarActor.Get()) : nullptr;

	ABaseCharacter* DeadCharacter = TriggerEventData
		? const_cast<ABaseCharacter*>(Cast<ABaseCharacter>(TriggerEventData->OptionalObject.Get())) : nullptr;

	if (!Outlaw || !IsValid(DeadCharacter) || !DeadCharacter->IsDead())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Outlaw->StealAmmo(DeadCharacter);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
