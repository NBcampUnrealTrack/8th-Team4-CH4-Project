#include "InteractiveProp/CoalItem.h"
#include "Player/GODPlayerState.h"
#include "GameFramework/Character.h"

ACoalItem::ACoalItem()
{
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
