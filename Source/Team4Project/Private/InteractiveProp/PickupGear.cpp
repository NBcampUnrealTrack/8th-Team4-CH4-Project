#include "InteractiveProp/PickupGear.h"
#include "InteractiveProp/GearSlot.h"
#include "Player/GODPlayerState.h"
#include "Game/BaseGameplayTags.h"
#include "GameFramework/Character.h"

APickupGear::APickupGear()
{
	EquipStateTag = State::Weapon::EquipGear.GetTag();
	WeightAmount = 15.f;
}

void APickupGear::Server_PickUp_Implementation(ACharacter* Holder)
{
	const bool bWasHeldBefore = bIsHeld;

	Super::Server_PickUp_Implementation(Holder);

	if (Holder)
	{
		if (AGODPlayerState* PS = Holder->GetPlayerState<AGODPlayerState>())
		{
			PS->SootLevel = FMath::Clamp(PS->SootLevel + SootOnPickup, 0.f, 1.f);
		}
	}

	// 실제로 이번 호출에서 새로 집힌 경우에만(중복 방지) 슬롯에 리스폰 카운트다운을 알린다.
	if (!bWasHeldBefore && bIsHeld && OwningGearSlot)
	{
		OwningGearSlot->OnGearTaken();
	}
}
