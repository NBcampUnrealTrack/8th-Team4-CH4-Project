#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Game/MinigameTypes.h"
#include "GearQTEWidget.generated.h"

class AGearSlot;

/**
 * 기어 재조립 화살표 QTE 오버레이.
 * 실제 화살표 아이콘 20개 배치/하이라이트는 WBP_GearQTE(UMG 디자이너)에서
 * OnQTEDataUpdated 이벤트를 받아 구현한다.
 */
UCLASS()
class TEAM4PROJECT_API UGearQTEWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetTargetSlot(AGearSlot* InSlot);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// Sequence: 0~3 = EQTEDirection. ProgressIndex: 다음에 맞춰야 할 인덱스.
	// TimeRemainingFraction: 1(시작)→0(타임아웃).
	UFUNCTION(BlueprintImplementableEvent, Category = "Gear QTE")
	void OnQTEDataUpdated(const TArray<uint8>& Sequence, int32 ProgressIndex, float TimeRemainingFraction);

public:
	UFUNCTION(BlueprintImplementableEvent, Category = "Gear QTE")
	void OnQTEFinished(bool bSuccess);

private:
	UPROPERTY()
	TWeakObjectPtr<AGearSlot> TargetSlot;
};
