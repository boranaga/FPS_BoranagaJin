#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Templates/SubclassOf.h"
#include "CharacterComponentSettings.generated.h"

class ABloodStainActor;

/** UHealthComponent configuration. Runtime CurrentHealth is NOT a DataTable field. */
USTRUCT(BlueprintType)
struct FPS_BORANAGAJIN_API FCharacterHealthSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health", meta = (ClampMin = "1.0"))
    float MaxHealth = 100.f;

    // These three variables are the actual bleeding decision policy used by
    // UHealthComponent::ApplyDamage() in the supplied project.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health - Bleeding", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float BleedingHealthThresholdPercent = 0.6f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health - Bleeding", meta = (ClampMin = "0.0"))
    float HeavyDamageThreshold = 25.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health - Bleeding", meta = (ClampMin = "0.0"))
    float BleedingDurationAfterHeavyDamage = 8.f;
};

/** UStaminaComponent configuration. Runtime CurrentStamina is not a preset. */
USTRUCT(BlueprintType)
struct FPS_BORANAGAJIN_API FCharacterStaminaSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina", meta = (ClampMin = "1.0"))
    float MaxStamina = 1000.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina", meta = (ClampMin = "0.0"))
    float RecoveryRate = 50.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina", meta = (ClampMin = "0.0"))
    float RecoveryDelay = 1.f;

    /** Points consumed per second while running; shared UStaminaComponent is the only pool. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina - Running", meta = (ClampMin = "0.0"))
    float RunStaminaCostPerSecond = 50.f;

    /** The minimum points required to start/maintain normal running. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina - Running", meta = (ClampMin = "0.0"))
    float MinimumRunStamina = 50.f;

    /** One-time positive cost whenever the player begins a dash, including from a slide. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina - Dash", meta = (ClampMin = "0.0"))
    float DashStaminaCost = 100.f;
};

/** UBloodTrailComponent configuration. BloodStainClass may be left unset in the
 * DataTable to preserve the existing class selected in the component Blueprint.
 * IMPORTANT: the duplicated thresholds here were unused in the uploaded
 * BloodTrailComponent logic; the Health settings above trigger bleeding.
 */
USTRUCT(BlueprintType)
struct FPS_BORANAGAJIN_API FCharacterBloodTrailSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood Trail")
    TSubclassOf<ABloodStainActor> BloodStainClass = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood Trail - Legacy", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float BleedingHealthThresholdPercent = 0.6f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood Trail - Legacy", meta = (ClampMin = "0.0"))
    float HeavyDamageThreshold = 25.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood Trail - Spawn", meta = (ClampMin = "0.01"))
    float MinimumBloodDropInterval = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood Trail - Spawn", meta = (ClampMin = "0.0"))
    float MinimumBloodDropDistance = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood Trail - Spawn", meta = (ClampMin = "1.0"))
    float GroundTraceDistance = 250.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood Trail - Legacy", meta = (ClampMin = "0.0"))
    float BleedingDurationAfterHeavyDamage = 8.f;

    // The component constructor used to hard-code this to 0.1 seconds.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood Trail - Performance", meta = (ClampMin = "0.01"))
    float BloodTrailTickInterval = 0.1f;
};

/** One row represents all three components of ONE actor/preset.
 * Assign the FDataTableRowHandle to AEnemyBase or ACharacterPlayer.
 */
USTRUCT(BlueprintType)
struct FPS_BORANAGAJIN_API FCharacterComponentSettingsRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
    FCharacterHealthSettings Health;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina")
    FCharacterStaminaSettings Stamina;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood Trail")
    FCharacterBloodTrailSettings BloodTrail;
};
