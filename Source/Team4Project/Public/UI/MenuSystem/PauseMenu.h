#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "PauseMenu.generated.h"

UCLASS()
class TEAM4PROJECT_API UPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 열릴 때 / 닫힐 때 애니메이션 — WBP 가 구현. 컨트롤러(TogglePauseMenu)가 호출.
	UFUNCTION(BlueprintImplementableEvent, Category = "Animation")
	void PlaySlideIn();

	UFUNCTION(BlueprintImplementableEvent, Category = "Animation")
	void PlaySlideOut();

	// 슬라이드 아웃 애니 길이(초). 이 시간 뒤 컨트롤러가 위젯을 제거한다. WBP 애니 길이에 맞춰 조정.
	float GetSlideOutDuration() const { return SlideOutDuration; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 슬라이드 아웃 애니 길이(초). WBP_PauseMenu 클래스 디폴트에서 조정.
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	float SlideOutDuration = 0.3f;

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