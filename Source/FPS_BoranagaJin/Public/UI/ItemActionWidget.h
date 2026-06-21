#pragma once

#include "CoreMinimal.h"
#include "BaseUIWidget.h"
#include "ItemActionWidget.generated.h"

class UButton;
class UTextBlock;

UCLASS()
class FPS_BORANAGAJIN_API UItemActionWidget : public UBaseUIWidget
{
	GENERATED_BODY()
public:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual EUIType GetUIType() const { return EUIType::UIType_Inventory; }
protected:
	UPROPERTY(EditAnyWhere, BlueprintReadWrite, meta = (BindWidget))
	UButton* ButtonItemAction;
	UPROPERTY(EditAnyWhere, BlueprintReadWrite, meta = (BindWidget))
	UTextBlock* TxtTextBlock;
	UPROPERTY(EditAnyWhere, BlueprintReadWrite, meta = (BindWidget))
	FText ActionText;
protected:
	UFUNCTION()
	void OnButtonHovered();
};
