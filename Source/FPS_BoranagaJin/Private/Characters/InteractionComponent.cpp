#include "Characters/InteractionComponent.h"
#include "Characters/Player/CharacterPlayer.h"
#include "PlayerUISubsystem.h"

#include "DrawDebugHelpers.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Interface/InteractableInterface.h"
#include "TimerManager.h"

UInteractionComponent::UInteractionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UInteractionComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerCharacter = Cast<ACharacterPlayer>(GetOwner());
    if (!IsValid(OwnerCharacter))
    {
        UE_LOG(LogTemp, Error,
            TEXT("UInteractionComponent must be owned by ACharacterPlayer. Owner: %s"),
            *GetNameSafe(GetOwner()));
        return;
    }

    OwnerPlayerController = Cast<APlayerController>(OwnerCharacter->GetController());
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            InteractionTraceTimerHandle, this,
            &UInteractionComponent::UpdateInteractionTarget,
            FMath::Max(0.01f, TraceInterval), true, 0.f);
    }
}

void UInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(InteractionTraceTimerHandle);
    }
    ClearInteractionTarget();
    Super::EndPlay(EndPlayReason);
}

UPlayerUISubsystem* UInteractionComponent::ResolvePlayerUISubsystem() const
{
    // Prefer the live controller to the cached one (possession can happen after BeginPlay).
    const APlayerController* PC = IsValid(OwnerCharacter)
        ? Cast<APlayerController>(OwnerCharacter->GetController()) : nullptr;
    if (!IsValid(PC))
    {
        PC = OwnerPlayerController.Get();
    }
    if (!IsValid(PC)) { return nullptr; }

    const ULocalPlayer* LocalPlayer = PC->GetLocalPlayer();
    return LocalPlayer ? LocalPlayer->GetSubsystem<UPlayerUISubsystem>() : nullptr;
}

bool UInteractionComponent::IsWorldInteractionBlocked() const
{
    const UPlayerUISubsystem* UI = ResolvePlayerUISubsystem();
    return IsValid(UI) && UI->IsExitDoorUIOpen();
}

void UInteractionComponent::TryInteract()
{
    // CharacterPlayer does not inspect UI; this component rejects world interactions.
    if (IsWorldInteractionBlocked()) { return; }

    if (!IsValid(CurrentInteractionTarget)) { return; }
    if (!CanInteractWithActor(CurrentInteractionTarget))
    {
        ClearInteractionTarget();
        return;
    }

    IInteractableInterface::Execute_Interact(CurrentInteractionTarget, GetOwner());
}

void UInteractionComponent::HandleEscapeInput()
{
    UPlayerUISubsystem* UI = ResolvePlayerUISubsystem();
    if (!IsValid(UI)) { return; }

    // ExitDoor editing has precedence over PauseMenu.
    if (UI->IsExitDoorUIOpen())
    {
        UI->CloseExitDoorUI();
        return;
    }

    // Preserve any existing game-over UI rather than stacking PauseMenu on top.
    if (UI->IsGameOverUIOpened()) { return; }

    UI->TogglePauseMenu();
}

void UInteractionComponent::UpdateInteractionTarget()
{
    if (!IsValid(OwnerCharacter)) { return; }

    if (!IsValid(OwnerPlayerController))
    {
        OwnerPlayerController = Cast<APlayerController>(OwnerCharacter->GetController());
    }

    // A modal ExitDoor widget owns interaction while visible.
    if (IsWorldInteractionBlocked())
    {
        ClearInteractionTarget();
        return;
    }

    FHitResult HitResult;
    if (!PerformInteractionTrace(HitResult))
    {
        ClearInteractionTarget();
        return;
    }

    AActor* HitActor = HitResult.GetActor();
    if (!IsActorInteractable(HitActor) || !CanInteractWithActor(HitActor))
    {
        ClearInteractionTarget();
        return;
    }

    SetInteractionTarget(HitActor);
}

