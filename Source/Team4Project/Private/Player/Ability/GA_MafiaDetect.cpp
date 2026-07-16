// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/Ability/GA_MafiaDetect.h"
#include "Player/BaseCharacter.h"
#include "Game/BaseGameplayTags.h"
#include "Sound/GameSoundTypes.h"
#include "DrawDebugHelpers.h"

UGA_MafiaDetect::UGA_MafiaDetect()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	// 권위적 명중 판정을 위해 서버 실행.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	// 긴급 회의 중 사용 불가.
	ActivationBlockedTags.AddTag(State::Meeting.GetTag());
}

void UGA_MafiaDetect::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// 쿨다운/비용 GE 가 BP 에 지정돼 있으면 함께 커밋.
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ABaseCharacter* Character = ActorInfo ? Cast<ABaseCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (!Character)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 감별 사용음 — 본인만 (감별 시도가 타인에게 들리면 보안관 위치/행동이 노출됨).
	Character->Client_PlayCharacterSound(SoundRows::AbilityDetect);

	if (Character->HasAuthority())
	{
		ABaseCharacter* Target = nullptr;

		// F 수색(캐릭터 인터랙트)으로 트리거된 경우 이벤트에 대상이 실려 온다 — 조준 불필요.
		// (BaseCharacter::Interact_Implementation 이 OptionalObject 에 자기 자신을 담아 보낸다)
		if (TriggerEventData && TriggerEventData->OptionalObject)
		{
			Target = Cast<ABaseCharacter>(const_cast<UObject*>(TriggerEventData->OptionalObject.Get()));
		}

		// 이벤트 대상이 없으면(슬롯 발동 등) 기존 시선(눈높이) 조준 트레이스로 폴백.
		if (!Target)
		{
			const FRotator AimRot = Character->GetBaseAimRotation();
			const FVector Start = Character->GetPawnViewLocation();
			const FVector End = Start + AimRot.Vector() * Range;

			FCollisionQueryParams Params;
			Params.AddIgnoredActor(Character);

			FHitResult Hit;
			const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Pawn, Params);
			if (bHit)
			{
				Target = Cast<ABaseCharacter>(Hit.GetActor());
			}

			if (bDrawDebug)
			{
				DrawDebugLine(GetWorld(), Start, bHit ? Hit.ImpactPoint : End,
					FColor::Green, false, 1.0f, 0, 1.0f);
			}
		}

		if (Target && Target != Character && !Target->IsDead())
		{
			const bool bIsMafia = Target->IsMafia();

			// 마피아면 대상 위치에 감별 이펙트 재생 (기존 동작 유지).
			if (bIsMafia)
			{
				Target->Multicast_PlayNiagaraAtSelf(DetectEffect);
			}

			// 성공/실패를 보안관 본인에게 통지 (New 2 — 수색 결과가 안 뜨던 문제).
			Character->Client_NotifySearchResult(bIsMafia, Target);
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
