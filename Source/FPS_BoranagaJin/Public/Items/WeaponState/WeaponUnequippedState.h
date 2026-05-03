

#pragma once

#include "CoreMinimal.h"
#include "WeaponBaseState.h"
#include "WeaponUnequippedState.generated.h"

/**
 *
 */
UCLASS()
class FPS_BORANAGAJIN_API UWeaponUnequippedState : public UWeaponBaseState
{
	GENERATED_BODY()
public:
	UWeaponUnequippedState();
	~UWeaponUnequippedState();

	virtual void EnterState(AWeapon* Weapon) override;

	virtual void UpdateState(AWeapon* Weapon, float DeltaTime)override;

	virtual void ExitState(AWeapon* Weapon)override;
};
