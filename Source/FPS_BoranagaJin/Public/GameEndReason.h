#pragma once

#include "CoreMinimal.h"
#include "GameEndReason.generated.h"

UENUM(BlueprintType)
enum class EGameEndReason : uint8
{
	None,
	PlayerDead,
	BossDefeated,
	Escaped
};