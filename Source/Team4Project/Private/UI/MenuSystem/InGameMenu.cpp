// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/MenuSystem/InGameMenu.h"
#include "Components/Button.h"
#include "Sound/GameSoundTypes.h"

bool UInGameMenu::Initialize()
{
	bool Success = Super::Initialize();
	if (!Success) return false;

	if (!ensure(CancelButton != nullptr))
		return false;
	CancelButton->OnClicked.AddDynamic(this, &UInGameMenu::CancelPressed);

	if (!ensure(QuitButton != nullptr))
		return false;
	QuitButton->OnClicked.AddDynamic(this, &UInGameMenu::QuitPressed);

	BindButtonSounds(CancelButton);
	BindButtonSounds(QuitButton);

	return true;
}

void UInGameMenu::NativeConstruct()
{
	Super::NativeConstruct();
	PlayUISound(SoundRows::UIMenuOpen);
}

void UInGameMenu::NativeDestruct()
{
	PlayUISound(SoundRows::UIMenuClose);
	Super::NativeDestruct();
}

FReply UInGameMenu::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		CancelPressed();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UInGameMenu::CancelPressed()
{
	Teardown();
}

void UInGameMenu::QuitPressed()
{
	if (MenuInterface != nullptr)
	{
		Teardown();
		MenuInterface->LoadMainMenu();
	}
}