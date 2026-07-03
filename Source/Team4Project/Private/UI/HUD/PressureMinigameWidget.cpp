#include "UI/HUD/PressureMinigameWidget.h"
#include "InteractiveProp/PressureValve.h"
#include "GameFramework/GameStateBase.h"

void UPressureMinigameWidget::SetTargetValve(APressureValve* InValve)
{
	TargetValve = InValve;
}

void UPressureMinigameWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	APressureValve* Valve = TargetValve.Get();
	if (!Valve || !GetWorld()) return;

	// RoundStartServerTime 은 "서버 월드시간" 기준. 클라 GetTimeSeconds 는 접속 시점부터
	// 시작하는 로컬 시계라 그대로 빼면 바늘 표시가 서버 판정과 크게 어긋난다.
	// GameState 의 서버 시계 추정치(핑 보정 포함)를 사용해 판정 공식과 정렬한다.
	const AGameStateBase* GS = GetWorld()->GetGameState();
	const float Now = GS ? GS->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
	const float Elapsed = Now - Valve->RoundStartServerTime;
	const float NeedlePos = APressureValve::ComputeNeedlePosition(Elapsed, Valve->NeedleOscillationPeriod);

	OnMinigameDataUpdated(NeedlePos, Valve->RedZoneStart, Valve->RedZoneEnd, Valve->SuccessCount, Valve->MissCount);
}
