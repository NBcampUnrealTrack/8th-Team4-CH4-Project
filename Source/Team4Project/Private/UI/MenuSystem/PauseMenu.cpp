// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/MenuSystem/PauseMenu.h"
#include "Kismet/GameplayStatics.h"
#include "Player/BasePlayerController.h"
#include "UI/MenuSystem/SettingsMenu.h"
#include "Blueprint/UserWidget.h"


void UPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// C++에서 버튼에 클릭 이벤트 묶어주기 (다이내믹 바인딩)
	if (Btn_Resume)
	{
		Btn_Resume->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnResumeClicked);
	}
	if (Btn_Exit)
	{
		Btn_Exit->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnExitClicked);
	}
	// 설정 버튼 — WBP 에 SettingsButton 이 있을 때만 바인딩 (Optional)
	if (SettingsButton)
	{
		SettingsButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnSettingsClicked);
	}
}

void UPauseMenuWidget::OnResumeClicked()
{
	// 내 컨트롤러를 가져와서 일시정지 메뉴를 토글(닫기)시킵니다.
	if (ABasePlayerController* PC = Cast<ABasePlayerController>(GetOwningPlayer()))
	{
		PC->TogglePauseMenu();
	}
}

void UPauseMenuWidget::OnExitClicked()
{
	// 오픈월드나 로비 등 메인 메뉴 레벨로 세션을 끊고 이동시킵니다.
	// "L_MainMenu" 자리에 본인의 메인타이틀 맵 이름을 넣으시면 됩니다.
	UGameplayStatics::OpenLevel(this, FName("L_MainMenu"));
}

void UPauseMenuWidget::OnSettingsClicked()
{
	if (!SettingsMenuClass)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[PauseMenu] SettingsMenuClass 미지정 — WBP_PauseMenu 클래스 디폴트에 WBP_SettingsMenu 를 지정하세요."));
		return;
	}
	// 이미 떠 있으면 중복 생성 방지.
	if (SettingsMenuInstance && SettingsMenuInstance->IsInViewport())
	{
		return;
	}
	SettingsMenuInstance = CreateWidget<USettingsMenu>(GetOwningPlayer(), SettingsMenuClass);
	if (SettingsMenuInstance)
	{
		// 일시정지 메뉴 위에 오버레이. 입력/포커스/닫기는 설정창이 자체 처리.
		SettingsMenuInstance->AddToViewport(20);
	}
}
