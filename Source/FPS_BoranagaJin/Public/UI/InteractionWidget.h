

#pragma once

#include "CoreMinimal.h"
#include "BaseUIWidget.h"
#include "InteractionWidget.generated.h"

class ACharacterPlayer;

class UWidgetAnimation;
//class UVerticalBox;
//class UImage;
//class UOverlay;
//class UProgressBar;
//class UTextBlock;

UCLASS()
class FPS_BORANAGAJIN_API UInteractionWidget : public UBaseUIWidget
{
	GENERATED_BODY()
public:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime);
	virtual EUIType GetUIType() const { return EUIType::Interaction; }
protected:
	UPROPERTY(meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* PopUp;

	//UPROPERTY(meta = (BindWidget))
	//UImage* Image = nullptr;
public:
	void PlayPopUpAnim();
	void PlayPopUpAnimReverse();
};
