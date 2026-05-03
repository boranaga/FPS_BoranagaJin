


#include "Items/WeaponState/WeaponFiringState.h"

UWeaponFiringState::UWeaponFiringState()
{
	WeaponStateType = EWeaponStateType::WeaponStateType_Firing;
}

UWeaponFiringState::~UWeaponFiringState()
{
}

void UWeaponFiringState::EnterState(AWeapon* Weapon)
{
	Super::EnterState(Weapon);
}

void UWeaponFiringState::UpdateState(AWeapon* Weapon, float DeltaTime)
{
	Super::UpdateState(Weapon, DeltaTime);
}

void UWeaponFiringState::ExitState(AWeapon* Weapon)
{
	Super::ExitState(Weapon);
	//Weapon->AutoReload();
}