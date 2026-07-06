// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/MenuSystem/MenuWidget.h"
#include "InGameMenu.generated.h"

/**
 * 
 */
UCLASS()
class TEAM4PROJECT_API UInGameMenu : public UMenuWidget
{
	GENERATED_BODY()
	
protected:
	virtual bool Initialize() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	// ESC 메뉴 열기/닫기 사운드
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

public:
	UPROPERTY(meta = (BindWidget))
	class UButton* CancelButton;

	UPROPERTY(meta = (BindWidget))
	class UButton* QuitButton;


	UFUNCTION()
	void CancelPressed();

	UFUNCTION()
	void QuitPressed();
};
