// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/Ability/GA_EnterVent.h"
#include "Player/BaseCharacter.h"
#include "Game/BaseGameplayTags.h"
#include "Sound/GameSoundTypes.h"

UGA_EnterVent::UGA_EnterVent()
{
	InstancingPolicy  = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	ActivationRequiredTags.AddTag(Character::Special::Mafia.GetTag());
	// 긴급 회의 중 사용 불가.
	ActivationBlockedTags.AddTag(State::Meeting.GetTag());
}

void UGA_EnterVent::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ABaseCharacter* Mafia = ActorInfo
		? Cast<ABaseCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;

	if (!Mafia)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (Mafia->IsInVent())
	{
		// 퇴장: 쿨타임 소비 없이 즉시 나감
		Mafia->ExitVent();
		Mafia->Client_PlayCharacterSound(SoundRows::AbilityVent);
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
		Mafia->Client_PlayCharacterSound(SoundRows::AbilityVent);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
