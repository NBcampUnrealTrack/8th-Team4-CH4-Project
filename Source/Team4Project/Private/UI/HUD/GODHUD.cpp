#include "UI/HUD/GODHUD.h"
#include "UI/HUD/GODMainHUDWidget.h"
#include "UI/HUD/GearQTEWidget.h"
#include "UI/HUD/PressureMinigameWidget.h"
#include "UI/HUD/QuestMinigameWidget.h"
#include "Quest/QuestStation.h"
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

void AGODHUD::ShowGearQTE(AGearSlot* Slot)
{
	if (!GearQTEWidgetClass) return;

	if (!GearQTEWidget)
	{
		APlayerController* PC = GetOwningPlayerController();
		if (!PC) return;

		GearQTEWidget = CreateWidget<UGearQTEWidget>(PC, GearQTEWidgetClass);
		if (!GearQTEWidget) return;

		GearQTEWidget->AddToViewport(10);
	}

	GearQTEWidget->SetTargetSlot(Slot);
	GearQTEWidget->SetVisibility(ESlateVisibility::Visible);
}

void AGODHUD::HideGearQTE(bool bSuccess)
{
	if (!GearQTEWidget) return;

	GearQTEWidget->OnQTEFinished(bSuccess);
	GearQTEWidget->SetVisibility(ESlateVisibility::Hidden);
}

void AGODHUD::ShowPressureMinigame(APressureValve* Valve)
{
	if (!PressureMinigameWidgetClass) return;

	if (!PressureMinigameWidget)
	{
		APlayerController* PC = GetOwningPlayerController();
		if (!PC) return;

		PressureMinigameWidget = CreateWidget<UPressureMinigameWidget>(PC, PressureMinigameWidgetClass);
		if (!PressureMinigameWidget) return;

		PressureMinigameWidget->AddToViewport(10);
	}

	PressureMinigameWidget->SetTargetValve(Valve);
	PressureMinigameWidget->SetVisibility(ESlateVisibility::Visible);
}

void AGODHUD::HidePressureMinigame(bool bSuccess)
{
	if (!PressureMinigameWidget) return;

	PressureMinigameWidget->OnMinigameFinished(bSuccess);
	PressureMinigameWidget->SetVisibility(ESlateVisibility::Hidden);
}

void AGODHUD::ShowQuestMinigame(AQuestStation* Station)
{
	if (!Station) return;

	const TSubclassOf<UQuestMinigameWidget> WidgetClass = Station->GetWidgetClass();
	if (!WidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Quest] %s 의 DT 행에 WidgetClass 가 없어 팝업을 띄울 수 없다 (QuestId=%s)"),
			*Station->GetName(), *Station->QuestId.ToString());
		return;
	}

	// 퀘스트마다 위젯 클래스가 다르므로 재사용하지 않고 매번 만들고 버린다.
	if (QuestMinigameWidget)
	{
		QuestMinigameWidget->RemoveFromParent();
		QuestMinigameWidget = nullptr;
	}

	APlayerController* PC = GetOwningPlayerController();
	if (!PC) return;

	QuestMinigameWidget = CreateWidget<UQuestMinigameWidget>(PC, WidgetClass);
	if (!QuestMinigameWidget) return;

	QuestMinigameWidget->AddToViewport(10);
	QuestMinigameWidget->SetStation(Station);
}

void AGODHUD::HideQuestMinigame(bool bSuccess)
{
	if (!QuestMinigameWidget) return;

	QuestMinigameWidget->OnQuestFinished(bSuccess);
	QuestMinigameWidget->RemoveFromParent();
	QuestMinigameWidget = nullptr;
}
