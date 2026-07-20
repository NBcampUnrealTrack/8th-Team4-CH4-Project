// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/MenuSystem/PauseMenu.h"
#include "Kismet/GameplayStatics.h"
#include "Player/BasePlayerController.h"
#include "UI/MenuSystem/SettingsMenu.h"
#include "UI/MenuSystem/MenuInterface.h"
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

	// 등장 애니메이션 재생 (WBP 에서 PlaySlideIn 구현 시). 미구현이면 no-op.
	PlaySlideIn();
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
	// 세션을 끊고 메인 메뉴(TitleMap)로 복귀. GameInstance 의 LoadMainMenu 가
	// DestroySession → TitleMap 이동까지 처리하고, 복귀 후 LoadMenuWidget 에서 검색 가드도 리셋된다.
	// (기존 OpenLevel("L_MainMenu") 은 존재하지 않는 맵 + 세션 미정리라 재참여/방목록이 깨졌었다.)
	UWorld* World = GetWorld();
	if (World)
	{
		if (IMenuInterface* MenuInterface = Cast<IMenuInterface>(World->GetGameInstance()))
		{
			MenuInterface->LoadMainMenu();
			return;
		}
	}

	// 폴백: 인터페이스를 못 찾으면 최소한 존재하는 타이틀 맵으로라도 이동.
	UGameplayStatics::OpenLevel(this, FName("TitleMap"));
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

void UPauseMenuWidget::NativeDestruct()
{
	// 💡 일시정지 메뉴가 닫힐 때, 레이어 20에 떠 있던 환경설정 창도 확실하게 화면에서 지워버립니다.
	if (SettingsMenuInstance && SettingsMenuInstance->IsInViewport())
	{
		SettingsMenuInstance->RemoveFromParent();
	}

	Super::NativeDestruct();
}