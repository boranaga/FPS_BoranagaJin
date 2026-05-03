

#pragma once

#include "CoreMinimal.h"
#include "WeaponStateType.h"
#include "WeaponBaseState.generated.h"

class AWeapon;
/**
 *
 */
UCLASS(Abstract)
class FPS_BORANAGAJIN_API UWeaponBaseState : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly, Category = "WeaponState")
	EWeaponStateType WeaponStateType;

	FName StateDisplayName;

	virtual EWeaponStateType GetWeaponStateType() const;

	virtual void EnterState(AWeapon* Weapon);

	virtual void UpdateState(AWeapon* Weapon, float DeltaTime);

	virtual void ExitState(AWeapon* Weapon);
};
