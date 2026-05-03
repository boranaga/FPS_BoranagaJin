


#include "Items/WeaponState/WeaponSwitchingState.h"
#include "Items/Weapons/Weapon.h"

UWeaponSwitchingState::UWeaponSwitchingState()
{
	WeaponStateType = EWeaponStateType::WeaponStateType_Switching;
}

UWeaponSwitchingState::~UWeaponSwitchingState()
{
}

void UWeaponSwitchingState::EnterState(AWeapon* Weapon)
{
	Super::EnterState(Weapon);
	Weapon->ZoomOut();
}

void UWeaponSwitchingState::UpdateState(AWeapon* Weapon, float DeltaTime)
{
	Super::UpdateState(Weapon, DeltaTime);
}

void UWeaponSwitchingState::ExitState(AWeapon* Weapon)
{
	Super::ExitState(Weapon);
}
