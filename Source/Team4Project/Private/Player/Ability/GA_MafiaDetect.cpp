// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/Ability/GA_MafiaDetect.h"
#include "Player/BaseCharacter.h"
#include "Player/Weapon/BaseWeapon.h"
#include "DrawDebugHelpers.h"

UGA_MafiaDetect::UGA_MafiaDetect()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	// 권위적 명중 판정을 위해 서버 실행.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
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

	if (Character->HasAuthority())
	{
		ABaseWeapon* Weapon = Character->GetCurrentWeapon();

		const FRotator AimRot = Character->GetBaseAimRotation();
		const FVector Start = Weapon ? Weapon->GetMuzzleLocation() : Character->GetActorLocation();
		const FVector End = Start + AimRot.Vector() * Range;

		FCollisionQueryParams Params;
		Params.AddIgnoredActor(Character);
		if (Weapon)
		{
			Params.AddIgnoredActor(Weapon);
		}

		FHitResult Hit;
		const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Pawn, Params);

		if (bHit)
		{
			// 맞은 대상이 마피아면 그 대상 위치에 감별 이펙트 재생.
			if (ABaseCharacter* Target = Cast<ABaseCharacter>(Hit.GetActor()))
			{
				if (Target->IsMafia())
				{
					Target->Multicast_PlayNiagaraAtSelf(DetectEffect);
				}
			}
		}

		if (bDrawDebug)
		{
			DrawDebugLine(GetWorld(), Start, bHit ? Hit.ImpactPoint : End,
				FColor::Green, false, 1.0f, 0, 1.0f);
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
