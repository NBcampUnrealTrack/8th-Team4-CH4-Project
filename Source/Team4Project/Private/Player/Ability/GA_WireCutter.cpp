// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/Ability/GA_WireCutter.h"
#include "Player/Role/GODCharacterMafia.h"
#include "Component/InteractComponent.h"
#include "InteractiveProp/PickupGear.h"
#include "Game/BaseGameplayTags.h"

UGA_WireCutter::UGA_WireCutter()
{
	InstancingPolicy  = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	ActivationRequiredTags.AddTag(Character::Special::Mafia.GetTag());
	// BP 에서 CooldownGameplayEffectClass = GE_Cooldown_WireCutter (Duration 120s) 지정
}

void UGA_WireCutter::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	AGODCharacterMafia* Mafia = ActorInfo
		? Cast<AGODCharacterMafia>(ActorInfo->AvatarActor.Get()) : nullptr;

	// 능력 버튼으로 직접 발동 시 InteractComponent에서 가장 가까운 기어를 탐색
	AActor* GearActor = nullptr;
	if (Mafia && Mafia->InteractComponent)
	{
		GearActor = Mafia->InteractComponent->GetClosestInteractableOfClass(APickupGear::StaticClass());
	}

	if (!Mafia || !IsValid(GearActor))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Mafia->UseWireCutter(GearActor);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
