#include "Characters/Enemies/EnemyCombatNoiseSubsystem.h"

#include "Engine/World.h"

void UEnemyCombatNoiseSubsystem::Deinitialize()
{
    RecentEvents.Empty();
    Super::Deinitialize();
}

FName UEnemyCombatNoiseSubsystem::RegisterNoise(const FEnemyCombatNoiseEvent& Event)
{
    UWorld* World = GetWorld();
    if (!World) return NAME_None;

    if ((NextTagSerial % 64u) == 0u)
    {
        PruneExpired();
    }

    const uint32 Slot = (NextTagSerial++) % TagSlotCount;
    const FName Tag(*FString::Printf(TEXT("CN_%04u"), Slot));
    FEnemyCombatNoiseEvent Copy = Event;
    Copy.TimeSeconds = World->GetTimeSeconds();
    RecentEvents.Add(Tag, MoveTemp(Copy));
    return Tag;
}

bool UEnemyCombatNoiseSubsystem::ResolveNoise(
    FName Tag, const FVector& StimulusLocation, const AActor* StimulusActor,
    FEnemyCombatNoiseEvent& OutEvent) const
{
    const UWorld* World = GetWorld();
    if (!World || !IsCombatNoiseTag(Tag)) return false;

    const FEnemyCombatNoiseEvent* Found = RecentEvents.Find(Tag);
    if (!Found || World->GetTimeSeconds() - Found->TimeSeconds > RecordLifetime)
    {
        return false;
    }

    // Distinct events could reuse a bounded slot after 4096 writes. A stale
    // stimulus must never resolve to an unrelated newer shot/impact.
    if (Found->Instigator.Get() != StimulusActor ||
        FVector::DistSquared(Found->Location, StimulusLocation) > FMath::Square(5.f))
    {
        return false;
    }

    OutEvent = *Found;
    return true;
}

bool UEnemyCombatNoiseSubsystem::IsCombatNoiseTag(FName Tag)
{
    return Tag.ToString().StartsWith(TEXT("CN_"));
}

void UEnemyCombatNoiseSubsystem::PruneExpired()
{
    const UWorld* World = GetWorld();
    if (!World) return;

    const float Now = World->GetTimeSeconds();
    for (auto It = RecentEvents.CreateIterator(); It; ++It)
    {
        if (Now - It.Value().TimeSeconds > RecordLifetime)
        {
            It.RemoveCurrent();
        }
    }
}
