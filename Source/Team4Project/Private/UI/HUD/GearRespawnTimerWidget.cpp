#include "UI/HUD/GearRespawnTimerWidget.h"
#include "InteractiveProp/GearSlot.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameFramework/GameStateBase.h"

void UGearRespawnTimerWidget::SetTargetSlot(AGearSlot* InSlot)
{
	TargetSlot = InSlot;
}

void UGearRespawnTimerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	AGearSlot* GearSlotPtr = TargetSlot.Get();
	if (!GearSlotPtr || !GetWorld()) return;

	// RespawnStartServerTime 은 서버 월드시간 기준 — 클라 로컬 시계 대신 서버 시계 추정치를
	// 사용한다 (GearQTEWidget / PressureMinigameWidget 과 동일한 이유).
	const AGameStateBase* GS = GetWorld()->GetGameState();
	const float Now = GS ? GS->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
	const float Elapsed = Now - GearSlotPtr->RespawnStartServerTime;
	const float TotalDur = FMath::Max(GearSlotPtr->GearRespawnDelay, KINDA_SMALL_NUMBER);
	const float Remaining = FMath::Clamp(TotalDur - Elapsed, 0.f, TotalDur);
	const float Fraction = Remaining / TotalDur;

	if (PB_Respawn)
	{
		PB_Respawn->SetPercent(Fraction);
	}

	if (TB_RespawnTime)
	{
		TB_RespawnTime->SetText(FText::FromString(FString::Printf(TEXT("%.0fs"), Remaining)));
	}
}
