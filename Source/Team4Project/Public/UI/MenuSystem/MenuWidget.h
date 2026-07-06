// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MenuInterface.h"
#include "MenuWidget.generated.h"

class UDataTable;
class UAudioComponent;

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

	// ============================================================
	// UI 사운드 — UI 사운드 DT (버튼/BGM. 행 이름은 SoundRows 참조)
	// ============================================================

	// 버튼 클릭/호버 등 단발 UI 사운드 재생 (행 이름 예: UI.Click / UI.Hover).
	UFUNCTION(BlueprintCallable, Category = "Sound")
	void PlayUISound(FName RowName);

	// BGM 시작 (행 이름 예: BGM.MainMenu). 이미 재생 중인 BGM 이 있으면 정지 후 교체.
	// 루프는 사운드 애셋에서 설정한다.
	UFUNCTION(BlueprintCallable, Category = "Sound")
	UAudioComponent* PlayUIMusic(FName RowName);

	UFUNCTION(BlueprintCallable, Category = "Sound")
	void StopUIMusic();

	// 버튼 공용 사운드 콜백 (BindButtonSounds 가 OnClicked/OnHovered 에 바인딩).
	UFUNCTION()
	void PlayClickSound();

	UFUNCTION()
	void PlayHoverSound();

protected:
	IMenuInterface* MenuInterface;

	// 버튼에 클릭/호버 사운드를 바인딩 (null 이면 무시 — Optional 위젯 안전).
	void BindButtonSounds(class UButton* Button);

	// UI 사운드 DT. 위젯 BP 디폴트에서 이것 하나만 지정.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<UDataTable> UISoundTable;

	// 현재 재생 중인 BGM (교체/정지용).
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> ActiveMusic;
};
