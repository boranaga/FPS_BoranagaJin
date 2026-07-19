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
	UIType_Base UMETA(DisplayName = "BaseUIWidget"),
	UIType_WeaponAim UMETA(DisplayName = "WeaponAimUIWidget"),
	UIType_AmmoCounter UMETA(DisplayName = "AmmoCounterWidget"),
	UIType_Stamina UMETA(DisplayName = "StaminaWidget"),
	UIType_Health UMETA(DisplayName = "HealthWidget"),
	UIType_Inventory UMETA(DisplayName = "InventoryWidget"),
	UIType_ThrowableWeaponInventory UMETA(DisplayName = "ThrowableWeaponInventoryWidget"),
	UIType_Interaction UMETA(DisplayName = "InteractionWidget")
};
ENUM_RANGE_BY_FIRST_AND_LAST(EUIType, EUIType::UIType_Base, EUIType::UIType_Interaction);

UCLASS()
class FPS_BORANAGAJIN_API AUIType : public AActor
{
	GENERATED_BODY()
public:
	AUIType();
};