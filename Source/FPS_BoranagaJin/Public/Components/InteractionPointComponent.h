#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "InteractionPointComponent.generated.h"

/**
 * A world-space anchor used by UInteractionComponent to place an interaction HUD.
 * Add as many components as necessary to any Actor Blueprint. Attach them to a
 * moving scene component (e.g., DoorPivot) to follow that component in real time.
 */
UCLASS(ClassGroup=(Interaction), meta=(BlueprintSpawnableComponent), BlueprintType)
class FPS_BORANAGAJIN_API UInteractionPointComponent : public USceneComponent
{
    GENERATED_BODY()

public:
    UInteractionPointComponent();

    /** World-space location where the bottom-center of the widget is placed. */
    UFUNCTION(BlueprintPure, Category="Interaction|Point")
    FVector GetInteractionWidgetLocation() const;

    UFUNCTION(BlueprintPure, Category="Interaction|Point")
    bool IsInteractionPointEnabled() const { return bInteractionPointEnabled; }

    UFUNCTION(BlueprintPure, Category="Interaction|Point")
    bool RequiresClearLineOfSight() const { return bRequireClearLineOfSight; }

    UFUNCTION(BlueprintPure, Category="Interaction|Point")
    float GetSelectionPriority() const { return SelectionPriority; }

protected:
    /** Disabled points are excluded; no actor-specific code changes are needed. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction|Point")
    bool bInteractionPointEnabled = true;

    /** Offset along WORLD axes; positive Z is above the point even as the door rotates. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction|Point", meta=(Units="cm"))
    FVector WidgetWorldOffset = FVector(0.f, 0.f, 20.f);

    /** A higher value wins when candidate points have comparable screen placement. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction|Point", meta=(ClampMin="-5.0", ClampMax="5.0"))
    float SelectionPriority = 0.f;

    /** Reject this point if another actor occludes it on the configured trace channel. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction|Point")
    bool bRequireClearLineOfSight = true;
};
