#include "SoundSystem/GameAudioSubsystem.h"

#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"

#include "SoundSystem/GameAudioSettings.h"
#include "SoundSystem/GameSoundDataAsset.h"
#include "SoundSystem/SoundData.h"

void UGameAudioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    const UGameAudioSettings* Settings = GetDefault<UGameAudioSettings>();

    if (!Settings)
    {
        return;
    }

    SoundDataAsset = Settings->SoundDataAsset.LoadSynchronous();
}

void UGameAudioSubsystem::Deinitialize()
{
    StopBGM(0.f);

    SoundDataAsset = nullptr;

    Super::Deinitialize();
}

void UGameAudioSubsystem::PlaySound2D(ESoundID SoundID)
{
    const FGameSoundData* SoundData = GetSoundData(SoundID);
    if (!SoundData) { return; }
    if (!IsValid(SoundData->Sound)) { return; }

    UGameplayStatics::PlaySound2D(
        this,
        SoundData->Sound,
        SoundData->GetRandomizedVolume(),
        SoundData->GetRandomizedPitch(),
        0.f,
        SoundData->Concurrency
    );
}

UAudioComponent* UGameAudioSubsystem::PlaySoundAtLocation(ESoundID SoundID, const FVector& Location)
{
    const FGameSoundData* SoundData = GetSoundData(SoundID);

    if (!SoundData)
    {
        return nullptr;
    }

    if (!IsValid(SoundData->Sound))
    {
        return nullptr;
    }

    return UGameplayStatics::SpawnSoundAtLocation(
        this,
        SoundData->Sound,
        Location,
        FRotator::ZeroRotator,
        SoundData->GetRandomizedVolume(),
        SoundData->GetRandomizedPitch(),
        0.f,
        SoundData->Attenuation,
        SoundData->Concurrency,
        true
    );
}

UAudioComponent* UGameAudioSubsystem::PlaySoundAttached(ESoundID SoundID, USceneComponent* AttachComponent)
{
    if (!IsValid(AttachComponent))
    {
        return nullptr;
    }

    const FGameSoundData* SoundData = GetSoundData(SoundID);

    if (!SoundData || !IsValid(SoundData->Sound))
    {
        return nullptr;
    }

    return UGameplayStatics::SpawnSoundAttached(
        SoundData->Sound,
        AttachComponent,
        NAME_None,
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        EAttachLocation::KeepRelativeOffset,
        true,
        SoundData->GetRandomizedVolume(),
        SoundData->GetRandomizedPitch(),
        0.f,
        SoundData->Attenuation,
        SoundData->Concurrency,
        true
    );
}

void UGameAudioSubsystem::PlayBGM(ESoundID SoundID, float FadeInDuration)
{
    const FGameSoundData* SoundData = GetSoundData(SoundID);

    if (!SoundData || !IsValid(SoundData->Sound))
    {
        return;
    }

    if (IsValid(CurrentBGMComponent))
    {
        CurrentBGMComponent->FadeOut(1.f, 0.f);
        CurrentBGMComponent = nullptr;
    }

    CurrentBGMComponent = UGameplayStatics::SpawnSound2D(
        this,
        SoundData->Sound,
        SoundData->VolumeMultiplier,
        SoundData->PitchMultiplier,
        0.f,
        SoundData->Concurrency,
        true,
        true
    );

    if (IsValid(CurrentBGMComponent))
    {
        CurrentBGMComponent->FadeIn(
            FadeInDuration,
            SoundData->VolumeMultiplier
        );
    }
}

void UGameAudioSubsystem::StopBGM(float FadeOutDuration)
{
    if (!IsValid(CurrentBGMComponent))
    {
        return;
    }

    if (FadeOutDuration <= 0.f)
    {
        CurrentBGMComponent->Stop();
    }
    else
    {
        CurrentBGMComponent->FadeOut(FadeOutDuration, 0.f);
    }

    CurrentBGMComponent = nullptr;
}

const FGameSoundData* UGameAudioSubsystem::GetSoundData(ESoundID SoundID) const
{
    if (!IsValid(SoundDataAsset))
    {
        return nullptr;
    }

    return SoundDataAsset->FindSoundData(SoundID);
}