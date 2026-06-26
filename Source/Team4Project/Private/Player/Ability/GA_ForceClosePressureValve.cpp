// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/Ability/GA_ForceClosePressureValve.h"
#include "Player/Role/GODCharacterStoker.h"
#include "Component/InteractComponent.h"
#include "InteractiveProp/PressureValve.h"
#include "Game/BaseGameplayTags.h"

UGA_ForceClosePressureValve::UGA_ForceClosePressureValve()
{
	InstancingPolicy  = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	ActivationRequiredTags.AddTag(Character::Crew::Stoker.GetTag());
}

void UGA_ForceClosePressureValve::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	AGODCharacterStoker* Stoker = ActorInfo
		? Cast<AGODCharacterStoker>(ActorInfo->AvatarActor.Get()) : nullptr;

	// 능력 버튼으로 직접 발동 시 InteractComponent에서 가장 가까운 밸브를 탐색
	AActor* ValveActor = nullptr;
	if (Stoker && Stoker->InteractComponent)
	{
		ValveActor = Stoker->InteractComponent->GetClosestInteractableOfClass(APressureValve::StaticClass());
	}

	if (!Stoker || !IsValid(ValveActor))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Stoker->ForceClosePressureValve(ValveActor);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
