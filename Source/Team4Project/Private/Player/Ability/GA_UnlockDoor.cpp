// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/Ability/GA_UnlockDoor.h"
#include "Player/BaseCharacter.h"
#include "Component/InteractComponent.h"
#include "InteractiveProp/DoorBase.h"
#include "Game/BaseGameplayTags.h"
#include "Sound/GameSoundTypes.h"
#include "EngineUtils.h"

namespace
{
	ADoorBase* FindNearestLockedDoor(const ABaseCharacter* Sheriff, float Radius)
	{
		ADoorBase* Best = nullptr;
		float BestDistSq = Radius * Radius;
		const FVector Origin = Sheriff->GetActorLocation();

		for (TActorIterator<ADoorBase> It(Sheriff->GetWorld()); It; ++It)
		{
			ADoorBase* Door = *It;
			if (!IsValid(Door) || !Door->bIsLocked) continue;

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

UGA_UnlockDoor::UGA_UnlockDoor()
{
	InstancingPolicy  = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	ActivationRequiredTags.AddTag(Character::Special::Sheriff.GetTag());
	// 긴급 회의 중 사용 불가.
	ActivationBlockedTags.AddTag(State::Meeting.GetTag());
}

void UGA_UnlockDoor::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ABaseCharacter* Sheriff = ActorInfo
		? Cast<ABaseCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (!Sheriff)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 문 F 상호작용에서 분리 — MasterKey/WireCutter 와 동일하게 발동 시 근처의
	// '잠긴' 문을 스스로 찾는다. (HUD 슬롯 버튼 / 키보드 발동용)
	// 1순위: F키 상호작용과 같은 대상(InteractComponent 최근접). 잠긴 문이 아니면 폴백으로.
	ADoorBase* Door = nullptr;
	float SearchRadius = 200.f;
	if (Sheriff->InteractComponent)
	{
		SearchRadius = Sheriff->InteractComponent->InteractRadius;
		Door = Cast<ADoorBase>(
			Sheriff->InteractComponent->GetClosestInteractableOfClass(ADoorBase::StaticClass()));
	}

	// 2순위: 콜리전과 무관한 거리 검사 폴백. (잠긴 문만 대상)
	if (!Door || !Door->bIsLocked)
	{
		Door = FindNearestLockedDoor(Sheriff, SearchRadius);
	}

	if (!Door)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[UnlockDoor] 반경 %.0f 안에 잠긴 문이 없어 발동 취소 (%s)"),
			SearchRadius, *Sheriff->GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Sheriff->UnlockDoor(Door);
	Sheriff->Client_PlayCharacterSound(SoundRows::AbilityUnlockDoor);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
