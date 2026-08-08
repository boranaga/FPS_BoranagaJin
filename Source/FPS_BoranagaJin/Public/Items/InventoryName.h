// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InventoryName.generated.h"

UENUM(BlueprintType)
enum class EInventoryName : uint8
{
	Item UMETA(DisplayName = "Itme"),
	Weapon UMETA(DisplayName = "Weapon"),
	ThrowableWeapon UMETA(DisplayName = "ThrowableWeapon")
};
ENUM_RANGE_BY_FIRST_AND_LAST(EInventoryName, EInventoryName::Item, EInventoryName::ThrowableWeapon);