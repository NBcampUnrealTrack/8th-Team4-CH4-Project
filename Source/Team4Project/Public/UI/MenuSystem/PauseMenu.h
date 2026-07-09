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

	UPROPERTY(meta = (BindWidget))
	UButton* Btn_Resume;

	UPROPERTY(meta = (BindWidget))
	UButton* Btn_Exit;

	UFUNCTION()
	void OnResumeClicked();

	UFUNCTION()
	void OnExitClicked();

	// ── 설정창 (WBP 에 아직 없어도 되도록 Optional) ──
	// 이 이름(SettingsButton)으로 버튼만 배치하면 자동으로 클릭 → 설정창 열림.
	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* SettingsButton;

	// 설정창 위젯 클래스 (WBP_SettingsMenu). WBP_PauseMenu 클래스 디폴트에서 지정.
	UPROPERTY(EditDefaultsOnly, Category = "Settings")
	TSubclassOf<class USettingsMenu> SettingsMenuClass;

	// 열려 있는 설정창 인스턴스 (중복 생성 방지).
	UPROPERTY()
	TObjectPtr<class USettingsMenu> SettingsMenuInstance;

	UFUNCTION()
	void OnSettingsClicked();
};