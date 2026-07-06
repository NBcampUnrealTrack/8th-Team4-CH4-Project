// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/MenuSystem/MenuWidget.h"
#include "Sound/GameSoundStatics.h"
#include "Sound/GameSoundTypes.h"
#include "Components/AudioComponent.h"
#include "Components/Button.h"

void UMenuWidget::SetMenuInterface(IMenuInterface* _MenuInterface)
{
	this->MenuInterface = _MenuInterface;
}

void UMenuWidget::PlayUISound(FName RowName)
{
	UGameSoundStatics::PlaySound2DFromTable(this, UISoundTable, RowName);
}

UAudioComponent* UMenuWidget::PlayUIMusic(FName RowName)
{
	StopUIMusic();
	ActiveMusic = UGameSoundStatics::SpawnSound2DFromTable(this, UISoundTable, RowName);
	return ActiveMusic;
}

void UMenuWidget::StopUIMusic()
{
	if (ActiveMusic)
	{
		ActiveMusic->Stop();
		ActiveMusic = nullptr;
	}
}

void UMenuWidget::PlayClickSound()
{
	PlayUISound(SoundRows::UIClick);
}

void UMenuWidget::PlayHoverSound()
{
	PlayUISound(SoundRows::UIHover);
}

void UMenuWidget::BindButtonSounds(UButton* Button)
{
	if (!Button)
	{
		return;
	}
	Button->OnClicked.AddDynamic(this, &UMenuWidget::PlayClickSound);
	Button->OnHovered.AddDynamic(this, &UMenuWidget::PlayHoverSound);
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