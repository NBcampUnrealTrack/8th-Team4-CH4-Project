// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/Ability/GA_WireCutter.h"
#include "Player/BaseCharacter.h"
#include "Component/InteractComponent.h"
#include "InteractiveProp/GearSlot.h"
#include "Game/BaseGameplayTags.h"
#include "Sound/GameSoundTypes.h"
#include "EngineUtils.h"

namespace
{
	AGearSlot* FindNearestAssembledGearSlot(const ABaseCharacter* Mafia, float Radius)
	{
		AGearSlot* Best = nullptr;
		float BestDistSq = Radius * Radius;
		const FVector Origin = Mafia->GetActorLocation();

		for (TActorIterator<AGearSlot> It(Mafia->GetWorld()); It; ++It)
		{
			AGearSlot* Slot = *It;
			if (!IsValid(Slot) || !Slot->bIsAssembled) continue;

			const float DistSq = FVector::DistSquared(Origin, Slot->GetActorLocation());
			if (DistSq <= BestDistSq)
			{
				BestDistSq = DistSq;
				Best = Slot;
			}
		}
		return Best;
	}
}

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
	ABaseCharacter* Mafia = ActorInfo
		? Cast<ABaseCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (!Mafia)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 1순위: F키 상호작용과 같은 대상(InteractComponent 최근접). 오버랩 등록이 안 돼 있으면 null.
	AGearSlot* GearSlot = nullptr;
	float SearchRadius = 200.f;
	if (Mafia->InteractComponent)
	{
		SearchRadius = Mafia->InteractComponent->InteractRadius;
		GearSlot = Cast<AGearSlot>(
			Mafia->InteractComponent->GetClosestInteractableOfClass(AGearSlot::StaticClass()));
	}

	// 2순위: 콜리전과 무관한 거리 검사 폴백.
	if (!GearSlot || !GearSlot->bIsAssembled)
	{
		GearSlot = FindNearestAssembledGearSlot(Mafia, SearchRadius);
	}

	if (!GearSlot)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[WireCutter] 반경 %.0f 안에 조립된 기어 슬롯이 없어 발동 취소 (%s)"),
			SearchRadius, *Mafia->GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Mafia->UseWireCutter(GearSlot);
	// 절단기 사용음은 본인만. 기어 파손음(Gear.Break)은 GearSlot 이 전 클라에 따로 낸다.
	Mafia->Client_PlayCharacterSound(SoundRows::AbilityWireCutter);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
