#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "EnemyCombatNoiseSubsystem.generated.h"

/** Only the perception controller interprets these types; they do not reveal the source actor's location. */
enum class EEnemyCombatNoiseType : uint8
{
    Gunshot,
    ProjectileImpact,
    PayloadActivation
};

/** Gameplay metadata associated with one UAISense_Hearing stimulus. */
struct FEnemyCombatNoiseEvent
{
    EEnemyCombatNoiseType Type = EEnemyCombatNoiseType::Gunshot;
    FVector Location = FVector::ZeroVector;
    FVector TravelDirection = FVector::ZeroVector;
    bool bMayInferSourceFromTravel = false;
    float Loudness = 1.f;
    float MaxRange = 0.f;
    float TimeSeconds = 0.f;
    TWeakObjectPtr<AActor> Instigator;
};

/**
 * Per-world, bounded, short-lived metadata storage.
 * UAISense_Hearing owns detection/range; this subsystem only attaches game details
 * (type and incoming trajectory) to the FAIStimulus.Tag.
 */
UCLASS()
class FPS_BORANAGAJIN_API UEnemyCombatNoiseSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Deinitialize() override;

    /** Register BEFORE ReportNoiseEvent. Returns a bounded FName identifier. */
    FName RegisterNoise(const FEnemyCombatNoiseEvent& Event);

    /** The location and actor must match the delivered stimulus (pool/slot safety). */
    bool ResolveNoise(FName Tag, const FVector& StimulusLocation,
        const AActor* StimulusActor, FEnemyCombatNoiseEvent& OutEvent) const;

    static bool IsCombatNoiseTag(FName Tag);

private:
    void PruneExpired();

    /** At most 4096 distinct FNames are created, even during hours of full-auto firing. */
    static constexpr uint32 TagSlotCount = 4096;
    static constexpr float RecordLifetime = 5.f;
    uint32 NextTagSerial = 0;
    TMap<FName, FEnemyCombatNoiseEvent> RecentEvents;
};
