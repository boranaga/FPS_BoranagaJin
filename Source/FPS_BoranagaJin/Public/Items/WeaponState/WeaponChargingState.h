

#pragma once

#include "CoreMinimal.h"
#include "WeaponBaseState.h"
#include "WeaponChargingState.generated.h"

/**
 *
 */
UCLASS()
class FPS_BORANAGAJIN_API UWeaponChargingState : public UWeaponBaseState
{
	GENERATED_BODY()
public:
	UWeaponChargingState();
	~UWeaponChargingState();

	virtual void EnterState(AWeapon* Weapon) override;

	virtual void UpdateState(AWeapon* Weapon, float DeltaTime) override;

	virtual void ExitState(AWeapon* Weapon) override;
};
