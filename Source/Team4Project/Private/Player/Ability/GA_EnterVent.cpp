// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/Ability/GA_EnterVent.h"
#include "Player/Role/GODCharacterMafia.h"
#include "Game/BaseGameplayTags.h"

UGA_EnterVent::UGA_EnterVent()
{
	InstancingPolicy  = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	ActivationRequiredTags.AddTag(Character::Special::Mafia.GetTag());
}

void UGA_EnterVent::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	AGODCharacterMafia* Mafia = ActorInfo
		? Cast<AGODCharacterMafia>(ActorInfo->AvatarActor.Get()) : nullptr;

	if (!Mafia)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (Mafia->IsInVent())
	{
		// 퇴장: 쿨타임 소비 없이 즉시 나감
		Mafia->ExitVent();
	}
	else
	{
		// 진입: 비용/쿨타임 커밋 (BP에서 GE 미지정 시 쿨타임 없음)
		if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}

		AActor* VentActor = TriggerEventData
			? const_cast<AActor*>(Cast<AActor>(TriggerEventData->OptionalObject.Get())) : nullptr;

		Mafia->EnterVent(VentActor);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
