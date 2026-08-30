#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SoundID.h"
#include "GameAudioSubsystem.generated.h"

class UAudioComponent;
class UGameSoundDataAsset;
class USoundBase;

UCLASS()
class FPS_BORANAGAJIN_API UGameAudioSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
public:
    void PlaySound2D(ESoundID SoundID);
    UAudioComponent* PlaySoundAtLocation(ESoundID SoundID, const FVector& Location);
    UAudioComponent* PlaySoundAttached(ESoundID SoundID, USceneComponent* AttachComponent);
#pragma region BGM
public:
    void PlayBGM(ESoundID SoundID, float FadeInDuration = 1.f);
    void StopBGM(float FadeOutDuration = 1.f);
private:
    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> CurrentBGMComponent;
#pragma endregion

#pragma region SoundData
private:
    const struct FGameSoundData* GetSoundData(ESoundID SoundID) const;

private:
    UPROPERTY(EditDefaultsOnly, Category = "Audio")
    TObjectPtr<UGameSoundDataAsset> SoundDataAsset;
#pragma endregion
};