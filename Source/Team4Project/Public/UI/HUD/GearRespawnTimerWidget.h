#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GearRespawnTimerWidget.generated.h"

class AGearSlot;
class UProgressBar;
class UTextBlock;

/**
 * 기어 슬롯 위 3D 리스폰 카운트다운 위젯 (UWidgetComponent, World Space).
 * GODAbilitySlotWidget 의 쿨다운 표시와 동일한 방식 — BindWidget 으로 직접 갱신하므로
 * WBP 쪽에 이벤트 그래프 배선이 필요 없다. WBP_GearRespawnTimer 에 아래 이름으로
 * 위젯만 배치하면 된다(Optional이라 없어도 컴파일/동작에는 지장 없음):
 *   PB_Respawn      — 잔여시간 프로그레스바 (1→0: 대기 시작→리스폰)
 *   TB_RespawnTime  — 잔여시간 초 텍스트
 */
UCLASS()
class TEAM4PROJECT_API UGearRespawnTimerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetTargetSlot(AGearSlot* InSlot);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> PB_Respawn;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_RespawnTime;

private:
	UPROPERTY()
	TWeakObjectPtr<AGearSlot> TargetSlot;
};
