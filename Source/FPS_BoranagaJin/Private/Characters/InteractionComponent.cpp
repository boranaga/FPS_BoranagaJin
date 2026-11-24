#include "Characters/InteractionComponent.h"
#include "Characters/Player/CharacterPlayer.h"
#include "PlayerUISubsystem.h"
#include "Items/InventorySystemComponent.h"
#include "Items/ItemPickUp.h"
#include "Items/Weapons/Weapon.h"
#include "Objects/ExitDoor.h"

#include "DrawDebugHelpers.h"
#include "Engine/EngineTypes.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Interface/InteractableInterface.h"
#include "Kismet/KismetSystemLibrary.h"
#include "TimerManager.h"

UInteractionComponent::UInteractionComponent()
{
    // Target search is timer-driven; cheap prompt positioning stays frame-smooth.
    PrimaryComponentTick.bCanEverTick = true;
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

void UInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // The expensive sphere query is timer-driven, but the existing world-space
    // interaction HUD should follow its target smoothly every rendered frame.
    if (IsValid(CurrentInteractionTarget) && !IsWorldInteractionBlocked())
    {
        UpdateInteractionPrompt();
    }
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
    return IsValid(UI) && UI->IsWorldInteractionInputBlocked();
}

void UInteractionComponent::TryInteract()
{
    if (IsWorldInteractionBlocked()) { return; }

    // Revalidate at the moment of input rather than relying on an old timer result.
    UpdateInteractionTarget();
    AActor* Target = CurrentInteractionTarget.Get();
    if (!IsValid(Target)) { return; }

    if (AItemPickUp* Pickup = Cast<AItemPickUp>(Target))
    {
        TryPickUpItem(Pickup);
        return; // One key press must never trigger two interactions.
    }

    if (!CanInteractWithActor(Target))
    {
        ClearInteractionTarget();
        return;
    }

    IInteractableInterface::Execute_Interact(Target, GetOwner());
}

bool UInteractionComponent::TryPickUpItem(AItemPickUp* Pickup)
{
    if (!IsPickupAvailable(Pickup) || !IsValid(OwnerCharacter)) { return false; }

    UInventorySystemComponent* Inventory = OwnerCharacter->GetInventorySystemComponent();
    if (!IsValid(Inventory)) { return false; }

    // Preserve the original restriction: do not pick up items while a weapon
    // is in a firing/reloading/otherwise busy state.
    if (AWeapon* CurrentWeapon = Inventory->GetCurrentWeapon())
    {
        const EWeaponStateType State = CurrentWeapon->GetCurrentState();
        if (State != EWeaponStateType::WeaponStateType_Unequipped &&
            State != EWeaponStateType::WeaponStateType_Idle)
        {
            return false;
        }
    }

    // Inventory mutates its slots and publishes inventory delegates only.
    if (!Inventory->TryStorePickup(Pickup)) { return false; }

    // The interaction layer consumes the world pickup only after inventory
    // storage succeeds (including magazines and duplicate-weapon ammo).
    ClearInteractionTarget();
    Pickup->DeactivateItemPickUp();
    UpdateInteractionTarget();
    return true;
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

    // Possession can change after BeginPlay; prefer the live controller.
    if (APlayerController* LiveController = Cast<APlayerController>(OwnerCharacter->GetController()))
    {
        OwnerPlayerController = LiveController;
    }

    if (IsWorldInteractionBlocked())
    {
        ClearInteractionTarget();
        UpdateInteractionPrompt();
        return;
    }

    // Explicit camera-traced world interactables (e.g., ExitDoor) take priority.
    // If no valid interface actor is hit, reuse the legacy nearby-item search.
    AActor* NextTarget = nullptr;
    FHitResult HitResult;
    if (PerformInteractionTrace(HitResult))
    {
        AActor* HitActor = HitResult.GetActor();
        if (IsActorInteractable(HitActor) && CanInteractWithActor(HitActor))
        {
            NextTarget = HitActor;
        }
    }

    if (!IsValid(NextTarget))
    {
        NextTarget = SearchItems();
    }

    SetInteractionTarget(NextTarget);
    UpdateInteractionPrompt();
}

AItemPickUp* UInteractionComponent::SearchItems() const
{
    if (!IsValid(OwnerCharacter) || !GetWorld() || SearchItemRadius <= 0.f)
    {
        return nullptr;
    }

    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(PickupObjectChannel.GetValue()));

    TArray<AActor*> IgnoredActors;
    IgnoredActors.Add(OwnerCharacter.Get());

    TArray<AActor*> OverlappingActors;
    UKismetSystemLibrary::SphereOverlapActors(
        GetWorld(), OwnerCharacter->GetActorLocation(), SearchItemRadius,
        ObjectTypes, AItemPickUp::StaticClass(), IgnoredActors, OverlappingActors);

    AItemPickUp* Nearest = nullptr;
    float BestDistanceSquared = FMath::Square(SearchItemRadius);

    for (AActor* Candidate : OverlappingActors)
    {
        AItemPickUp* Pickup = Cast<AItemPickUp>(Candidate);
        if (!IsPickupAvailable(Pickup)) { continue; }

        const float DistanceSquared = FVector::DistSquared(
            OwnerCharacter->GetActorLocation(), Pickup->GetActorLocation());
        if (DistanceSquared < BestDistanceSquared)
        {
            BestDistanceSquared = DistanceSquared;
            Nearest = Pickup;
        }
    }
    return Nearest;
}

