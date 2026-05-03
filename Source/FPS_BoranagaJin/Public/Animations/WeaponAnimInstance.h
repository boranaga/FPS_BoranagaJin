

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Items/WeaponState/WeaponStateType.h"
#include "WeaponAnimInstance.generated.h"

class AWeapon;
/**
 *
 */
UCLASS()
class FPS_BORANAGAJIN_API UWeaponAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
protected:
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	AWeapon* Weapon = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	USkeletalMeshComponent* WeaponMesh = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	EWeaponStateType PreviousWeaponStateType;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	EWeaponStateType CurrentWeaponStateType;

	virtual void NativeInitializeAnimation() override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	UAnimMontage* AM_Mag_Reload = nullptr;
};
