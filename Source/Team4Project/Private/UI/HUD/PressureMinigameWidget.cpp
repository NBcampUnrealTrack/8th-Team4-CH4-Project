#include "UI/HUD/PressureMinigameWidget.h"
#include "InteractiveProp/PressureValve.h"

void UPressureMinigameWidget::SetTargetValve(APressureValve* InValve)
{
	TargetValve = InValve;
}

void UPressureMinigameWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	APressureValve* Valve = TargetValve.Get();
	if (!Valve || !GetWorld()) return;

	const float Elapsed = GetWorld()->GetTimeSeconds() - Valve->RoundStartServerTime;
	const float NeedlePos = APressureValve::ComputeNeedlePosition(Elapsed, Valve->NeedleOscillationPeriod);

	OnMinigameDataUpdated(NeedlePos, Valve->RedZoneStart, Valve->RedZoneEnd, Valve->SuccessCount, Valve->MissCount);
}
