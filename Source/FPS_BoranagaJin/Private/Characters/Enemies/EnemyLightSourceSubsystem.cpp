#include "Characters/Enemies/EnemyLightSourceSubsystem.h"
#include "Items/FlashlightItem.h"

#include "Components/SpotLightComponent.h"
#include "GameFramework/Actor.h"

void UEnemyLightSourceSubsystem::RegisterPlayerSpotLight(
    USpotLightComponent* Light, AActor* EmitterActor, AActor* PlayerActor)
{
    if (!IsValid(Light) || !IsValid(EmitterActor) || !IsValid(PlayerActor)) return;

    for (FEnemyRegisteredSpotLight& Entry : Sources)
    {
        if (Entry.Light.Get() == Light)
        {
            Entry.EmitterActor = EmitterActor;
            Entry.PlayerActor = PlayerActor;
            return;
        }
    }

    FEnemyRegisteredSpotLight Entry;
    Entry.Light = Light;
    Entry.EmitterActor = EmitterActor;
    Entry.PlayerActor = PlayerActor;
    Sources.Add(MoveTemp(Entry));
}

void UEnemyLightSourceSubsystem::UnregisterPlayerSpotLight(const USpotLightComponent* Light)
{
    Sources.RemoveAll([Light](const FEnemyRegisteredSpotLight& Entry)
    {
        return !Entry.Light.IsValid() || Entry.Light.Get() == Light;
    });
}

void UEnemyLightSourceSubsystem::GetActivePlayerSpotLights(
    TArray<FEnemyRegisteredSpotLight>& OutSources) const
{
    OutSources.Reset();
    for (const FEnemyRegisteredSpotLight& Entry : Sources)
    {
        const USpotLightComponent* Light = Entry.Light.Get();
        const AActor* Emitter = Entry.EmitterActor.Get();
        const AActor* Player = Entry.PlayerActor.Get();
        if (!IsValid(Light) || !IsValid(Emitter) || !IsValid(Player) ||
            !Light->IsRegistered() || !Light->IsVisible() || Light->Intensity <= 0.f)
        {
            continue;
        }

        // Flashlight-specific power/equip/battery rules; flicker uses IsVisible().
        if (const AFlashlightItem* Flashlight = Cast<AFlashlightItem>(Emitter))
        {
            if (!Flashlight->IsEquipped() || !Flashlight->IsFlashlightOn() ||
                !Flashlight->HasBatteryCharge())
            {
                continue;
            }
        }
        OutSources.Add(Entry);
    }
}

void UEnemyLightSourceSubsystem::Deinitialize()
{
    Sources.Reset();
    Super::Deinitialize();
}
