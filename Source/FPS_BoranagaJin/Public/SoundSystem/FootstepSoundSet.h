#pragma once

#include "CoreMinimal.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "SoundSystem/SoundID.h"
#include "FootstepSoundSet.generated.h"

USTRUCT(BlueprintType)
struct FFootstepSoundSet
{
	GENERATED_BODY()
public:
	ESoundID FindSoundID(EPhysicalSurface SurfaceType) const;
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footstep")
	ESoundID DefaultSoundID = ESoundID::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footstep")
	TMap<TEnumAsByte<EPhysicalSurface>, ESoundID> SurfaceSoundMap;
};