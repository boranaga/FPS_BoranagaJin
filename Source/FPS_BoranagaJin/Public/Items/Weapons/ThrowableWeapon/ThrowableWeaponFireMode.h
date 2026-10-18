#pragma once

#include "CoreMinimal.h"
#include "ThrowableWeaponFireMode.generated.h"

UENUM(BlueprintType)
enum class EThrowableWeaponFireMode : uint8
{
	Upward		UMETA(DisplayName = "Upward"),
	Downward	UMETA(DisplayName = "Downward"),
	None        UMETA(DisplayName = "None")
};