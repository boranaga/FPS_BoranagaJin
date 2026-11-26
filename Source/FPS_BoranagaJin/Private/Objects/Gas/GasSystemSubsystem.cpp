#include "Objects/Gas/GasSystemSubsystem.h"
#include "Objects/Gas/GasRoomVolume.h"
#include "Objects/Gas/GasPipe.h"
#include "Interface/DamageInterface.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

bool UGasSystemSubsystem::DoesSupportWorldType(EWorldType::Type Type) const
{
    return Type == EWorldType::Game || Type == EWorldType::PIE || Type == EWorldType::GamePreview;
}
TStatId UGasSystemSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGasSystemSubsystem, STATGROUP_Tickables);
}
void UGasSystemSubsystem::Deinitialize()
{
    Rooms.Reset(); Pipes.Reset(); Accumulator = 0.f;
    Super::Deinitialize();
}
void UGasSystemSubsystem::RegisterRoom(AGasRoomVolume* Room) { if (IsValid(Room)) Rooms.Add(Room); }
void UGasSystemSubsystem::UnregisterRoom(AGasRoomVolume* Room) { Rooms.Remove(Room); }
void UGasSystemSubsystem::RegisterPipe(AGasPipe* Pipe) { if (IsValid(Pipe)) Pipes.Add(Pipe); }
void UGasSystemSubsystem::UnregisterPipe(AGasPipe* Pipe) { Pipes.Remove(Pipe); }
void UGasSystemSubsystem::Tick(float DeltaTime)
{
    if (!GetWorld() || !GetWorld()->HasBegunPlay() || GetWorld()->GetNetMode() == NM_Client) return;
    if (Rooms.IsEmpty() && Pipes.IsEmpty()) { Accumulator = 0.f; return; }
    Accumulator = FMath::Min(Accumulator + FMath::Max(0.f, DeltaTime), 1.f);
    while (Accumulator + KINDA_SMALL_NUMBER >= Step)
    {
        Accumulator = FMath::Max(0.f, Accumulator - Step);
        // Snapshots tolerate actors being destroyed/unregistered during damage callbacks.
        const auto PipeSnapshot = Pipes.Array();
        for (const auto& Entry : PipeSnapshot) if (AGasPipe* Pipe = Entry.Get()) Pipe->Simulate(Step);
        const auto RoomSnapshot = Rooms.Array();
        for (const auto& Entry : RoomSnapshot) if (AGasRoomVolume* Room = Entry.Get()) Room->Simulate(Step);
    }
}
AGasRoomVolume* UGasSystemSubsystem::FindRoomAt(const FVector& Point) const
{
    AGasRoomVolume* Best = nullptr;
    float BestSize = TNumericLimits<float>::Max();
    for (const auto& Entry : Rooms)
    {
        AGasRoomVolume* Room = Entry.Get();
        if (IsValid(Room) && Room->ContainsPoint(Point) && Room->GetRoomVolume() < BestSize)
        { Best = Room; BestSize = Room->GetRoomVolume(); }
    }
    return Best;
}
float UGasSystemSubsystem::GetOpticalDepth(const FVector& Start, const FVector& End) const
{
    float Depth = 0.f;
    for (const auto& Entry : Rooms) if (const AGasRoomVolume* Room = Entry.Get()) Depth += Room->GetOpticalDepth(Start, End);
    return Depth;
}
bool UGasSystemSubsystem::BlocksSight(const UWorld* World, const FVector& Start, const FVector& End, float Threshold)
{
    const UGasSystemSubsystem* System = World ? World->GetSubsystem<UGasSystemSubsystem>() : nullptr;
    return System && System->GetOpticalDepth(Start, End) >= FMath::Max(0.01f, Threshold);
}
bool UGasSystemSubsystem::BlocksActorSight(const APawn* Observer, const AActor* Target, float Threshold)
{
    if (!IsValid(Observer) || !IsValid(Target)) return false;
    FVector Start, End; FRotator Rotation;
    Observer->GetActorEyesViewPoint(Start, Rotation);
    Target->GetActorEyesViewPoint(End, Rotation);
    return BlocksSight(Observer->GetWorld(), Start, End, Threshold);
}
void UGasSystemSubsystem::ApplyGasDamage(APawn* Target, float Amount, AActor* Source, const FVector& Point)
{
    if (!IsValid(Target) || !Target->HasAuthority() || !FMath::IsFinite(Amount) || Amount <= 0.f) return;
    IDamageableInterface* Damageable = Cast<IDamageableInterface>(Target);
    if (!Damageable || Damageable->IsDead()) return;
    FDamageParams Params;
    Params.DamageAmount = Amount;
    Params.DamageType = EGameDamageType::Poison;
    // Environmental source, never the player who originally damaged the pipe.
    // Existing EnemyStateMachine treats any non-null DamageCauser as a chase
    // target. Gas is environmental damage, so never make an AI chase a pipe.
    // Player damage callbacks can still identify the environmental source.
    Params.DamageCauser = Target->IsPlayerControlled() ? Source : nullptr;
    Params.ImpactPoint = Point;
    Damageable->ReceiveDamage(Params);
}
