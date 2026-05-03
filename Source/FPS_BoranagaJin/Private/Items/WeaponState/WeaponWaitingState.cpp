


#include "Items/WeaponState/WeaponWaitingState.h"
#include "Items/Weapons/Weapon.h"

UWeaponWaitingState::UWeaponWaitingState()
{
	WeaponStateType = EWeaponStateType::WeaponStateType_Waiting;
}

UWeaponWaitingState::~UWeaponWaitingState()
{
}

void UWeaponWaitingState::EnterState(AWeapon* Weapon)
{
	Super::EnterState(Weapon);
	Weapon->ActivateAimUIWidget(false);
}

void UWeaponWaitingState::UpdateState(AWeapon* Weapon, float DeltaTime)
{
	Super::UpdateState(Weapon, DeltaTime);
}

void UWeaponWaitingState::ExitState(AWeapon* Weapon)
{
	Super::ExitState(Weapon);
	Weapon->ActivateAimUIWidget(true);
}
