


#include "Items/WeaponState/WeaponUnequippedState.h"
#include "Items/Weapons/Weapon.h"

UWeaponUnequippedState::UWeaponUnequippedState()
{
	WeaponStateType = EWeaponStateType::WeaponStateType_Unequipped;
}

UWeaponUnequippedState::~UWeaponUnequippedState()
{
}

void UWeaponUnequippedState::EnterState(AWeapon* Weapon)
{
	Super::EnterState(Weapon);

	//TODO: 관련함수들 묶어서 관리하는 함수 만들기
	Weapon->ForceStopCamModification();
	Weapon->ActivateAimUIWidget(false);
	Weapon->ActivateAmmoCounterWidget(false);
	//Weapon->ActivateTargetingSkillWidget(false);
}

void UWeaponUnequippedState::UpdateState(AWeapon* Weapon, float DeltaTime)
{
	Super::UpdateState(Weapon, DeltaTime);
}

void UWeaponUnequippedState::ExitState(AWeapon* Weapon)
{
	Super::ExitState(Weapon);
	//Weapon->StartCameraSettingChange(&Weapon->CamSetting_Default); //TODO:
}
