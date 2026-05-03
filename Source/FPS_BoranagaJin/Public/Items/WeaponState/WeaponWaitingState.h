

#pragma once

#include "CoreMinimal.h"
#include "WeaponBaseState.h"
#include "WeaponWaitingState.generated.h"

/**
 *
 */
UCLASS()
class FPS_BORANAGAJIN_API UWeaponWaitingState : public UWeaponBaseState
{
	GENERATED_BODY()
public:
	UWeaponWaitingState();
	~UWeaponWaitingState();

	virtual void EnterState(AWeapon* Weapon) override;

	virtual void UpdateState(AWeapon* Weapon, float DeltaTime) override;

	virtual void ExitState(AWeapon* Weapon) override;
};