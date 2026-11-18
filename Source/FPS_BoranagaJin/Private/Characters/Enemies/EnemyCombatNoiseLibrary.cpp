#include "Characters/Enemies/EnemyCombatNoiseLibrary.h"
#include "Characters/Enemies/EnemyCombatNoiseSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Perception/AISense_Hearing.h"

namespace
{
    void EmitCombatSound(UObject* WorldContextObject, const FVector& Location,
        AActor* Instigator, EEnemyCombatNoiseType Type, const FVector& TravelDirection,
        float Loudness, float MaxRange, bool bMayInferSource)
    {
        if (!IsValid(WorldContextObject) || !IsValid(Instigator) ||
            !GEngine || Loudness <= 0.f || MaxRange <= 0.f)
        {
            return;
        }

        UWorld* World = GEngine->GetWorldFromContextObject(
            WorldContextObject, EGetWorldErrorMode::ReturnNull);
        if (!World || !World->IsGameWorld() || World->GetNetMode() == NM_Client)
        {
            return; // AI hearing is server authoritative
        }

        UEnemyCombatNoiseSubsystem* Store = World->GetSubsystem<UEnemyCombatNoiseSubsystem>();
        if (!Store) return;

        FEnemyCombatNoiseEvent Event;
        Event.Type = Type;
        Event.Location = Location;
        Event.TravelDirection = TravelDirection.GetSafeNormal();
        Event.bMayInferSourceFromTravel = bMayInferSource;
        Event.Loudness = Loudness;
        Event.MaxRange = MaxRange;
        Event.Instigator = Instigator;

        const FName Tag = Store->RegisterNoise(Event);
        if (Tag.IsNone()) return;

        // Only ONE perception event. Listener range & MaxAge are owned by AI Perception.
        UAISense_Hearing::ReportNoiseEvent(
            World, Location, Loudness, Instigator, MaxRange, Tag);
    }
}

void UEnemyCombatNoiseLibrary::ReportGunshot(UObject* WorldContextObject,
    FVector MuzzleLocation, AActor* Shooter, float Loudness, float MaxHearingRange)
{
    EmitCombatSound(WorldContextObject, MuzzleLocation, Shooter,
        EEnemyCombatNoiseType::Gunshot, FVector::ZeroVector,
        Loudness, MaxHearingRange, false);
}

void UEnemyCombatNoiseLibrary::ReportProjectileImpact(UObject* WorldContextObject,
    FVector ImpactLocation, AActor* ProjectileOwner, FVector ProjectileTravelDirection,
    float Loudness, float MaxHearingRange, bool bMayInferShooterFromDirection)
{
    EmitCombatSound(WorldContextObject, ImpactLocation, ProjectileOwner,
        EEnemyCombatNoiseType::ProjectileImpact, ProjectileTravelDirection,
        Loudness, MaxHearingRange, bMayInferShooterFromDirection);
}

void UEnemyCombatNoiseLibrary::ReportPayloadActivation(UObject* WorldContextObject,
    FVector Location, AActor* ProjectileOwner, float Loudness, float MaxHearingRange)
{
    EmitCombatSound(WorldContextObject, Location, ProjectileOwner,
        EEnemyCombatNoiseType::PayloadActivation, FVector::ZeroVector,
        Loudness, MaxHearingRange, false);
}
