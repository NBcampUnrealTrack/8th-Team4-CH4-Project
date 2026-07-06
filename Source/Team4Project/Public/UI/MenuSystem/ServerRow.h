// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ServerRow.generated.h"

/**
 * 
 */
UCLASS()
class TEAM4PROJECT_API UServerRow : public UUserWidget
{
	GENERATED_BODY()
public:

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* ServerName;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* HostUser;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* ConnectionFraction;

	// 비밀번호 방 자물쇠 표시 (이미지/텍스트 등 아무 위젯). WBP에 있으면 자동 연결.
	UPROPERTY(meta = (BindWidgetOptional))
	class UWidget* LockIcon;

	UPROPERTY(BlueprintReadOnly)
	bool Selected = false;

	void Setup(class UMainMenu* Parent, uint32 Index);

private:
	UPROPERTY(meta = (BindWidget))
	class UButton* RowButton;

	UPROPERTY()
	class UMainMenu* Parent;

	uint32 Index;

	UFUNCTION()
	void OnClicked();
};
