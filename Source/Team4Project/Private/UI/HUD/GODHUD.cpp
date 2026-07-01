#include "UI/HUD/GODHUD.h"
#include "UI/HUD/GODMainHUDWidget.h"
#include "Blueprint/UserWidget.h"

void AGODHUD::BeginPlay()
{
	Super::BeginPlay();

	if (!MainHUDWidgetClass) return;

	APlayerController* PC = GetOwningPlayerController();
	if (!PC || !PC->IsLocalController()) return;

	MainHUDWidget = CreateWidget<UGODMainHUDWidget>(PC, MainHUDWidgetClass);
	if (MainHUDWidget)
	{
		MainHUDWidget->AddToViewport(0);
	}
}

void AGODHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (MainHUDWidget)
	{
		MainHUDWidget->RemoveFromParent();
		MainHUDWidget = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void AGODHUD::ShowMainHUD()
{
	if (MainHUDWidget)
		MainHUDWidget->SetVisibility(ESlateVisibility::Visible);
}

void AGODHUD::HideMainHUD()
{
	if (MainHUDWidget)
		MainHUDWidget->SetVisibility(ESlateVisibility::Hidden);
}
