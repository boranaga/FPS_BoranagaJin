


#include "Items/Weapons/WeaponInterface.h"
#include "Items/Weapons/ArmRecoilParams.h"

// Add default functionality here for any IWeaponInterface functions that are not pure virtual.

void IWeaponInterface::SetRightHandToAimSocketOffset(FVector offset)
{
}

void IWeaponInterface::ZoomIn(bool bZoomIn)
{
}

void IWeaponInterface::ReloadingEnd()
{
}

void IWeaponInterface::EjectProjectileShell()
{
}

void IWeaponInterface::SwitchToOtherWeapon()
{
}

void IWeaponInterface::AddArmRecoil(FArmRecoilParams* armrecoil, float AdditionalRecoilAmountX, float AdditionalRecoilAmountY, float AdditionalRecoilAmountZ)
{
}

void IWeaponInterface::AddSkillWeaponRecoil(FArmRecoilParams* armrecoil, float AdditionalRecoilAmountX, float AdditionalRecoilAmountY, float AdditionalRecoilAmountZ)
{
}
