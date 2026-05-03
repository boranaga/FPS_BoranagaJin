

#pragma once

#include "CoreMinimal.h"
#include "WeaponBaseState.h"
#include "WeaponIdleState.generated.h"

/**
 *
 */
UCLASS()
class FPS_BORANAGAJIN_API UWeaponIdleState : public UWeaponBaseState
{
	GENERATED_BODY()
public:
	UWeaponIdleState();
	~UWeaponIdleState();

	virtual void EnterState(AWeapon* Weapon) override;

	virtual void UpdateState(AWeapon* Weapon, float DeltaTime)override;

	virtual void ExitState(AWeapon* Weapon)override;
};

