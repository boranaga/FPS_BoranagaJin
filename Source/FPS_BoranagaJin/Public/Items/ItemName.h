#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemName.generated.h"

UENUM(BlueprintType)
enum class EItemName : uint8
{
	ItemName_Base UMETA(DisplayName = "BaseItem"),
	ItemName_Rifle UMETA(DisplayName = "Rifle"),
	ItemName_ShotGun UMETA(DisplayName = "ShotGun"),
	ItemName_RocketLauncher UMETA(DisplayName = "RocketLauncher"),
	ItemName_Grenade UMETA(DisplayName = "Grenade"),
	ItemName_Bandage UMETA(DisplayName = "Bandage"),
	ItemName_Dummy UMETA(DisplayName = "DummyItem"),
	ItemName_Dummy_2 UMETA(DisplayName = "DummyItem_2"),
	ItemName_None UMETA(DisplayName = "None")
};
ENUM_RANGE_BY_FIRST_AND_LAST(EItemName, EItemName::ItemName_Base, EItemName::ItemName_None);

UCLASS()
class FPS_BORANAGAJIN_API AItemName : public AActor
{
	GENERATED_BODY()
public:
	AItemName();
};