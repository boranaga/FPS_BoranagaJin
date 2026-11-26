#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GasSystemSubsystem.generated.h"

class AGasRoomVolume;
class AGasPipe;
class APawn;

/** Single fixed-cadence gameplay simulation and room registry per world. */
UCLASS()
class FPS_BORANAGAJIN_API UGasSystemSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual void Deinitialize() override;
    void RegisterRoom(AGasRoomVolume* Room);
    void UnregisterRoom(AGasRoomVolume* Room);
    void RegisterPipe(AGasPipe* Pipe);
    void UnregisterPipe(AGasPipe* Pipe);
    AGasRoomVolume* FindRoomAt(const FVector& Point) const;
    float GetOpticalDepth(const FVector& Start, const FVector& End) const;
    static bool BlocksSight(const UWorld* World, const FVector& Start, const FVector& End, float Threshold=1.6f);
    static bool BlocksActorSight(const APawn* Observer, const AActor* Target, float Threshold=1.6f);
    static void ApplyGasDamage(APawn* Target, float Amount, AActor* Source, const FVector& Point);
private:
    TSet<TWeakObjectPtr<AGasRoomVolume>> Rooms;
    TSet<TWeakObjectPtr<AGasPipe>> Pipes;
    float Accumulator = 0.f;
    static constexpr float Step = 0.2f;
};
