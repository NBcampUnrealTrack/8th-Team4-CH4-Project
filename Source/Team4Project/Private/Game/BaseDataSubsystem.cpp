// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/BaseDataSubsystem.h"

const FString UBaseDataSubsystem::CharacterAttributeTablePath = TEXT("/Game/GameSystem/Data/DT_CharacterRow");

UBaseDataSubsystem::UBaseDataSubsystem()
{
	static ConstructorHelpers::FObjectFinder<UDataTable> DT_Attribute(*CharacterAttributeTablePath);
	if (DT_Attribute.Succeeded()) CharacterAttributeTable = DT_Attribute.Object;
	
}

const FCharacterAttributeRow* UBaseDataSubsystem::GetCharacterAttributeRow(FGameplayTag CharacterTag) const
{
	if (!CharacterAttributeTable || !CharacterTag.IsValid()) return nullptr;
	
	const FName RowName = CharacterTag.GetTagName();

	return CharacterAttributeTable->FindRow<FCharacterAttributeRow>(RowName, TEXT("AttributeContext"));
}

