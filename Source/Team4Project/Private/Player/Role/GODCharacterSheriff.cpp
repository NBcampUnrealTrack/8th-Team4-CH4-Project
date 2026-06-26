// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/Role/GODCharacterSheriff.h"
#include "InteractiveProp/DoorBase.h"
#include "Game/BaseGameplayTags.h"
#include "Kismet/KismetSystemLibrary.h"

AGODCharacterSheriff::AGODCharacterSheriff()
{
	CharacterTag = Character::Special::Sheriff;
}

void AGODCharacterSheriff::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(BodyDetectionTimer, this,
			&AGODCharacterSheriff::CheckForHiddenBodies,
			BodyDetectionInterval, true);
	}
}

void AGODCharacterSheriff::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(BodyDetectionTimer);
	Super::EndPlay(EndPlayReason);
}

// ── 패시브: 소멸 시체 감지 ──

void AGODCharacterSheriff::CheckForHiddenBodies()
{
	TArray<AActor*> Overlapping;
	UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		GetActorLocation(),
		BodyDetectionRadius,
		TArray<TEnumAsByte<EObjectTypeQuery>>{ UEngineTypes::ConvertToObjectType(ECC_Pawn) },
		ABaseCharacter::StaticClass(),
		TArray<AActor*>{ this },
		Overlapping
	);

	for (AActor* Actor : Overlapping)
	{
		ABaseCharacter* DeadChar = Cast<ABaseCharacter>(Actor);
		if (DeadChar && DeadChar->IsDead() && DeadChar->ActorHasTag(TEXT("Body.Hidden")))
		{
			OnHiddenBodyDetected(DeadChar);
		}
	}
}

// ── 액티브 B: 문 따기 ──

void AGODCharacterSheriff::UnlockDoor(AActor* DoorActor)
{
	if (!HasAuthority() || !IsValid(DoorActor)) return;

	if (ADoorBase* Door = Cast<ADoorBase>(DoorActor))
	{
		Door->SetLocked(false);
	}
}
