// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponName.generated.h"

UENUM(BlueprintType)
enum class EWeaponName : uint8
{
	//WeaponName_Pistol UMETA(DisplayName = "Pistol"),
	WeaponName_Rifle UMETA(DisplayName = "Rifle"),
	WeaponName_ShotGun UMETA(DisplayName = "ShotGun")
	//WeaponName_MissileLauncher UMETA(DisplayName = "MissileLauncher"),
	//WeaponName_RailGun UMETA(DisplayName = "RailGun"),
	//WeaponName_EnergyRifle UMETA(DisplayName = "EnergyRifle")
};
ENUM_RANGE_BY_FIRST_AND_LAST(EWeaponName, EWeaponName::WeaponName_Rifle, EWeaponName::WeaponName_ShotGun);

UCLASS()
class FPS_BORANAGAJIN_API AWeaponName : public AActor
{
	GENERATED_BODY()
public:
	AWeaponName();
};