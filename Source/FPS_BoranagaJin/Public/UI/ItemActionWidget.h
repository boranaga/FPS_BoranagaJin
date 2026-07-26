#pragma once

#include "CoreMinimal.h"
#include "BaseUIWidget.h"
#include "ItemActionWidget.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnItemActionClicked);

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
public:
	FOnItemActionClicked OnItemActionClicked;
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
	UFUNCTION()
	void OnButtonClicked();
};
