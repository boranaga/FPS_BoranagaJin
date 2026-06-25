#pragma once

#include "CoreMinimal.h"
#include "EnemyStateType.generated.h"

UENUM(BlueprintType)
enum class EEnemyStateType : uint8
{
	Idle	UMETA(DisplayName = "Idle"),
	Patrol	UMETA(DisplayName = "Patrol"),
	Chase	UMETA(DisplayName = "Chase"),
	Attack	UMETA(DisplayName = "Attack"),
	Dead	UMETA(DisplayName = "Dead")
};