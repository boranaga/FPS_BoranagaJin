

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WeaponInterface.generated.h"

struct FArmRecoilParams;

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UWeaponInterface : public UInterface
{
	GENERATED_BODY()
};
/**
 *
 */
class FPS_BORANAGAJIN_API IWeaponInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual void SetRightHandToAimSocketOffset(FVector offset);

	virtual void ZoomIn(bool bZoomIn);

	virtual void ReloadingEnd();

	virtual void EjectProjectileShell();

	virtual void SwitchToOtherWeapon();

	//<ArmRecoilAnimation>
	virtual void AddArmRecoil(FArmRecoilParams* armrecoil = nullptr, float AdditionalRecoilAmountX = 0.f, float AdditionalRecoilAmountY = 0.f, float AdditionalRecoilAmountZ = 0.f);
	virtual void AddSkillWeaponRecoil(FArmRecoilParams* armrecoil = nullptr, float AdditionalRecoilAmountX = 0.f, float AdditionalRecoilAmountY = 0.f, float AdditionalRecoilAmountZ = 0.f);
};

