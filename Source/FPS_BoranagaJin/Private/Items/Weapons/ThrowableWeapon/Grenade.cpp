#include "Items/Weapons/ThrowableWeapon/Grenade.h"

AGrenade::AGrenade()
{
	ItemName =
		EItemName::ItemName_Grenade;

	UpwardThrowStrength = 1600.f;
	DownwardThrowStrength = 900.f;

	UpwardPitchOffset = 15.f;
	DownwardPitchOffset = -20.f;
}