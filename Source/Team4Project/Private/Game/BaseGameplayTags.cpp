#include "Game/BaseGameplayTags.h"

namespace Character
{
	namespace Crew
	{
		UE_DEFINE_GAMEPLAY_TAG(Mechanic, "Character.Crew.Mechanic")
		UE_DEFINE_GAMEPLAY_TAG(Sheriff, "Character.Crew.Sheriff")
		UE_DEFINE_GAMEPLAY_TAG(Mafia, "Character.Crew.Mafia")
		UE_DEFINE_GAMEPLAY_TAG(Outlaw, "Character.Crew.Outlaw")
	}
}

namespace SetByCaller
{
	UE_DEFINE_GAMEPLAY_TAG(Cooldown, "SetByCaller.Cooldown")
}

namespace Abilities
{
	UE_DEFINE_GAMEPLAY_TAG(Attack, "Ability.Attack")
	UE_DEFINE_GAMEPLAY_TAG(Interact, "Ability.Interact")
	UE_DEFINE_GAMEPLAY_TAG(Invisible, "Ability.Invisible")
	UE_DEFINE_GAMEPLAY_TAG(Equip, "Ability.Equip")
	UE_DEFINE_GAMEPLAY_TAG(Detect, "Ability.Detect")
}

namespace State
{
	namespace Weapon
	{
		UE_DEFINE_GAMEPLAY_TAG(EquipGun, "State.Equip.Gun")
		UE_DEFINE_GAMEPLAY_TAG(EquipCoal, "State.Equip.Coal")
		UE_DEFINE_GAMEPLAY_TAG(EquipGear, "State.Equip.Gear")
	}
}

namespace Cooldown
{
	UE_DEFINE_GAMEPLAY_TAG(Attack, "Cooldown.Attack")
	UE_DEFINE_GAMEPLAY_TAG(Use, "Cooldown.Invisible")
	UE_DEFINE_GAMEPLAY_TAG(Detect, "Cooldown.Detect")
}

namespace Event
{
	UE_DEFINE_GAMEPLAY_TAG(Kill, "Event.Kill")
}

namespace Items
{
	namespace Equipment
	{
		namespace Weapons
		{
			UE_DEFINE_GAMEPLAY_TAG(Gun, "Items.Equipment.Weapons.Gun")
			UE_DEFINE_GAMEPLAY_TAG(Knife, "Items.Equipment.Weapons.Knife")
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