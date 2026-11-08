#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponName.generated.h"

UENUM(BlueprintType)
enum class EWeaponName : uint8
{
	// Existing numeric values are kept for the original weapons.
	WeaponName_Rifle UMETA(DisplayName = "Rifle"),
	WeaponName_ShotGun UMETA(DisplayName = "ShotGun"),
	WeaponName_RocketLauncher UMETA(DisplayName = "RocketLauncher"),
	WeaponName_Pistol UMETA(DisplayName = "Pistol"),
	WeaponName_FlareGun UMETA(DisplayName = "FlareGun")
	//WeaponName_RailGun UMETA(DisplayName = "RailGun"),
	//WeaponName_EnergyRifle UMETA(DisplayName = "EnergyRifle")
};
ENUM_RANGE_BY_FIRST_AND_LAST(EWeaponName, EWeaponName::WeaponName_Rifle, EWeaponName::WeaponName_FlareGun);

UCLASS()
class FPS_BORANAGAJIN_API AWeaponName : public AActor
{
	GENERATED_BODY()
public:
	AWeaponName();
};
