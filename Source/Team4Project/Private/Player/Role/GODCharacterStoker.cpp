// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/Role/GODCharacterStoker.h"
#include "Game/BaseGameplayTags.h"

AGODCharacterStoker::AGODCharacterStoker(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	CharacterTag = Character::Crew::Stoker;
}
