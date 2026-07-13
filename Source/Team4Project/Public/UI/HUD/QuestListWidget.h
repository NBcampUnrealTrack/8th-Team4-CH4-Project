#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Quest/QuestTypes.h"
#include "QuestListWidget.generated.h"

/**
 * 좌상단 상시 퀘스트 목록. 자기 배정 목록(PlayerState, OwnerOnly)과
 * 전체 진행도(GameState)를 각각 델리게이트로 받아 WBP 에 넘김
 */
UCLASS()
class TEAM4PROJECT_API UQuestListWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 내 퀘스트 목록. Completed/Total 은 "내" 진행도(예: 1/3).
	UFUNCTION(BlueprintImplementableEvent, Category = "Quest")
	void OnQuestListUpdated(const TArray<FQuestListEntry>& Entries, int32 Completed, int32 Total);

	// 시민 전체 진행도와 현재 열차 속도 배율. 마피아에게도 보인다.
	UFUNCTION(BlueprintImplementableEvent, Category = "Quest")
	void OnTeamProgressUpdated(float SpeedMultiplier, int32 CompletedCitizens, int32 TotalCitizens);

private:
	UFUNCTION()
	void HandleAssignedQuestsChanged();

	UFUNCTION()
	void HandleQuestProgressChanged(float SpeedMultiplier, int32 CompletedCitizens, int32 TotalCitizens);

	bool TryBind();
	void RetryBind();

	FTimerHandle BindRetryTimer;
	bool bBound = false;
};
