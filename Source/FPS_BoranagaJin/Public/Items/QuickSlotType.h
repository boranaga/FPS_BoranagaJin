
#pragma once

#include "CoreMinimal.h"
#include "QuickSlotType.generated.h"

UENUM(BlueprintType)
enum class EQuickSlotType : uint8
{
    PrimaryWeapon,
    SecondaryWeapon,
    ThrowableWeapon,
    Flashlight
};
