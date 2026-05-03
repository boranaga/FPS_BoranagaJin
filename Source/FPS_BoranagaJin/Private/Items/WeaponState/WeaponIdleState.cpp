


#include "Items/WeaponState/WeaponIdleState.h"
#include "Items/Weapons/Weapon.h"

UWeaponIdleState::UWeaponIdleState()
{
	WeaponStateType = EWeaponStateType::WeaponStateType_Idle;
}

UWeaponIdleState::~UWeaponIdleState()
{
}

void UWeaponIdleState::EnterState(AWeapon* Weapon)
{
	Super::EnterState(Weapon);
	Weapon->AutoReload();
}

void UWeaponIdleState::UpdateState(AWeapon* Weapon, float DeltaTime)
{
	Super::UpdateState(Weapon, DeltaTime);
}

void UWeaponIdleState::ExitState(AWeapon* Weapon)
{
	Super::ExitState(Weapon);
}
