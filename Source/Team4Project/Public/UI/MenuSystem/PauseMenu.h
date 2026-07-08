// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "PauseMenu.generated.h"

UCLASS()
class TEAM4PROJECT_API UPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	// 블프 디자이너 창의 버튼 이름과 정확히 일치시켜 줍니다.
	UPROPERTY(meta = (BindWidget))
	UButton* Btn_Resume;

	UPROPERTY(meta = (BindWidget))
	UButton* Btn_Exit;

	// 버튼이 눌렸을 때 실행될 내부 함수들
	UFUNCTION()
	void OnResumeClicked();

	UFUNCTION()
	void OnExitClicked();
};