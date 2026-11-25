#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractionComponent.generated.h"

class ACharacterPlayer;
class APlayerController;
class UPlayerUISubsystem;
class AItemPickUp;
class UInteractionPointComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FOnInteractionTargetChanged,
    AActor*, NewTarget
);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FPS_BORANAGAJIN_API UInteractionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UInteractionComponent();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

public:
    /** Called by CharacterPlayer when the interaction InputAction starts. */
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void TryInteract();

    /** Called by CharacterPlayer when EscapeInputAction starts. */
    UFUNCTION(BlueprintCallable, Category = "Interaction|Input")
    void HandleEscapeInput();

    /** True while ExitDoor UI is editing requirements. */
    UFUNCTION(BlueprintPure, Category = "Interaction|Input")
    bool IsWorldInteractionBlocked() const;

    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void UpdateInteractionTarget();

    UFUNCTION(BlueprintPure, Category = "Interaction")
    AActor* GetCurrentInteractionTarget() const;

    UFUNCTION(BlueprintPure, Category = "Interaction")
    bool HasInteractionTarget() const;

    UFUNCTION(BlueprintPure, Category = "Interaction")
    FText GetCurrentInteractionText() const;

    /** Re-evaluate the active target's best world-space prompt anchor. */
    UFUNCTION(BlueprintCallable, Category = "Interaction|Widget")
    void RefreshInteractionPrompt();

protected:
    bool PerformInteractionTrace(FHitResult& OutHitResult) const;

    /** Finds the closest active pickup within the old inventory search sphere/viewport. */
    AItemPickUp* SearchItems() const;
    bool IsPickupAvailable(AItemPickUp* Pickup) const;
    bool TryPickUpItem(AItemPickUp* Pickup);
    bool IsPointWithinPickupViewport(const FVector& WorldLocation) const;
    bool FindBestInteractionPoint(AActor* Target, FVector& OutLocation);
    bool IsPointVisibleFromCamera(const FVector& WorldLocation, AActor* Target) const;
    void UpdateInteractionPrompt();
    void SetInteractionTarget(AActor* NewTarget);
    void ClearInteractionTarget();
    bool IsActorInteractable(AActor* TargetActor) const;
    bool CanInteractWithActor(AActor* TargetActor) const;
    FVector GetTraceStart() const;
    FVector GetTraceDirection() const;

    /** Player UI lookup stays inside the interaction/input boundary. */
    UPlayerUISubsystem* ResolvePlayerUISubsystem() const;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Trace",
        meta = (ClampMin = "0.0", Units = "cm"))
    float InteractionDistance = 250.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Trace",
        meta = (ClampMin = "0.01", Units = "s"))
    float TraceInterval = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Trace")
    TEnumAsByte<ECollisionChannel> InteractionTraceChannel = ECC_Visibility;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Trace")
    bool bTraceComplex = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Debug")
    bool bDrawDebugTrace = false;

    // Moved from InventorySystemComponent: detection and visual filtering
    // now belong exclusively to interaction gameplay.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Pickup",
        meta = (ClampMin = "0.0", Units = "cm"))
    float SearchItemRadius = 150.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Pickup",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SearchItemViewportRatio_Width = 0.7f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Pickup",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SearchItemViewportRatio_Height = 0.7f;

    // Retain the existing pickup object channel (GameTraceChannel6).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Pickup")
    TEnumAsByte<ECollisionChannel> PickupObjectChannel = ECC_GameTraceChannel6;

    /** Channel used solely for choosing a visible widget location. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Widget")
    TEnumAsByte<ECollisionChannel> PromptOcclusionTraceChannel = ECC_Visibility;

    /** A point must beat the currently selected point by this score to switch. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Widget",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float PromptPointSwitchHysteresis = 0.12f;

    /** Points are evaluated only for the current target, not all nearby actors. */
    UPROPERTY(BlueprintAssignable, Category = "Interaction")
    FOnInteractionTargetChanged OnInteractionTargetChanged;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Interaction")
    TObjectPtr<AActor> CurrentInteractionTarget;

private:
    UPROPERTY()
    TObjectPtr<ACharacterPlayer> OwnerCharacter;

    UPROPERTY()
    TObjectPtr<APlayerController> OwnerPlayerController;

    FTimerHandle InteractionTraceTimerHandle;

    TWeakObjectPtr<UInteractionPointComponent> SelectedInteractionPoint;
};
