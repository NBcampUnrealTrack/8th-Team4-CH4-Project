// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/Role/GODCharacterWatchman.h"
#include "Game/BaseGameplayTags.h"

AGODCharacterWatchman::AGODCharacterWatchman(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	CharacterTag = Character::Crew::Watchman;
}
