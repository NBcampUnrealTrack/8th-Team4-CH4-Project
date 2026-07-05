// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/Role/GODCharacterMafia.h"
#include "Game/BaseGameplayTags.h"

AGODCharacterMafia::AGODCharacterMafia(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	CharacterTag = Character::Special::Mafia;
}
