#pragma once

#include "CoreMinimal.h"
#include "ThrowableWeaponStateType.generated.h"

UENUM(BlueprintType)
enum class EThrowableWeaponStateType : uint8
{
	Idle		UMETA(DisplayName = "Idle"),
	Aiming		UMETA(DisplayName = "Aiming"),
	Firing		UMETA(DisplayName = "Firing"),
	Switching   UMETA(DisplayName = "Switching"),
	Reloading   UMETA(DisplayName = "Reloading"),
	Charging    UMETA(DisplayName = "Charging"),
	Unequiped   UMETA(DisplayName = "Unequiped"),
	None        UMETA(DisplayName = "None")
};