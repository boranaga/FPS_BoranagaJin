

#pragma once

#include "CoreMinimal.h"
#include "WeaponBaseState.h"
#include "WeaponPumpActionReloadState.generated.h"

/**
 *
 */
UCLASS()
class FPS_BORANAGAJIN_API UWeaponPumpActionReloadState : public UWeaponBaseState
{
	GENERATED_BODY()
public:
	UWeaponPumpActionReloadState();
	~UWeaponPumpActionReloadState();

	virtual void EnterState(AWeapon* Weapon) override;

	virtual void UpdateState(AWeapon* Weapon, float DeltaTime) override;

	virtual void ExitState(AWeapon* Weapon) override;
};

