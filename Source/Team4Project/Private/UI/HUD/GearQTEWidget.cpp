#include "UI/HUD/GearQTEWidget.h"
#include "InteractiveProp/GearSlot.h"
#include "GameFramework/GameStateBase.h"

void UGearQTEWidget::SetTargetSlot(AGearSlot* InSlot)
{
	TargetSlot = InSlot;
}

void UGearQTEWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	AGearSlot* GearSlotPtr = TargetSlot.Get();
	if (!GearSlotPtr || !GetWorld()) return;

	// QTEStartServerTime 은 서버 월드시간 기준 — 클라 로컬 시계 대신 서버 시계 추정치를 사용.
	// (PressureMinigameWidget 과 동일한 이유)
	const AGameStateBase* GS = GetWorld()->GetGameState();
	const float Now = GS ? GS->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
	const float Elapsed = Now - GearSlotPtr->QTEStartServerTime;
	const float Fraction = FMath::Clamp(1.f - (Elapsed / FMath::Max(GearSlotPtr->QTETimeLimit, KINDA_SMALL_NUMBER)), 0.f, 1.f);

	OnQTEDataUpdated(GearSlotPtr->QTESequence, GearSlotPtr->QTEProgressIndex, Fraction);
}
