


#include "Items/WeaponState/WeaponFullAutoFiringState.h"

UWeaponFullAutoFiringState::UWeaponFullAutoFiringState()
{
	WeaponStateType = EWeaponStateType::WeaponStateType_FullAutoFiring;
}

UWeaponFullAutoFiringState::~UWeaponFullAutoFiringState()
{
}

void UWeaponFullAutoFiringState::EnterState(AWeapon* Weapon)
{
	Super::EnterState(Weapon);
}

void UWeaponFullAutoFiringState::UpdateState(AWeapon* Weapon, float DeltaTime)
{
	Super::UpdateState(Weapon, DeltaTime);
}

void UWeaponFullAutoFiringState::ExitState(AWeapon* Weapon)
{
	Super::ExitState(Weapon);
}

