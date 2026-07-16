// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/Ability/GA_Push.h"
#include "Player/BaseCharacter.h"
#include "Game/BaseGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "DrawDebugHelpers.h"

UGA_Push::UGA_Push()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	// 권위적 명중/넉백 판정을 위해 서버 실행 (GA_FireGun 과 동일).
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	FGameplayTagContainer Tags;
	Tags.AddTag(Abilities::Attack.GetTag());
	Tags.AddTag(Abilities::Push.GetTag());
	SetAssetTags(Tags);

	// 투명화 중에는 위치가 드러나므로 불가.
	ActivationBlockedTags.AddTag(Abilities::Invisible.GetTag());
	// 밀려서 비틀거리는 동안에는 반격 불가.
	ActivationBlockedTags.AddTag(State::Stumble.GetTag());
	// 긴급 회의 중 사용 불가.
	ActivationBlockedTags.AddTag(State::Meeting.GetTag());
}

void UGA_Push::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ABaseCharacter* Character = ActorInfo ? Cast<ABaseCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (!Character || !Character->HasAuthority() || Character->IsDead())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbilityCooldown(Handle, ActorInfo, ActivationInfo, /*ForceCooldown=*/false))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 밀치는 모션은 빗나가도 재생한다 (헛손질이 보여야 상대가 회피를 인지한다).
	Character->Multicast_PlayPushMontage();

	// 판정: 시야 방향으로 짧은 구체 스윕. 피치를 포함해야 계단 위/아래 상대도 잡힌다.
	const FVector Start = Character->GetActorLocation();
	const FVector AimDir = Character->GetBaseAimRotation().Vector();
	const FVector End = Start + AimDir * PushRange;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Character);

	// SweepSingleByChannel(ECC_Pawn) 은 "첫 블로킹 히트"에서 멈춘다 — 열차 바닥/벽도
	// Pawn 채널을 블록하므로 조준이 조금만 아래로 향해도 바닥에 먹혀 판정이 실패했다.
	// 폰 오브젝트 타입만 멀티 스윕해 가장 가까운 캐릭터를 고르고, 벽 관통 방지는
	// 별도 가시선(Visibility) 트레이스로 확인한다.
	TArray<FHitResult> Hits;
	FCollisionObjectQueryParams ObjParams;
	ObjParams.AddObjectTypesToQuery(ECC_Pawn);
	GetWorld()->SweepMultiByObjectType(
		Hits, Start, End, FQuat::Identity, ObjParams,
		FCollisionShape::MakeSphere(PushRadius), Params);

	ABaseCharacter* Target = nullptr;
	for (const FHitResult& Hit : Hits)
	{
		ABaseCharacter* Candidate = Cast<ABaseCharacter>(Hit.GetActor());
		if (!Candidate || Candidate == Character || Candidate->IsDead()) continue;

		// 벽 너머 상대를 밀지 못하게 가시선 확인 (둘 다 무시하고 사이에 막힌 게 있는지만 본다).
		FCollisionQueryParams LOSParams;
		LOSParams.AddIgnoredActor(Character);
		LOSParams.AddIgnoredActor(Candidate);
		FHitResult LOSHit;
		const bool bBlocked = GetWorld()->LineTraceSingleByChannel(
			LOSHit, Start, Candidate->GetActorLocation(), ECC_Visibility, LOSParams);
		if (bBlocked) continue;

		Target = Candidate;
		break; // Hits 는 스윕 경로 순서라 첫 유효 대상이 가장 가깝다.
	}

	if (bDrawDebug)
	{
		DrawDebugLine(GetWorld(), Start, End, Target ? FColor::Green : FColor::Silver, false, 1.f, 0, 1.f);
	}

	if (Target)
	{
		// 수평 방향은 두 캐릭터의 위치차로 잡는다. 조준 벡터를 그대로 쓰면
		// 위를 보고 밀었을 때 상대가 수직으로만 떠올라 밖으로 나가지 않는다.
		FVector LaunchDir = (Target->GetActorLocation() - Start).GetSafeNormal2D();
		if (LaunchDir.IsNearlyZero())
		{
			LaunchDir = Character->GetActorForwardVector().GetSafeNormal2D();
		}

		// 전향한 무법자(이중스파이)는 슈퍼 넉백 — 전향 시 SetCharacterTag 로
		// ASC 에 Character.Special.Outlaw 루즈 태그가 들어오므로 그걸로 판정한다.
		// (위장 중에는 시민 직업 태그라 배수가 붙지 않는다)
		float Multiplier = 1.f;
		if (UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent())
		{
			if (ASC->HasMatchingGameplayTag(Character::Special::Outlaw.GetTag()))
			{
				Multiplier = TurnedOutlawKnockbackMultiplier;
			}
		}

		const FVector LaunchVelocity =
			LaunchDir * PushStrength * Multiplier + FVector::UpVector * PushUpStrength * Multiplier;
		Target->ReceivePush(Character, LaunchVelocity, StumbleDuration);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
