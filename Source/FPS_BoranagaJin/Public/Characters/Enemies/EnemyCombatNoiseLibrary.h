#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "EnemyCombatNoiseLibrary.generated.h"

/**
 * Produces ONE UAISense_Hearing event per gameplay sound.
 * Enemy AIController receives it through AIPerception and delegates behavior
 * to EnemyStateMachineComponent. No TActorIterator / direct Enemy notifications.
 */
UCLASS()
class FPS_BORANAGAJIN_API UEnemyCombatNoiseLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Enemy|Combat Noise",
        meta = (WorldContext = "WorldContextObject"))
    static void ReportGunshot(UObject* WorldContextObject, FVector MuzzleLocation,
        AActor* Shooter, float Loudness = 1.f, float MaxHearingRange = 4000.f);

    /** Pass pre-bounce velocity direction, or ZeroVector when unknown. */
    UFUNCTION(BlueprintCallable, Category = "Enemy|Combat Noise",
        meta = (WorldContext = "WorldContextObject"))
    static void ReportProjectileImpact(UObject* WorldContextObject, FVector ImpactLocation,
        AActor* ProjectileOwner, FVector ProjectileTravelDirection,
        float Loudness = 1.f, float MaxHearingRange = 2200.f,
        bool bMayInferShooterFromDirection = true);

    /** Fuse/flash/smoke activation sound: not confused with a wall impact. */
    UFUNCTION(BlueprintCallable, Category = "Enemy|Combat Noise",
        meta = (WorldContext = "WorldContextObject"))
    static void ReportPayloadActivation(UObject* WorldContextObject, FVector Location,
        AActor* ProjectileOwner, float Loudness = 1.f, float MaxHearingRange = 2500.f);
};
