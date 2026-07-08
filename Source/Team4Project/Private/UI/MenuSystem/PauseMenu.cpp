// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/MenuSystem/PauseMenu.h"
#include "Kismet/GameplayStatics.h"
#include "Player/BasePlayerController.h"


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
