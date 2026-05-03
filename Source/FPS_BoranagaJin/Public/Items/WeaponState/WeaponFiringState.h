

#pragma once

#include "CoreMinimal.h"
#include "WeaponBaseState.h"
#include "WeaponFiringState.generated.h"

/**
 *
 */
UCLASS()
class FPS_BORANAGAJIN_API UWeaponFiringState : public UWeaponBaseState
{
	GENERATED_BODY()
public:
	UWeaponFiringState();
	~UWeaponFiringState();

	virtual void EnterState(AWeapon* Weapon) override;

	virtual void UpdateState(AWeapon* Weapon, float DeltaTime) override;

	virtual void ExitState(AWeapon* Weapon) override;
};
