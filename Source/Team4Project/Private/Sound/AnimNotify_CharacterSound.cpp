// Fill out your copyright notice in the Description page of Project Settings.

#include "Sound/AnimNotify_CharacterSound.h"
#include "Components/SkeletalMeshComponent.h"
#include "Player/BaseCharacter.h"

void UAnimNotify_CharacterSound::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	ABaseCharacter* Character = MeshComp ? Cast<ABaseCharacter>(MeshComp->GetOwner()) : nullptr;
	if (!Character || Character->IsDead())
	{
		return;
	}

	Character->PlayCharacterSoundLocal(RowName);
}

FString UAnimNotify_CharacterSound::GetNotifyName_Implementation() const
{
	return RowName.ToString();
}
