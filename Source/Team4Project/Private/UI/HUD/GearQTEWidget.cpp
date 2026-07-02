#include "UI/HUD/GearQTEWidget.h"
#include "InteractiveProp/GearSlot.h"

void UGearQTEWidget::SetTargetSlot(AGearSlot* InSlot)
{
	TargetSlot = InSlot;
}

void UGearQTEWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	AGearSlot* GearSlotPtr = TargetSlot.Get();
	if (!GearSlotPtr || !GetWorld()) return;

	const float Elapsed = GetWorld()->GetTimeSeconds() - GearSlotPtr->QTEStartServerTime;
	const float Fraction = FMath::Clamp(1.f - (Elapsed / FMath::Max(GearSlotPtr->QTETimeLimit, KINDA_SMALL_NUMBER)), 0.f, 1.f);

	OnQTEDataUpdated(GearSlotPtr->QTESequence, GearSlotPtr->QTEProgressIndex, Fraction);
}
