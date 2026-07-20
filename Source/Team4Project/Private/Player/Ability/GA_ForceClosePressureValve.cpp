// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/Ability/GA_ForceClosePressureValve.h"
#include "Player/BaseCharacter.h"
#include "Component/InteractComponent.h"
#include "InteractiveProp/PressureValve.h"
#include "Game/BaseGameplayTags.h"
#include "Sound/GameSoundTypes.h"
#include "EngineUtils.h"

namespace
{
	// 절단기(GA_WireCutter)의 FindNearestAssembledGearSlot 과 동일한 폴백 패턴.
	// InteractComponent 오버랩 목록에 밸브가 등록 안 됐을 때(오버랩 감지 불안정) 쓰는,
	// 콜리전과 무관한 반경 내 최근접 밸브 탐색.
	APressureValve* FindNearestValve(const ABaseCharacter* Stoker, float Radius)
	{
		APressureValve* Best = nullptr;
		float BestDistSq = Radius * Radius;
		const FVector Origin = Stoker->GetActorLocation();

		for (TActorIterator<APressureValve> It(Stoker->GetWorld()); It; ++It)
		{
			APressureValve* Valve = *It;
			if (!IsValid(Valve)) continue;

			const float DistSq = FVector::DistSquared(Origin, Valve->GetActorLocation());
			if (DistSq <= BestDistSq)
			{
				BestDistSq = DistSq;
				Best = Valve;
			}
		}
		return Best;
	}
}

UGA_ForceClosePressureValve::UGA_ForceClosePressureValve()
{
	InstancingPolicy  = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	ActivationRequiredTags.AddTag(Character::Crew::Stoker.GetTag());
	// 긴급 회의 중 사용 불가.
	ActivationBlockedTags.AddTag(State::Meeting.GetTag());
}

void UGA_ForceClosePressureValve::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ABaseCharacter* Stoker = ActorInfo
		? Cast<ABaseCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;

	if (!Stoker)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 1순위: F키 상호작용과 같은 대상(InteractComponent 최근접). 오버랩 등록이 안 돼 있으면 null.
	AActor* ValveActor = nullptr;
	float SearchRadius = 200.f;
	if (Stoker->InteractComponent)
	{
		SearchRadius = Stoker->InteractComponent->InteractRadius;
		ValveActor = Stoker->InteractComponent->GetClosestInteractableOfClass(APressureValve::StaticClass());
	}

	// 2순위: 콜리전과 무관한 거리 검사 폴백 (절단기와 동일). 오버랩 감지가 불안정해
	// 밸브 근처에서도 능력이 조용히 실패하던 문제 보정.
	if (!IsValid(ValveActor))
	{
		ValveActor = FindNearestValve(Stoker, SearchRadius);
	}

	if (!IsValid(ValveActor))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[ForceCloseValve] 반경 %.0f 안에 밸브가 없어 발동 취소 (%s)"),
			SearchRadius, *Stoker->GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Stoker->ForceClosePressureValve(ValveActor);
	Stoker->Client_PlayCharacterSound(SoundRows::AbilityForceValve);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
