

#pragma once

#include "CoreMinimal.h"
#include "WeaponBaseState.h"
#include "WeaponReloadingState.generated.h"

/**
 *
 */
UCLASS()
class FPS_BORANAGAJIN_API UWeaponReloadingState : public UWeaponBaseState
{
	GENERATED_BODY()
public:
	UWeaponReloadingState();
	~UWeaponReloadingState();

	virtual void EnterState(AWeapon* Weapon) override;

	virtual void UpdateState(AWeapon* Weapon, float DeltaTime) override;

	virtual void ExitState(AWeapon* Weapon) override;
};

