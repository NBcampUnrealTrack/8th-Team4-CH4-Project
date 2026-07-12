#include "UI/HUD/QuestMinigameWidget.h"
#include "Quest/QuestStation.h"
#include "Player/BaseCharacter.h"
#include "Input/Reply.h"

void UQuestMinigameWidget::SetStation(AQuestStation* InStation)
{
	Station = InStation;

	// 키 입력(ESC)을 받으려면 포커스가 있어야 한다.
	SetKeyboardFocus();

	OnQuestStarted(InStation ? InStation->GetDisplayName() : FText::GetEmpty());
}

void UQuestMinigameWidget::CompleteQuest()
{
	if (ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwningPlayerPawn()))
	{
		Character->Server_CompleteQuest();
	}
}

void UQuestMinigameWidget::CancelQuest()
{
	if (ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwningPlayerPawn()))
	{
		Character->Server_CancelQuest();
	}
}

FReply UQuestMinigameWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		CancelQuest();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}
