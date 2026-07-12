#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "QuestTypes.generated.h"

class UQuestMinigameWidget;
class AQuestStation;

/**
 * 퀘스트 정의 (DT_Quest). 행 이름 = AQuestStation::QuestId.
 * 같은 퀘스트를 열차 여러 곳에 배치하려면, 스테이션을 여러 개 놓고 같은 QuestId 를 지정한다.
 */
USTRUCT(BlueprintType)
struct FQuestRow : public FTableRowBase
{
	GENERATED_BODY()

	// 퀘스트 목록 UI에 표시할 이름 ("설거지")
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FText DisplayName;

	// 위치 힌트 ("식당칸"). 목록 UI 부제목.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FText LocationHint;

	// 상호작용 시 띄울 미니게임 팝업 위젯 (WBP_Quest_Dishes 등)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	TSubclassOf<UQuestMinigameWidget> WidgetClass;

	// 최소 소요 시간(초). 미니게임 로직은 클라 위젯에 있으므로, 서버는 이 시간이
	// 지나기 전에 도착한 완료 보고를 거부한다. 즉시완료 매크로 방지용 하한선.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	float MinDuration = 5.f;
};

/** 플레이어에게 배정된 퀘스트 한 칸 (스테이션 인스턴스 단위). */
USTRUCT(BlueprintType)
struct FAssignedQuest
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Quest")
	TObjectPtr<AQuestStation> Station = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Quest")
	bool bCompleted = false;
};

/** 좌상단 목록 위젯에 넘기는 표시용 항목. */
USTRUCT(BlueprintType)
struct FQuestListEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Quest")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Quest")
	FText LocationHint;

	UPROPERTY(BlueprintReadOnly, Category = "Quest")
	bool bCompleted = false;
};
