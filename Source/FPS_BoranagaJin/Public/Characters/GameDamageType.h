#pragma once

#include "CoreMinimal.h"
#include "GameDamageType.generated.h"

UENUM(BlueprintType)
enum class EGameDamageType : uint8
{
	None		UMETA(DisplayName = "None"),
	Bullet		UMETA(DisplayName = "Bullet"),
	Melee		UMETA(DisplayName = "Melee"),
	Explosion	UMETA(DisplayName = "Explosion"),
	Fire		UMETA(DisplayName = "Fire"),
	Poison		UMETA(DisplayName = "Poison"),
	Charge		UMETA(DisplayName = "Charge"),
};