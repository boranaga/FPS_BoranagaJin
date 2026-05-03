

#include "Animations/WeaponAnimInstance.h"
#include "Items/Weapons/Weapon.h"
#include "Items/WeaponState/WeaponBaseState.h"

void UWeaponAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	WeaponMesh = GetOwningComponent();

	if (WeaponMesh)
	{
		Weapon = Cast<AWeapon>(WeaponMesh->GetOwner());
	}

	PreviousWeaponStateType = EWeaponStateType::WeaponStateType_None;
	CurrentWeaponStateType = EWeaponStateType::WeaponStateType_None;
}

void UWeaponAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

	if (Weapon)
	{
		if (Weapon->GetCurrentState())
		{
			if (CurrentWeaponStateType != Weapon->GetCurrentState()->GetWeaponStateType())
			{
				PreviousWeaponStateType = CurrentWeaponStateType;
				CurrentWeaponStateType = Weapon->GetCurrentState()->GetWeaponStateType();

				if (CurrentWeaponStateType == EWeaponStateType::WeaponStateType_Reloading)
				{
					Montage_Play(AM_Mag_Reload);
				}
			}
		}
	}
}
