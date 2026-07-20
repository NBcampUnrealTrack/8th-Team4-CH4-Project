// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/Ability/GA_MafiaDetect.h"
#include "Player/BaseCharacter.h"
#include "Component/InteractComponent.h"
#include "Game/BaseGameplayTags.h"
#include "Sound/GameSoundTypes.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"

namespace
{
	// 절단기(GA_WireCutter)의 최근접 탐색과 동일한 폴백 패턴 — 반경 내 살아있는 최근접 타인.
	ABaseCharacter* FindNearestOtherCharacter(const ABaseCharacter* Seeker, float Radius)
	{
		ABaseCharacter* Best = nullptr;
		float BestDistSq = Radius * Radius;
		const FVector Origin = Seeker->GetActorLocation();

		for (TActorIterator<ABaseCharacter> It(Seeker->GetWorld()); It; ++It)
		{
			ABaseCharacter* Other = *It;
			if (!IsValid(Other) || Other == Seeker || Other->IsDead()) continue;

			const float DistSq = FVector::DistSquared(Origin, Other->GetActorLocation());
			if (DistSq <= BestDistSq)
			{
				BestDistSq = DistSq;
				Best = Other;
			}
		}
		return Best;
	}
}

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

		// 1/2/R 슬롯 발동 등 이벤트 대상이 없으면, 밸브/절단기와 동일하게 "근처 최근접 인원"을
		// 감별한다(조준 불필요 — 근처에 서 있기만 하면 됨).
		if (!Target)
		{
			float Radius = SearchRadius;

			// 1순위: InteractComponent 근접 목록(F 상호작용과 동일한 대상 판정).
			if (Character->InteractComponent)
			{
				Radius = Character->InteractComponent->InteractRadius;
				Target = Cast<ABaseCharacter>(
					Character->InteractComponent->GetClosestInteractableOfClass(ABaseCharacter::StaticClass()));
			}

			// 2순위: 콜리전과 무관한 거리 검사 폴백.
			if (!Target)
			{
				Target = FindNearestOtherCharacter(Character, Radius);
			}

			if (bDrawDebug && Target)
			{
				DrawDebugLine(GetWorld(), Character->GetActorLocation(), Target->GetActorLocation(),
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
		else
		{
			// 근처에 수색할 사람이 없었음 — 스킬이 먹통인지 헷갈리지 않게 본인에게 통지.
			Character->Client_NotifySearchResult(false, nullptr);
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
