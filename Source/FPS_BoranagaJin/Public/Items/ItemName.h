#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemName.generated.h"

UENUM(BlueprintType)
enum class EItemName : uint8
{
	ItemName_Rifle UMETA(DisplayName = "Rifle"),
	ItemName_ShotGun UMETA(DisplayName = "ShotGun"),
	ItemName_RocketLauncher UMETA(DisplayName = "RocketLauncher")
};
ENUM_RANGE_BY_FIRST_AND_LAST(EItemName, EItemName::ItemName_Rifle, EItemName::ItemName_RocketLauncher);

UCLASS()
class FPS_BORANAGAJIN_API AItemName : public AActor
{
	GENERATED_BODY()
public:
	AItemName();
};