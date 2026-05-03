


#include "Items/WeaponState/WeaponTargetingState.h"
#include "Items/Weapons/Weapon.h"

UWeaponTargetingState::UWeaponTargetingState()
{
	WeaponStateType = EWeaponStateType::WeaponStateType_Targeting;
}

UWeaponTargetingState::~UWeaponTargetingState()
{
}

void UWeaponTargetingState::EnterState(AWeapon* Weapon)
{
	Super::EnterState(Weapon);
	//Weapon->SetGlobalTimeDilation(Weapon->TargetingGlobalTimeScale);
	//if (Weapon->IsSkillWeapon()) { Weapon->ActivateAimUIWidget(true); }
}

void UWeaponTargetingState::UpdateState(AWeapon* Weapon, float DeltaTime)
{
	Super::UpdateState(Weapon, DeltaTime);
	//Weapon->UpdateTargetMarkers();
}

void UWeaponTargetingState::ExitState(AWeapon* Weapon)
{
	Super::ExitState(Weapon);
	//Weapon->SetGlobalTimeDilation(1.f);
	//if (Weapon->IsSkillWeapon()) { Weapon->ActivateAimUIWidget(false); }
}
