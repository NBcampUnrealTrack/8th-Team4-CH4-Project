// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/Role/GODCharacterStoker.h"
#include "Game/BaseGameplayTags.h"

AGODCharacterStoker::AGODCharacterStoker()
{
	CharacterTag = Character::Crew::Stoker;
}

float AGODCharacterStoker::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	// 열 데미지 소스 태그가 붙은 액터로부터의 피해는 완전 면역
	if (DamageCauser && DamageCauser->ActorHasTag(TEXT("DamageSource.Heat")))
	{
		return 0.f;
	}

	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

void AGODCharacterStoker::ForceClosePressureValve(AActor* PressureValveActor)
{
	if (!HasAuthority() || !PressureValveActor) return;

	// 압력 밸브 액터의 강제 차단 인터페이스 호출.
	// 밸브 액터 구현 시 Execute_ForceClose 또는 컴포넌트 함수로 교체할 것.
	PressureValveActor->Tags.AddUnique(TEXT("Stoker.ForceClose"));
}
