// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/Ability/GA_UnlockDoor.h"
#include "Player/Role/GODCharacterSheriff.h"
#include "Game/BaseGameplayTags.h"

UGA_UnlockDoor::UGA_UnlockDoor()
{
	InstancingPolicy  = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	ActivationRequiredTags.AddTag(Character::Special::Sheriff.GetTag());
}

void UGA_UnlockDoor::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	AGODCharacterSheriff* Sheriff = ActorInfo
		? Cast<AGODCharacterSheriff>(ActorInfo->AvatarActor.Get()) : nullptr;

	AActor* DoorActor = TriggerEventData
		? const_cast<AActor*>(Cast<AActor>(TriggerEventData->OptionalObject.Get())) : nullptr;

	if (!Sheriff || !IsValid(DoorActor) || !DoorActor->ActorHasTag(TEXT("Door.Locked")))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Sheriff->UnlockDoor(DoorActor);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
