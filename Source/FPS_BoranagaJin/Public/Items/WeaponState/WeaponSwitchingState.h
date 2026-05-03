

#pragma once

#include "CoreMinimal.h"
#include "WeaponBaseState.h"
#include "WeaponSwitchingState.generated.h"

/**
 *
 */
UCLASS()
class FPS_BORANAGAJIN_API UWeaponSwitchingState : public UWeaponBaseState
{
	GENERATED_BODY()
public:
	UWeaponSwitchingState();
	~UWeaponSwitchingState();

	virtual void EnterState(AWeapon* Weapon) override;

	virtual void UpdateState(AWeapon* Weapon, float DeltaTime)override;

	virtual void ExitState(AWeapon* Weapon)override;
};
