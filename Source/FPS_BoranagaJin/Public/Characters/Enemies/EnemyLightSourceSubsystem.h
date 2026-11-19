#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "EnemyLightSourceSubsystem.generated.h"

class AActor;
class USpotLightComponent;

/** Lightweight registration; does NOT make the player a Sight stimulus. */
struct FEnemyRegisteredSpotLight
{
    TWeakObjectPtr<USpotLightComponent> Light;
    TWeakObjectPtr<AActor> EmitterActor;
    TWeakObjectPtr<AActor> PlayerActor;
};

/**
 * Registered player-operated spotlights. No global actor search per enemy.
 * Other player-held lamps can register their SpotLightComponent here, too.
 * Does not simulate reflected light; the observer tests visible illuminated surfaces.
 */
UCLASS()
class FPS_BORANAGAJIN_API UEnemyLightSourceSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    void RegisterPlayerSpotLight(
        USpotLightComponent* Light, AActor* EmitterActor, AActor* PlayerActor);
    void UnregisterPlayerSpotLight(const USpotLightComponent* Light);
    void GetActivePlayerSpotLights(TArray<FEnemyRegisteredSpotLight>& OutSources) const;

    virtual void Deinitialize() override;
private:
    TArray<FEnemyRegisteredSpotLight> Sources;
};
