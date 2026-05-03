

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraShakeBase.h"
#include "WeaponCamShakeBase.generated.h"

class UPerlinNoiseCameraShakePattern;

UCLASS()
class FPS_BORANAGAJIN_API UWeaponCamShakeBase : public UCameraShakeBase
{
	GENERATED_BODY()
public:
	UWeaponCamShakeBase();
protected:
	UPROPERTY(EditAnywhere, Category = "Camera Shake")
	TObjectPtr<UPerlinNoiseCameraShakePattern> ShakePattern_Perlin;
};