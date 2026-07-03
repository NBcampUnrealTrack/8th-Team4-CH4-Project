#include "InteractiveProp/CoalItem.h"
#include "Player/GODPlayerState.h"
#include "Game/BaseGameplayTags.h"
#include "GameFramework/Character.h"
#include "Player/BaseCharacter.h"

ACoalItem::ACoalItem()
{
	EquipStateTag = State::Weapon::EquipCoal.GetTag();
	WeightAmount = 30.f;

	// 손 소켓에 부착 (스켈레톤에 동일 이름의 소켓이 있어야 함).
	AttachSocketName = TEXT("Right_HandSocket");

	// 떨궜을 때 잘 안 굴러가도록 감쇠를 크게 (구르기 억제엔 각 감쇠가 핵심).
	DroppedAngularDamping = 8.f;
	DroppedLinearDamping = 1.f;
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
		
		 if (ABaseCharacter* Char = Cast<ABaseCharacter>(Holder))
            {
                if (!Char->IsCoalEquipped())
                {
                    Char->SetCoalEquipped(true);
                }
            }
	}
}