bool UInteractionComponent::PerformInteractionTrace(FHitResult& OutHitResult) const
{
    if (!GetWorld() || !IsValid(OwnerCharacter)) { return false; }

    const FVector TraceStart = GetTraceStart();
    const FVector TraceEnd = TraceStart + GetTraceDirection() * InteractionDistance;

    FCollisionQueryParams QueryParams(
        SCENE_QUERY_STAT(InteractionTrace), bTraceComplex, GetOwner());
    QueryParams.AddIgnoredActor(GetOwner());

    const bool bHit = GetWorld()->LineTraceSingleByChannel(
        OutHitResult, TraceStart, TraceEnd, InteractionTraceChannel, QueryParams);

#if ENABLE_DRAW_DEBUG
    if (bDrawDebugTrace)
    {
        const FColor Color = bHit ? FColor::Green : FColor::Red;
        DrawDebugLine(GetWorld(), TraceStart, TraceEnd, Color,
            false, TraceInterval, 0, 1.5f);
        if (bHit)
        {
            DrawDebugSphere(GetWorld(), OutHitResult.ImpactPoint, 8.f,
                12, Color, false, TraceInterval);
        }
    }
#endif

    return bHit;
}

void UInteractionComponent::SetInteractionTarget(AActor* NewTarget)
{
    if (CurrentInteractionTarget == NewTarget) { return; }

    if (IsValid(CurrentInteractionTarget))
    {
        IInteractableInterface::Execute_EndFocus(CurrentInteractionTarget, GetOwner());
    }

    CurrentInteractionTarget = NewTarget;

    if (IsValid(CurrentInteractionTarget))
    {
        IInteractableInterface::Execute_BeginFocus(CurrentInteractionTarget, GetOwner());
    }

    OnInteractionTargetChanged.Broadcast(CurrentInteractionTarget);
}

void UInteractionComponent::ClearInteractionTarget()
{
    if (!IsValid(CurrentInteractionTarget))
    {
        CurrentInteractionTarget = nullptr;
        return;
    }

    IInteractableInterface::Execute_EndFocus(CurrentInteractionTarget, GetOwner());
    CurrentInteractionTarget = nullptr;
    OnInteractionTargetChanged.Broadcast(nullptr);
}

bool UInteractionComponent::IsActorInteractable(AActor* TargetActor) const
{
    return IsValid(TargetActor) && TargetActor->Implements<UInteractableInterface>();
}

bool UInteractionComponent::CanInteractWithActor(AActor* TargetActor) const
{
    return IsActorInteractable(TargetActor) &&
        IInteractableInterface::Execute_CanInteract(TargetActor, GetOwner());
}

FVector UInteractionComponent::GetTraceStart() const
{
    if (IsValid(OwnerPlayerController))
    {
        FVector ViewLocation;
        FRotator ViewRotation;
        OwnerPlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
        return ViewLocation;
    }
    return IsValid(OwnerCharacter)
        ? OwnerCharacter->GetPawnViewLocation() : GetOwner()->GetActorLocation();
}

FVector UInteractionComponent::GetTraceDirection() const
{
    if (IsValid(OwnerPlayerController))
    {
        FVector ViewLocation;
        FRotator ViewRotation;
        OwnerPlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
        return ViewRotation.Vector();
    }
    return IsValid(OwnerCharacter)
        ? OwnerCharacter->GetBaseAimRotation().Vector()
        : GetOwner()->GetActorForwardVector();
}

AActor* UInteractionComponent::GetCurrentInteractionTarget() const
{
    return CurrentInteractionTarget.Get();
}

bool UInteractionComponent::HasInteractionTarget() const
{
    return IsValid(CurrentInteractionTarget);
}

FText UInteractionComponent::GetCurrentInteractionText() const
{
    if (!IsValid(CurrentInteractionTarget) ||
        !CurrentInteractionTarget->Implements<UInteractableInterface>())
    {
        return FText::GetEmpty();
    }
    return IInteractableInterface::Execute_GetInteractionText(
        CurrentInteractionTarget, GetOwner());
}
