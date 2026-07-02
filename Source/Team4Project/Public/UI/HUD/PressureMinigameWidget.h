#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PressureMinigameWidget.generated.h"

class APressureValve;

/**
 * 압력 밸브 바늘-정지 미니게임 오버레이.
 * 바늘 게이지/빨간 구간 표시는 WBP_PressureMinigame(UMG 디자이너)에서
 * OnMinigameDataUpdated 이벤트를 받아 구현한다.
 */
UCLASS()
class TEAM4PROJECT_API UPressureMinigameWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetTargetValve(APressureValve* InValve);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// NeedlePos/RedZoneStart/RedZoneEnd: 0~1 정규화된 위치.
	UFUNCTION(BlueprintImplementableEvent, Category = "Pressure Minigame")
	void OnMinigameDataUpdated(float NeedlePos, float RedZoneStart, float RedZoneEnd, int32 SuccessCount, int32 MissCount);

public:
	UFUNCTION(BlueprintImplementableEvent, Category = "Pressure Minigame")
	void OnMinigameFinished(bool bSuccess);

private:
	UPROPERTY()
	TWeakObjectPtr<APressureValve> TargetValve;
};
