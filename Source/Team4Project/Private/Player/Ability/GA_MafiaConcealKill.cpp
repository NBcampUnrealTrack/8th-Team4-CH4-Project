// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/Ability/GA_MafiaConcealKill.h"
#include "Player/BaseCharacter.h"
#include "Game/BaseGameplayTags.h"
#include "Sound/GameSoundTypes.h"

UGA_MafiaConcealKill::UGA_MafiaConcealKill()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	// 처치 이벤트(Event.Kill)가 오면 자동 발동.
	FAbilityTriggerData Trigger;
	Trigger.TriggerTag = Event::Kill.GetTag();
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(Trigger);
}

void UGA_MafiaConcealKill::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ABaseCharacter* Owner = ActorInfo ? Cast<ABaseCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;

	// 서버 + 마피아일 때만 시체 은폐.
	if (!Owner || !Owner->HasAuthority() || !Owner->IsMafia())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const AActor* VictimActor = TriggerEventData ? TriggerEventData->Target : nullptr;
	if (ABaseCharacter* Victim = const_cast<ABaseCharacter*>(Cast<ABaseCharacter>(VictimActor)))
	{
		Victim->HideCorpseForDuration(ConcealDuration);
		// 은폐 발동을 마피아 본인에게만 알린다 (타인이 들으면 은폐 사실이 노출됨).
		Owner->Client_PlayCharacterSound(SoundRows::AbilityConceal);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
