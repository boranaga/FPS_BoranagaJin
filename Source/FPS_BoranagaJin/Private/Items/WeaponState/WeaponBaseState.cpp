


#include "Items/WeaponState/WeaponBaseState.h"
#include "Items/Weapons/Weapon.h"

EWeaponStateType UWeaponBaseState::GetWeaponStateType() const
{
    return WeaponStateType;
}

void UWeaponBaseState::EnterState(AWeapon* Weapon)
{
}

void UWeaponBaseState::UpdateState(AWeapon* Weapon, float DeltaTime)
{
}

void UWeaponBaseState::ExitState(AWeapon* Weapon)
{
}
