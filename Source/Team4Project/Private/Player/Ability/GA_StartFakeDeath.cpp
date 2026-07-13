// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/Ability/GA_StartFakeDeath.h"
#include "Player/BaseCharacter.h"
#include "Game/BaseGameplayTags.h"
#include "Sound/GameSoundTypes.h"

UGA_StartFakeDeath::UGA_StartFakeDeath()
{
	InstancingPolicy  = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	ActivationRequiredTags.AddTag(Character::Special::Outlaw.GetTag());
	// 긴급 회의 중 사용 불가.
	ActivationBlockedTags.AddTag(State::Meeting.GetTag());
	// BP 에서 CooldownGameplayEffectClass = GE_Cooldown_FakeDeath (Duration 30s) 지정
}

void UGA_StartFakeDeath::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ABaseCharacter* Outlaw = ActorInfo
		? Cast<ABaseCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;

	// 이미 죽은 척 중이거나 실제 사망 상태면 발동 불가
	if (!Outlaw || Outlaw->IsFakeDead() || Outlaw->IsDead())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Outlaw->StartFakeDeath();
	// 죽은 척은 유일하게 전 클라 재생 — 진짜 사망(Multicast_HandleDeath)과 같은 행(Character.Die)을
	// 재생해 소리로 구분되지 않게 한다.
	Outlaw->Multicast_PlayCharacterSound(SoundRows::CharacterDie);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
