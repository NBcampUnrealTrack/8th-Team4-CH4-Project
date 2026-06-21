// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MenuInterface.h"
#include "MenuWidget.generated.h"

/**
 * 
 */
UCLASS()
class TEAM4PROJECT_API UMenuWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void Setup();
	void InSetup();
	void Teardown();

	void SetMenuInterface(IMenuInterface* _MenuInterface);

protected:
	IMenuInterface* MenuInterface;
};
