

#pragma once

#pragma once

#include "CoreMinimal.h"
#include "WeaponBaseState.h"
#include "WeaponTargetingState.generated.h"

UCLASS()
class FPS_BORANAGAJIN_API UWeaponTargetingState : public UWeaponBaseState
{
	GENERATED_BODY()
public:
	UWeaponTargetingState();
	~UWeaponTargetingState();

	virtual void EnterState(AWeapon* Weapon) override;

	virtual void UpdateState(AWeapon* Weapon, float DeltaTime) override;

	virtual void ExitState(AWeapon* Weapon) override;
};
