#include "InteractiveProp/CoalItem.h"
#include "Player/GODPlayerState.h"
#include "Game/BaseGameplayTags.h"
#include "GameFramework/Character.h"

ACoalItem::ACoalItem()
{
	EquipStateTag = State::Weapon::EquipCoal.GetTag();
	WeightAmount = 30.f; 
}

void ACoalItem::Server_PickUp_Implementation(ACharacter* Holder)
{
	Super::Server_PickUp_Implementation(Holder);

	if (Holder)
	{
		if (AGODPlayerState* PS = Holder->GetPlayerState<AGODPlayerState>())
		{
			PS->SootLevel = FMath::Clamp(PS->SootLevel + SootOnPickup, 0.f, 1.f);
		}
	}
}
