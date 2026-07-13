#include "Game/BaseGameplayTags.h"

namespace Character
{
	namespace Crew
	{
		UE_DEFINE_GAMEPLAY_TAG(Mechanic, "Character.Crew.Mechanic")
		UE_DEFINE_GAMEPLAY_TAG(Watchman, "Character.Crew.Watchman")
		UE_DEFINE_GAMEPLAY_TAG(Stoker,   "Character.Crew.Stoker")
		UE_DEFINE_GAMEPLAY_TAG(Porter,   "Character.Crew.Porter")
	}
	namespace Special
	{
		UE_DEFINE_GAMEPLAY_TAG(Sheriff, "Character.Special.Sheriff")
		UE_DEFINE_GAMEPLAY_TAG(Mafia,   "Character.Special.Mafia")
		UE_DEFINE_GAMEPLAY_TAG(Outlaw,  "Character.Special.Outlaw")
	}
}

namespace Role
{
	UE_DEFINE_GAMEPLAY_TAG(HasWireCutter, "Role.HasWireCutter")
}

namespace SetByCaller
{
	UE_DEFINE_GAMEPLAY_TAG(Cooldown, "SetByCaller.Cooldown")
}

namespace Abilities
{
	UE_DEFINE_GAMEPLAY_TAG(Attack,   "Ability.Attack")
	UE_DEFINE_GAMEPLAY_TAG(Interact, "Ability.Interact")
	UE_DEFINE_GAMEPLAY_TAG(Invisible, "Ability.Invisible")
	UE_DEFINE_GAMEPLAY_TAG(Equip, "Ability.Equip")
	UE_DEFINE_GAMEPLAY_TAG(Detect, "Ability.Detect")
	UE_DEFINE_GAMEPLAY_TAG(Push, "Ability.Push")
}

namespace State
{
	namespace Weapon
	{
		UE_DEFINE_GAMEPLAY_TAG(EquipGun,  "State.Equip.Gun")
		UE_DEFINE_GAMEPLAY_TAG(EquipCoal, "State.Equip.Coal")
		UE_DEFINE_GAMEPLAY_TAG(EquipGear, "State.Equip.Gear")
	}
	namespace Debuff
	{
		UE_DEFINE_GAMEPLAY_TAG(Snow, "State.Debuff.Snow")
	}

	UE_DEFINE_GAMEPLAY_TAG(Stumble, "State.Stumble")
	UE_DEFINE_GAMEPLAY_TAG(Meeting, "State.Meeting")
}

namespace Cooldown
{
	UE_DEFINE_GAMEPLAY_TAG(Attack, "Cooldown.Attack")
	UE_DEFINE_GAMEPLAY_TAG(Use, "Cooldown.Invisible")
	UE_DEFINE_GAMEPLAY_TAG(Detect, "Cooldown.Detect")
	UE_DEFINE_GAMEPLAY_TAG(Push, "Cooldown.Push")
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
			UE_DEFINE_GAMEPLAY_TAG(Gun,   "Items.Equipment.Weapons.Gun")
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
