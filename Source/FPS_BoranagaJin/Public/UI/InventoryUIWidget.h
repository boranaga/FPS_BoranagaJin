#pragma once

#include "CoreMinimal.h"
#include "BaseUIWidget.h"
#include "InventoryUIWidget.generated.h"

class UInventorySlotWidget;

class UWrapBox;

UCLASS()
class FPS_BORANAGAJIN_API UInventoryUIWidget : public UBaseUIWidget
{
	GENERATED_BODY()
public:
	virtual void NativePreConstruct() override;
public:
	UPROPERTY(meta = (BindWidget))
	UWrapBox* WrapBoxInventory;

	UPROPERTY(EditAnywhere, BlueprintreadWrite, Category = "InventorySlotWidget")
	TSubclassOf<UInventorySlotWidget> InventorySlotWidgetClass;

protected:
	void CreateInventorySlots();
};
