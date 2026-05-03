

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PlayerSoundData.generated.h"

USTRUCT(BlueprintType)
struct FPlayerSoundData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundBase> Sound;

	UPROPERTY(EditAnywhere)
	bool bDebug = false;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bDebug"))
	float DebugDisplayDuration = 3.f;

	UPROPERTY(EditAnywhere)
	bool bMapVolume = true;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bMapVolume"))
	FFloatInterval VolumeRange = FFloatInterval(0.f, 1.f);
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bMapVolume"))
	FFloatInterval VolumeSpeedRange = FFloatInterval(0.f, 1000.f);
	UPROPERTY(EditAnywhere)
	bool bMapPitch = true;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bMapPitch"))
	FFloatInterval PitchRange = FFloatInterval(0.f, 1.f);
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bMapPitch"))
	FFloatInterval PitchSpeedRange = FFloatInterval(0.f, 1000.f);
};

/**
 *
 */
UCLASS()
class FPS_BORANAGAJIN_API UPlayerSound_DataAsset : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, Category = "Hit")
	TObjectPtr<USoundBase> BossNormalHitSound;

	UPROPERTY(EditAnywhere, Category = "Hit")
	TObjectPtr<USoundBase> BossDownedHitSound;

	UPROPERTY(EditAnywhere, Category = "Hit")
	TObjectPtr<USoundBase> MeleeEnemyHitSound;

	UPROPERTY(EditAnywhere, Category = "Hit")
	TObjectPtr<USoundBase> RifleEnemyHitSound;

	UPROPERTY(EditAnywhere, Category = "Hit")
	TObjectPtr<USoundBase> ChargerEnemyHitSound;

	UPROPERTY(EditAnywhere, Category = "Hit")
	TObjectPtr<USoundBase> TurretEnemyHitSound;

	UPROPERTY(EditAnywhere, Category = "Movement")
	FPlayerSoundData PrimaryJumpSound;
	UPROPERTY(EditAnywhere, Category = "Movement")
	FPlayerSoundData DoubleJumpSound;
	UPROPERTY(EditAnywhere, Category = "Movement")
	FPlayerSoundData WallRunSound;
	UPROPERTY(EditAnywhere, Category = "Movement")
	FPlayerSoundData SlideSound;
	UPROPERTY(EditAnywhere, Category = "Movement")
	FPlayerSoundData LandSound;
	UPROPERTY(EditAnywhere, Category = "Movement")
	FPlayerSoundData DashSound;
};
