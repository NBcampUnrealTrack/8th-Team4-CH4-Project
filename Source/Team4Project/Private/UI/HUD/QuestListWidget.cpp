#include "UI/HUD/QuestListWidget.h"
#include "Player/GODPlayerState.h"
#include "Game/GODGameState.h"
#include "Quest/QuestStation.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

void UQuestListWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!TryBind())
	{
		GetWorld()->GetTimerManager().SetTimer(
			BindRetryTimer, this, &UQuestListWidget::RetryBind, 0.5f, /*bLoop=*/true);
	}
}

void UQuestListWidget::NativeDestruct()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(BindRetryTimer);
	}

	if (APlayerController* PC = GetOwningPlayer())
	{
		if (AGODPlayerState* PS = PC->GetPlayerState<AGODPlayerState>())
		{
			PS->OnAssignedQuestsChanged.RemoveAll(this);
		}
	}
	if (AGODGameState* GS = GetWorld() ? GetWorld()->GetGameState<AGODGameState>() : nullptr)
	{
		GS->OnQuestProgressChanged.RemoveAll(this);
	}

	Super::NativeDestruct();
}

bool UQuestListWidget::TryBind()
{
	if (bBound) return true;

	APlayerController* PC = GetOwningPlayer();
	AGODPlayerState* PS = PC ? PC->GetPlayerState<AGODPlayerState>() : nullptr;
	AGODGameState* GS = GetWorld() ? GetWorld()->GetGameState<AGODGameState>() : nullptr;

	if (!PS || !GS) return false;

	PS->OnAssignedQuestsChanged.AddDynamic(this, &UQuestListWidget::HandleAssignedQuestsChanged);
	GS->OnQuestProgressChanged.AddDynamic(this, &UQuestListWidget::HandleQuestProgressChanged);
	bBound = true;

	HandleAssignedQuestsChanged();
	HandleQuestProgressChanged(GS->QuestSpeedMultiplier, GS->QuestCompletedCitizens, GS->QuestTotalCitizens);
	return true;
}

void UQuestListWidget::RetryBind()
{
	if (TryBind())
	{
		GetWorld()->GetTimerManager().ClearTimer(BindRetryTimer);
	}
}

void UQuestListWidget::HandleAssignedQuestsChanged()
{
	APlayerController* PC = GetOwningPlayer();
	AGODPlayerState* PS = PC ? PC->GetPlayerState<AGODPlayerState>() : nullptr;
	if (!PS) return;

	TArray<FQuestListEntry> Entries;
	Entries.Reserve(PS->AssignedQuests.Num());

	for (const FAssignedQuest& Quest : PS->AssignedQuests)
	{
		// 스테이션 액터가 아직 복제되지 않았으면 이름을 못 읽는다. 다음 갱신에서 채워진다.
		if (!IsValid(Quest.Station)) continue;

		FQuestListEntry Entry;
		Entry.DisplayName = Quest.Station->GetDisplayName();
		Entry.LocationHint = Quest.Station->GetLocationHint();
		Entry.bCompleted = Quest.bCompleted;
		Entries.Add(Entry);
	}

	OnQuestListUpdated(Entries, PS->GetCompletedQuestCount(), PS->AssignedQuests.Num());
}

void UQuestListWidget::HandleQuestProgressChanged(float SpeedMultiplier, int32 CompletedCitizens, int32 TotalCitizens)
{
	OnTeamProgressUpdated(SpeedMultiplier, CompletedCitizens, TotalCitizens);
}
