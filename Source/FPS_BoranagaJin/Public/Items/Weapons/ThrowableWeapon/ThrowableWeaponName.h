#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ThrowableWeaponName.generated.h"

UENUM(BlueprintType)
enum class EThrowableWeaponName : uint8
{

	ThrowableWeaponName_Base UMETA(DisplayName = "Base"),
	ThrowableWeaponName_Grenade UMETA(DisplayName = "Grenade"),
	ThrowableWeaponName_SmokeGrenade UMETA(DisplayName = "SmokeGrenade")

};
ENUM_RANGE_BY_FIRST_AND_LAST(EThrowableWeaponName, EThrowableWeaponName::ThrowableWeaponName_Base, EThrowableWeaponName::ThrowableWeaponName_SmokeGrenade);

UCLASS()
class FPS_BORANAGAJIN_API AThrowableWeaponName : public AActor
{
	GENERATED_BODY()
public:
	AThrowableWeaponName();
};