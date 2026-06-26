#pragma once

#include "NativeGameplayTags.h"
//#include "Runtime/GameplayTags/Public/NativeGameplayTags.h"

namespace Character
{
	namespace Crew
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mechanic)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Watchman)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stoker)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Porter)
	}
	namespace Special
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sheriff)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mafia)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Outlaw)
	}
}

namespace Role
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(HasWireCutter)
}

namespace SetByCaller
{
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown)
}
namespace Abilities
{
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attack)
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Interact)
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Invisible)
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Equip)
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Detect)
}

namespace State
{
    namespace Weapon
    {
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(EquipGun)
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(EquipCoal)
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(EquipGear)
    }
}

namespace Cooldown
{
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attack)
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Invisible)
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Detect)
}

namespace Event
{
    // 처치 이벤트. 사망 시 킬러 ASC 로 전송 → 마피아 시체 은폐 등 패시브 어빌리티 트리거.
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Kill)
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
