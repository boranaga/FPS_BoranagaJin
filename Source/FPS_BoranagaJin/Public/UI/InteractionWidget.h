

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InteractionWidget.generated.h"

class ACharacterPlayer;

class UWidgetAnimation;
//class UVerticalBox;
//class UImage;
//class UOverlay;
//class UProgressBar;
//class UTextBlock;

UCLASS()
class FPS_BORANAGAJIN_API UInteractionWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime);
protected:
	UPROPERTY(meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* PopUp;

	//UPROPERTY(meta = (BindWidget))
	//UImage* Image = nullptr;
public:
	void PlayPopUpAnim();
	void PlayPopUpAnimReverse();
};
