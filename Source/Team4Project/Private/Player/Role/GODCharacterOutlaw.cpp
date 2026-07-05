// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/Role/GODCharacterOutlaw.h"
#include "Game/BaseGameplayTags.h"

AGODCharacterOutlaw::AGODCharacterOutlaw(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	CharacterTag = Character::Special::Outlaw;
}
