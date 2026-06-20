// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/MenuSystem/MenuWidget.h"

void UMenuWidget::SetMenuInterface(IMenuInterface* _MenuInterface)
{
	this->MenuInterface = _MenuInterface;
}

void UMenuWidget::Setup()
{
	this->SetIsFocusable(true);
	this->AddToViewport();

	UWorld* World = GetWorld();
	if (!ensure(World != nullptr))
		return;

	APlayerController* PC = World->GetFirstPlayerController();
	if (!ensure(PC != nullptr))
		return;

	FInputModeUIOnly InputModeData;
	InputModeData.SetWidgetToFocus(this->TakeWidget());
	InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

	PC->SetInputMode(InputModeData);
	PC->bShowMouseCursor = true;
}

void UMenuWidget::InSetup()
{
	this->SetIsFocusable(true);
	this->AddToViewport();

	UWorld* World = GetWorld();
	if (!ensure(World != nullptr))
		return;
}

void UMenuWidget::Teardown()
{
	this->RemoveFromParent();

	UWorld* World = GetWorld();
	if (!ensure(World != nullptr))	return;

	APlayerController* PC = World->GetFirstPlayerController();
	if (PC != nullptr)
	{
		FInputModeGameOnly InputModeData;
		PC->SetInputMode(InputModeData);
		PC->bShowMouseCursor = false;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Teardown 호출 시점에 PlayerController가 nullptr입니다."));
	}
}