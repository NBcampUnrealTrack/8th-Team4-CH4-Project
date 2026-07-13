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
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Push)
}

namespace State
{
    namespace Weapon
    {
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(EquipCoal)
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(EquipGear)
    }
	namespace Debuff
    {
    	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Snow)
    }

    // 밀쳐져 비틀거리는 중. 이 동안 반격(밀치기) 불가.
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stumble)

    // 긴급 회의 중. 회의 시작 시 전원 ASC 에 부여 → 살해/능력 GA 전부 차단.
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Meeting)
}

namespace Cooldown
{
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Invisible)
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Detect)
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Push)
}
