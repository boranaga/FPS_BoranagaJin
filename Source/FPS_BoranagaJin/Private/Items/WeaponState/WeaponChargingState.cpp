


#include "Items/WeaponState/WeaponChargingState.h"

UWeaponChargingState::UWeaponChargingState()
{
	WeaponStateType = EWeaponStateType::WeaponStateType_Charging;
}

UWeaponChargingState::~UWeaponChargingState()
{
}

void UWeaponChargingState::EnterState(AWeapon* Weapon)
{
	Super::EnterState(Weapon);
}

void UWeaponChargingState::UpdateState(AWeapon* Weapon, float DeltaTime)
{
	Super::UpdateState(Weapon, DeltaTime);
}

void UWeaponChargingState::ExitState(AWeapon* Weapon)
{
	Super::ExitState(Weapon);
}

