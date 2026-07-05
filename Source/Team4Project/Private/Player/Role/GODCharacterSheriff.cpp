// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/Role/GODCharacterSheriff.h"
#include "Game/BaseGameplayTags.h"

AGODCharacterSheriff::AGODCharacterSheriff(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	CharacterTag = Character::Special::Sheriff;
}
