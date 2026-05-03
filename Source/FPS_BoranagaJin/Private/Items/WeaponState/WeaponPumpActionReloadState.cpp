


#include "Items/WeaponState/WeaponPumpActionReloadState.h"
#include "Items/Weapons/Weapon.h"

UWeaponPumpActionReloadState::UWeaponPumpActionReloadState()
{
	WeaponStateType = EWeaponStateType::WeaponStateType_PumpActionReloading;
}

UWeaponPumpActionReloadState::~UWeaponPumpActionReloadState()
{
}

void UWeaponPumpActionReloadState::EnterState(AWeapon* Weapon)
{
	Super::EnterState(Weapon);
	Weapon->StartPumpActionReload();
}

void UWeaponPumpActionReloadState::UpdateState(AWeapon* Weapon, float DeltaTime)
{
	Super::UpdateState(Weapon, DeltaTime);
}

void UWeaponPumpActionReloadState::ExitState(AWeapon* Weapon)
{
	Super::ExitState(Weapon);
}

