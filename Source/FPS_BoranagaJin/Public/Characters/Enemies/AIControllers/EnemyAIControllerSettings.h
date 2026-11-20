#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "EnemyAIControllerSettings.generated.h"

/**
 * Per-enemy AIController tuning profile.
 * Runtime references, timers, path request IDs and perception memory are not settings.
 * These categories are flat for compatibility with the DataTable row editor.
 */
USTRUCT(BlueprintType)
struct FPS_BORANAGAJIN_API FEnemyAIControllerSettings : public FTableRowBase
{
    GENERATED_BODY()

public:
    // SightRadius
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception - Sight", meta = (ClampMin = "0.0"))
    float SightRadius = 2000.f;

    // LoseSightRadius
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception - Sight", meta = (ClampMin = "0.0"))
    float LoseSightRadius = 5000.f;

    // PeripheralVisionAngleDegrees
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception - Sight", meta = (ClampMin = "0.0", ClampMax = "180.0"))
    float PeripheralVisionAngleDegrees = 120.f;

    // SightStimulusMaxAge
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception - Sight", meta = (ClampMin = "0.0"))
    float SightStimulusMaxAge = 10.f;

    // bSightDetectEnemies
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception - Sight")
    bool bSightDetectEnemies = true;

    // bSightDetectFriendlies
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception - Sight")
    bool bSightDetectFriendlies = true;

    // bSightDetectNeutrals
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception - Sight")
    bool bSightDetectNeutrals = true;

    // HearingRange
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception - Hearing", meta = (ClampMin = "0.0"))
    float HearingRange = 3500.f;

    // LoSHearingRange
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception - Hearing", meta = (ClampMin = "0.0"))
    float LoSHearingRange = 3500.f;

    // HearingStimulusMaxAge
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception - Hearing", meta = (ClampMin = "0.0"))
    float HearingStimulusMaxAge = 5.f;

    // bHearingDetectEnemies
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception - Hearing")
    bool bHearingDetectEnemies = true;

    // bHearingDetectFriendlies
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception - Hearing")
    bool bHearingDetectFriendlies = true;

    // bHearingDetectNeutrals
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception - Hearing")
    bool bHearingDetectNeutrals = true;

    // bIgnoreEnemyGeneratedNoise
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception - Hearing")
    bool bIgnoreEnemyGeneratedNoise = true;

    // LightObservationInterval
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception - Light", meta = (ClampMin = "0.1"))
    float LightObservationInterval = 0.3f;

    // LightSourceDetectionRange
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception - Light", meta = (ClampMin = "100.0"))
    float LightSourceDetectionRange = 2400.f;

    // LightPatchDetectionRange
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception - Light", meta = (ClampMin = "100.0"))
    float LightPatchDetectionRange = 2400.f;

    // LightObservationHalfFOV
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception - Light", meta = (ClampMin = "0.0", ClampMax = "179.0"))
    float LightObservationHalfFOV = 100.f;

    // MinimumVisibleLightIntensity
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception - Light", meta = (ClampMin = "0.0"))
    float MinimumVisibleLightIntensity = 1.f;

    // LightMaxBeamTraceLength
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception - Light", meta = (ClampMin = "100.0"))
    float LightMaxBeamTraceLength = 2500.f;

    // LightBeamTraceCount
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception - Light", meta = (ClampMin = "1", ClampMax = "5"))
    int32 LightBeamTraceCount = 5;

    // LightEstimateLocationError
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception - Light", meta = (ClampMin = "0.0"))
    float LightEstimateLocationError = 130.f;

    // LightObservationReportCooldown
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception - Light", meta = (ClampMin = "0.1"))
    float LightObservationReportCooldown = 0.85f;

    // LightObservationRetargetDistance
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception - Light", meta = (ClampMin = "20.0"))
    float LightObservationRetargetDistance = 200.f;

    // bDetectLightOnSurfaces
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception - Light")
    bool bDetectLightOnSurfaces = true;

    // bDrawLightObservationDebug
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception - Light")
    bool bDrawLightObservationDebug = false;

    // GroundTraceHeight
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement - Ground", meta = (ClampMin = "0.0"))
    float GroundTraceHeight = 300.f;

    // GroundTraceDepth
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement - Ground", meta = (ClampMin = "0.0"))
    float GroundTraceDepth = 500.f;

    // NavProjectionExtent
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement - Ground")
    FVector NavProjectionExtent = FVector(100.f, 100.f, 300.f);

    // ChaseMoveAcceptanceRadius
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement - Chase", meta = (ClampMin = "30.0"))
    float ChaseMoveAcceptanceRadius = 75.f;

    // ChaseMoveRetryInterval
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement - Chase", meta = (ClampMin = "0.1"))
    float ChaseMoveRetryInterval = 0.65f;

    // ChaseFallbackRetargetDistance
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement - Chase", meta = (ClampMin = "20.0"))
    float ChaseFallbackRetargetDistance = 170.f;

    // ChaseStuckTime
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement - Chase", meta = (ClampMin = "0.5"))
    float ChaseStuckTime = 2.25f;

    // ChaseProgressThreshold
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement - Chase", meta = (ClampMin = "1.0"))
    float ChaseProgressThreshold = 10.f;

    // ChaseNavProjectionExtent
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement - Chase")
    FVector ChaseNavProjectionExtent = FVector(250.f, 250.f, 400.f);

    // bLogChaseMoveDiagnostics
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement - Diagnostics")
    bool bLogChaseMoveDiagnostics = true;
};
