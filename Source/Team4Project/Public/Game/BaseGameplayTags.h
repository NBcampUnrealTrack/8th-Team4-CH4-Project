#pragma once

#include "NativeGameplayTags.h"
//#include "Runtime/GameplayTags/Public/NativeGameplayTags.h"

namespace Character
{
    namespace Crew
    {
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mechanic)
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sheriff)
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mafia)
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Outlaw)
    }
}

namespace SetByCaller
{
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown)
}
namespace Abilities
{
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attack)
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Interact)
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Use)
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Equip)
}

namespace State
{
    namespace Weapon
    {
        // 총기 장착 상태 (Fire 어빌리티 발동 조건)
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(GunEquipped)
    }
}

namespace Cooldown
{
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attack)
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Use)
}

namespace Items
{
    namespace Equipment
    {
        namespace Weapons
        {
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gun)
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Knife)
 
        }
    }

    namespace Consumables
    {
        namespace Potions
        {
            
        }
    }

    namespace Craftables
    {

    }
}
