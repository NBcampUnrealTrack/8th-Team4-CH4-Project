#include "InteractiveProp/PickupGear.h"
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
	Super::Server_PickUp_Implementation(Holder);

	if (Holder)
	{
		if (AGODPlayerState* PS = Holder->GetPlayerState<AGODPlayerState>())
		{
			PS->SootLevel = FMath::Clamp(PS->SootLevel + SootOnPickup, 0.f, 1.f);
		}
	}
}
