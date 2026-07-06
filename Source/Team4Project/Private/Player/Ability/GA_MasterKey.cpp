// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/Ability/GA_MasterKey.h"
#include "Player/BaseCharacter.h"
#include "Game/BaseGameplayTags.h"
#include "Sound/GameSoundTypes.h"

UGA_MasterKey::UGA_MasterKey()
{
	InstancingPolicy  = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	ActivationRequiredTags.AddTag(Character::Special::Mafia.GetTag());
	// BP 에서 CooldownGameplayEffectClass = GE_Cooldown_MasterKey (Duration 60s) 지정
}

void UGA_MasterKey::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ABaseCharacter* Mafia = ActorInfo
		? Cast<ABaseCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;

	AActor* DoorActor = TriggerEventData
		? const_cast<AActor*>(Cast<AActor>(TriggerEventData->OptionalObject.Get())) : nullptr;

	if (!Mafia || !IsValid(DoorActor))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Mafia->UseMasterKey(DoorActor);
	Mafia->Client_PlayCharacterSound(SoundRows::AbilityMasterKey);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
