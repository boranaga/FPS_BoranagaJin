#include "Components/InteractionPointComponent.h"

UInteractionPointComponent::UInteractionPointComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetCanEverAffectNavigation(false);
    SetMobility(EComponentMobility::Movable);
}

FVector UInteractionPointComponent::GetInteractionWidgetLocation() const
{
    return GetComponentLocation() + WidgetWorldOffset;
}
