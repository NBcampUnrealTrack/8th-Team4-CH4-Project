// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/Role/GODCharacterMechanic.h"
#include "Game/BaseGameplayTags.h"

AGODCharacterMechanic::AGODCharacterMechanic(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	CharacterTag = Character::Crew::Mechanic;
}
