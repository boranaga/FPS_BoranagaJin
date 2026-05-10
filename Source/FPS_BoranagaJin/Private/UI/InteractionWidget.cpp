


#include "UI/InteractionWidget.h"

#include "Animation/WidgetAnimation.h"
// #include "InterchangeResult.h"
//#include "Items/Projectile/Projectile.h"
//#include "Characters/Player/CharacterPlayer.h"
//#include "Characters/Player/PlayerMovementComponent.h"
//
//#include "Components/CanvasPanelSlot.h"
//#include "Components/Image.h"
//#include "Components/Overlay.h"
//#include "Components/ProgressBar.h"
//#include "Components/TextBlock.h"
//#include "Components/VerticalBox.h"


void UInteractionWidget::NativeConstruct()
{
    Super::NativeConstruct();

    //UE_LOG(LogTemp, Error, TEXT("UWeaponAimUIWidget::NativeConstruct()!!!"));
}

void UInteractionWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
}

void UInteractionWidget::PlayPopUpAnim()
{
    PlayAnimation(PopUp);
}

void UInteractionWidget::PlayPopUpAnimReverse()
{
    PlayAnimation(PopUp, EUMGSequencePlayMode::Reverse);
}