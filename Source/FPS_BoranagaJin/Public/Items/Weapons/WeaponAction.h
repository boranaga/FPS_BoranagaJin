

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponAction.generated.h"

UENUM(BlueprintType)
enum class EWeaponAction : uint8
{
	WeaponAction_SingleShot UMETA(DisplayName = "SingleShot"),
	WeaponAction_BurstShot UMETA(DisplayName = "BurstShot"),
	WeaponAction_FullAutoShot UMETA(DisplayName = "FullAutoShot"),
	WeaponAction_Charge UMETA(DisplayName = "Charge"),
	WeaponAction_Hold UMETA(DisplayName = "Hold"),
	WeaponAction_Zoom UMETA(DisplayName = "Zoom"),
	WeaponAction_SkillToggle UMETA(DisplayName = "SkillToggle"),
	WeaponAction_SkillFire UMETA(DisplayName = "SkillFire"),
	WeaponAction_None UMETA(DisplayName = "None")
};

UCLASS()
class FPS_BORANAGAJIN_API AWeaponAction : public AActor
{
	GENERATED_BODY()
public:
	AWeaponAction();
};