bool UInteractionComponent::IsPickupAvailable(AItemPickUp* Pickup) const
{
    if (!IsValid(Pickup) || Pickup->IsDead() || !IsValid(OwnerCharacter))
    {
        return false;
    }

    return FVector::DistSquared(OwnerCharacter->GetActorLocation(),
        Pickup->GetActorLocation()) <= FMath::Square(SearchItemRadius)
        && IsPointWithinPickupViewport(Pickup->GetActorLocation());
}

bool UInteractionComponent::IsPointWithinPickupViewport(const FVector& WorldLocation) const
{
    APlayerController* Controller = IsValid(OwnerCharacter)
        ? Cast<APlayerController>(OwnerCharacter->GetController()) : nullptr;
    if (!IsValid(Controller)) { return false; }

    FVector2D ScreenLocation;
    if (!Controller->ProjectWorldLocationToScreen(WorldLocation, ScreenLocation))
    {
        return false;
    }

    int32 Width = 0;
    int32 Height = 0;
    Controller->GetViewportSize(Width, Height);
    if (Width <= 0 || Height <= 0) { return false; }

    // Match the prior viewport-ratio semantics: 0.7 means the center 40%
    // of each axis ([0.3, 0.7]), while 0/1 allow the whole axis.
    const auto WithinAxis = [](float Pixel, float Size, float Ratio)
        {
            if (Ratio <= 0.f || Ratio >= 1.f)
            {
                return Pixel >= 0.f && Pixel <= Size;
            }
            const float Edge = FMath::Min(Ratio, 1.f - Ratio);
            return Pixel >= Size * Edge && Pixel <= Size * (1.f - Edge);
        };

    return WithinAxis(ScreenLocation.X, static_cast<float>(Width), SearchItemViewportRatio_Width)
        && WithinAxis(ScreenLocation.Y, static_cast<float>(Height), SearchItemViewportRatio_Height);
}

void UInteractionComponent::UpdateInteractionPrompt()
{
    if (!IsValid(OwnerCharacter)) { return; }

    AActor* Target = CurrentInteractionTarget.Get();
    if (!IsValid(Target) || IsWorldInteractionBlocked())
    {
        OwnerCharacter->OnInteractionUIUpdatedDelegate.Broadcast(false, FVector::ZeroVector);
        return;
    }

    FVector PromptLocation = Target->GetActorLocation();
    if (const AExitDoor* Door = Cast<AExitDoor>(Target))
    {
        PromptLocation = Door->GetInteractionPromptLocation();
    }
    // Items keep their original location as the screen-space widget anchor.
    OwnerCharacter->OnInteractionUIUpdatedDelegate.Broadcast(true, PromptLocation);
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

    if (IsActorInteractable(CurrentInteractionTarget.Get()))
    {
        IInteractableInterface::Execute_EndFocus(CurrentInteractionTarget.Get(), GetOwner());
    }

    CurrentInteractionTarget = NewTarget;

    if (IsActorInteractable(NewTarget))
    {
        IInteractableInterface::Execute_BeginFocus(NewTarget, GetOwner());
    }

    OnInteractionTargetChanged.Broadcast(NewTarget);
    if (IsValid(NewTarget) && IsValid(OwnerCharacter))
    {
        OwnerCharacter->OnInteractionUIPopUpDelegate.Broadcast();
    }
}

void UInteractionComponent::ClearInteractionTarget()
{
    if (!IsValid(CurrentInteractionTarget))
    {
        CurrentInteractionTarget = nullptr;
        if (IsValid(OwnerCharacter))
        {
            OwnerCharacter->OnInteractionUIUpdatedDelegate.Broadcast(false, FVector::ZeroVector);
        }
        return;
    }

    SetInteractionTarget(nullptr);
    if (IsValid(OwnerCharacter))
    {
        OwnerCharacter->OnInteractionUIUpdatedDelegate.Broadcast(false, FVector::ZeroVector);
    }
}

bool UInteractionComponent::IsActorInteractable(AActor* TargetActor) const
{
    return IsValid(TargetActor) && TargetActor->Implements<UInteractableInterface>();
}

bool UInteractionComponent::CanInteractWithActor(AActor* TargetActor) const
{
    if (AItemPickUp* Pickup = Cast<AItemPickUp>(TargetActor))
    {
        return IsPickupAvailable(Pickup);
    }
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
    if (!IsValid(CurrentInteractionTarget)) { return FText::GetEmpty(); }

    if (Cast<AItemPickUp>(CurrentInteractionTarget.Get()))
    {
        return NSLOCTEXT("Interaction", "Pickup", "아이템 줍기");
    }

    if (!IsActorInteractable(CurrentInteractionTarget.Get()))
    {
        return FText::GetEmpty();
    }

    return IInteractableInterface::Execute_GetInteractionText(
        CurrentInteractionTarget, GetOwner());
}
