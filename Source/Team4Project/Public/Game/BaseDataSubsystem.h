// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/DataTable.h"
#include "Game/BaseGameDataTypes.h"
#include "BaseDataSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class TEAM4PROJECT_API UBaseDataSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	UBaseDataSubsystem();
	
	const FCharacterAttributeRow* GetCharacterAttributeRow(FGameplayTag CharacterTag) const;
	
protected:
	static const FString CharacterAttributeTablePath;

	UPROPERTY()
	TObjectPtr<UDataTable> CharacterAttributeTable;
};
