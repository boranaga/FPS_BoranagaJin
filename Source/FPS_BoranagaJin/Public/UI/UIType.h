#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UIType.generated.h"

UENUM(BlueprintType)
enum class EUIZOrder : uint8
{
	Stamina = 0,
	Interaction = 1,
	WeaponAim = 2,
	Inventory = 100,
	ItemTool = 101,
	ThrowableWeaponInventory = 102,
};

UENUM(BlueprintType)
enum class EUIType : uint8
{
	Base UMETA(DisplayName = "BaseUIWidget"),
	MainMenu UMETA(DisplayName = "MainMenu"),
	WeaponAim UMETA(DisplayName = "WeaponAimUIWidget"),
	AmmoCounter UMETA(DisplayName = "AmmoCounterWidget"),
	Stamina UMETA(DisplayName = "StaminaWidget"),
	Health UMETA(DisplayName = "HealthWidget"),
	Inventory UMETA(DisplayName = "InventoryWidget"),
	ThrowableWeaponInventory UMETA(DisplayName = "ThrowableWeaponInventoryWidget"),
	Interaction UMETA(DisplayName = "InteractionWidget")
};
ENUM_RANGE_BY_FIRST_AND_LAST(EUIType, EUIType::Base, EUIType::Interaction);

UCLASS()
class FPS_BORANAGAJIN_API AUIType : public AActor
{
	GENERATED_BODY()
public:
	AUIType();
};