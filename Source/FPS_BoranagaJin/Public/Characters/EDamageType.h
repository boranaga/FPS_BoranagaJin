

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EDamageType.generated.h"

UENUM(BlueprintType)
enum class EDamageType : uint8
{
	None UMETA(DisplayName = "None"),
	Explosion UMETA(DisplayName = "Explosion"),
	Projectile UMETA(DisplayName = "Projectile"),
	Melee UMETA(DisplayName = "Melee"),
	Charge UMETA(DisplayName = "Charge")
};

UCLASS()
class FPS_BORANAGAJIN_API ADamageTypeEnum : public AActor
{
	GENERATED_BODY()
public:
	ADamageTypeEnum();
};