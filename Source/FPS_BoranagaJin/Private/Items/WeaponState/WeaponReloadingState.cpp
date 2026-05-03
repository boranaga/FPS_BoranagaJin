


#include "Items/WeaponState/WeaponReloadingState.h"
#include "Items/Weapons/Weapon.h"

UWeaponReloadingState::UWeaponReloadingState()
{
	WeaponStateType = EWeaponStateType::WeaponStateType_Reloading;
}

UWeaponReloadingState::~UWeaponReloadingState()
{
}

void UWeaponReloadingState::EnterState(AWeapon* Weapon)
{
	Super::EnterState(Weapon);
	Weapon->StartReload();
}

void UWeaponReloadingState::UpdateState(AWeapon* Weapon, float DeltaTime)
{
	Super::UpdateState(Weapon, DeltaTime);
}

void UWeaponReloadingState::ExitState(AWeapon* Weapon)
{
	Super::ExitState(Weapon);
}

