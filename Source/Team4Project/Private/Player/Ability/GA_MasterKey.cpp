// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/Ability/GA_MasterKey.h"
#include "Player/BaseCharacter.h"
#include "Component/InteractComponent.h"
#include "InteractiveProp/DoorBase.h"
#include "Game/BaseGameplayTags.h"
#include "Sound/GameSoundTypes.h"
#include "EngineUtils.h"

namespace
{
	ADoorBase* FindNearestUnlockedDoor(const ABaseCharacter* Mafia, float Radius)
	{
		ADoorBase* Best = nullptr;
		float BestDistSq = Radius * Radius;
		const FVector Origin = Mafia->GetActorLocation();

		for (TActorIterator<ADoorBase> It(Mafia->GetWorld()); It; ++It)
		{
			ADoorBase* Door = *It;
			if (!IsValid(Door) || Door->bIsLocked) continue;

			const float DistSq = FVector::DistSquared(Origin, Door->GetActorLocation());
			if (DistSq <= BestDistSq)
			{
				BestDistSq = DistSq;
				Best = Door;
			}
		}
		return Best;
	}
}

UGA_MasterKey::UGA_MasterKey()
{
	InstancingPolicy  = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	ActivationRequiredTags.AddTag(Character::Special::Mafia.GetTag());
	// 긴급 회의 중 사용 불가.
	ActivationBlockedTags.AddTag(State::Meeting.GetTag());
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
	if (!Mafia)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 문 F 상호작용에서 분리 — WireCutter 와 동일하게 발동 시 근처 문을 스스로 찾는다.
	// (HUD 슬롯 버튼 / 키보드 발동용. 대상이 없으면 쿨타임 없이 취소)
	// 1순위: F키 상호작용과 같은 대상(InteractComponent 최근접). 오버랩 등록이 안 돼 있으면 null.
	ADoorBase* Door = nullptr;
	float SearchRadius = 200.f;
	if (Mafia->InteractComponent)
	{
		SearchRadius = Mafia->InteractComponent->InteractRadius;
		Door = Cast<ADoorBase>(
			Mafia->InteractComponent->GetClosestInteractableOfClass(ADoorBase::StaticClass()));
	}

	// 2순위: 콜리전과 무관한 거리 검사 폴백. (이미 잠긴 문은 대상에서 제외)
	if (!Door || Door->bIsLocked)
	{
		Door = FindNearestUnlockedDoor(Mafia, SearchRadius);
	}

	if (!Door)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[MasterKey] 반경 %.0f 안에 잠글 수 있는 문이 없어 발동 취소 (%s)"),
			SearchRadius, *Mafia->GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Mafia->UseMasterKey(Door);
	Mafia->Client_PlayCharacterSound(SoundRows::AbilityMasterKey);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
