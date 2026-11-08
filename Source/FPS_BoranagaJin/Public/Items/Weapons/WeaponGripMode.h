#pragma once

#include "CoreMinimal.h"
#include "WeaponGripMode.generated.h"

UENUM(BlueprintType)
enum class EWeaponGripMode : uint8
{
    TwoHanded UMETA(DisplayName = "Two Handed"),
    OneHanded UMETA(DisplayName = "One Handed")
};

