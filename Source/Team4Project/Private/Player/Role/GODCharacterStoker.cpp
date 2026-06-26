// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/Role/GODCharacterStoker.h"
#include "InteractiveProp/PressureValve.h"
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

	if (APressureValve* Valve = Cast<APressureValve>(PressureValveActor))
	{
		Valve->Server_StopTurning();
		Valve->Tags.AddUnique(TEXT("Stoker.ForceClose")); // 이후 일반 조작 차단용
	}
}
