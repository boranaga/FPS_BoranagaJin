
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponStateType.generated.h"

UENUM(BlueprintType)
enum class EWeaponStateType : uint8
{
	WeaponStateType_None UMETA(DisplayName = "None"),
	WeaponStateType_Idle UMETA(DisplayName = "Idle"),
	WeaponStateType_Firing UMETA(DisplayName = "Firing"),
	WeaponStateType_FullAutoFiring UMETA(DisplayName = "FullAutoFiring"),
	WeaponStateType_Reloading UMETA(DisplayName = "Reloading"),
	WeaponStateType_PumpActionReloading UMETA(DisplayName = "PumpActionReloading"),
	WeaponStateType_Unequipped UMETA(DisplayName = "Unequipped"),
	WeaponStateType_Switching UMETA(DisplayName = "Switching"),
	WeaponStateType_Targeting UMETA(DisplayName = "Targeting"),
	WeaponStateType_Charging UMETA(DisplayName = "Charging"),
	WeaponStateType_Waiting UMETA(DisplayName = "Waiting")
};

UCLASS()
class FPS_BORANAGAJIN_API AWeaponStateType : public AActor
{
	GENERATED_BODY()
public:
	AWeaponStateType();
};
