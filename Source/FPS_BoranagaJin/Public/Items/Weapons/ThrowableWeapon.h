
#pragma once

#include "CoreMinimal.h"
#include "Items/Weapons/Weapon.h"
#include "ThrowableWeapon.generated.h"

UCLASS()
class FPS_BORANAGAJIN_API AThrowableWeapon : public AWeapon
{
	GENERATED_BODY()
public:
	AThrowableWeapon();
protected:
	virtual void BeginPlay() override;
public:
	virtual void Tick(float DeltaTime) override;
	virtual void InitItem(ACharacterPlayer* NewCharacter) override;
	virtual void LoadWeaponData_Upgrade() override;
	virtual void SetInputActionBinding() override;
};
