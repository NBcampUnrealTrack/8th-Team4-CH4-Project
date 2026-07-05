// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/Role/GODCharacterPorter.h"
#include "Game/BaseGameplayTags.h"

AGODCharacterPorter::AGODCharacterPorter(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	CharacterTag = Character::Crew::Porter;
}
