#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "QuestMinigameWidget.generated.h"

class AQuestStation;

/**
 * 퀘스트 미니게임 팝업의 베이스. 
 */
UCLASS()
class TEAM4PROJECT_API UQuestMinigameWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetStation(AQuestStation* InStation);

	// 미니게임을 다 풀었을 때 WBP 가 호출. 서버로 완료를 보고한다.
	UFUNCTION(BlueprintCallable, Category = "Quest Minigame")
	void CompleteQuest();

	// 중도 포기 (ESC / 닫기 버튼).
	UFUNCTION(BlueprintCallable, Category = "Quest Minigame")
	void CancelQuest();

	// 팝업이 열릴 때. WBP 가 여기서 퍼즐을 초기화한다.
	UFUNCTION(BlueprintImplementableEvent, Category = "Quest Minigame")
	void OnQuestStarted(const FText& DisplayName);

	// 서버가 완료/중단을 확정했을 때.
	UFUNCTION(BlueprintImplementableEvent, Category = "Quest Minigame")
	void OnQuestFinished(bool bSuccess);

	UFUNCTION(BlueprintPure, Category = "Quest Minigame")
	AQuestStation* GetStation() const { return Station.Get(); }

protected:
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	TWeakObjectPtr<AQuestStation> Station;
};
