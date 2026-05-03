

#pragma once

#include "CoreMinimal.h"
#include "WeaponBaseState.h"
#include "WeaponFullAutoFiringState.generated.h"
/**
 *
 */
UCLASS()
class FPS_BORANAGAJIN_API UWeaponFullAutoFiringState : public UWeaponBaseState
{
	GENERATED_BODY()
public:
	UWeaponFullAutoFiringState();
	~UWeaponFullAutoFiringState();

	virtual void EnterState(AWeapon* Weapon) override;

	virtual void UpdateState(AWeapon* Weapon, float DeltaTime) override;

	virtual void ExitState(AWeapon* Weapon) override;
};

