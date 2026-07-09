// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/MenuSystem/InGameMenu.h"
#include "UI/MenuSystem/SettingsMenu.h"
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

	if (SettingsButton) SettingsButton->OnClicked.AddDynamic(this, &UInGameMenu::OpenSettings);

	BindButtonSounds(CancelButton);
	BindButtonSounds(QuitButton);
	BindButtonSounds(SettingsButton);

	return true;
}

void UInGameMenu::OpenSettings()
{
	if (!SettingsMenuClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Settings] SettingsMenuClass 미지정 — WBP_InGameMenu 에 WBP_SettingsMenu 를 지정하세요."));
		return;
	}
	if (SettingsMenuInstance && SettingsMenuInstance->IsInViewport())
	{
		return;
	}
	SettingsMenuInstance = CreateWidget<USettingsMenu>(GetOwningPlayer(), SettingsMenuClass);
	if (SettingsMenuInstance)
	{
		SettingsMenuInstance->SetMenuInterface(MenuInterface);
		SettingsMenuInstance->AddToViewport(10);
	}
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