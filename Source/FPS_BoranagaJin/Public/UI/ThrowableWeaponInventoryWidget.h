#pragma once

#include "CoreMinimal.h"
#include "BaseUIWidget.h"
#include "ThrowableWeaponInventoryWidget.generated.h"

class UWrapBox;

class UInventorySlotWidget;

struct FInventorySlot;

UCLASS()
class FPS_BORANAGAJIN_API UThrowableWeaponInventoryWidget : public UBaseUIWidget
{
	GENERATED_BODY()
public:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual EUIType GetUIType() const { return EUIType::UIType_ThrowableWeaponInventory; }
public:
	UPROPERTY(meta = (BindWidget))
	UWrapBox* WrapBoxInventory;
public:
	void OpenUI();
	void CloseUI();
protected:
	void ShowMouseCursor();
	void HideMouseCursor();
public:
	UFUNCTION()
	void CreateInventorySlots(int32 InventorySlotCount);
	UFUNCTION()
	void UpdateInventorySlots(const TArray<FInventorySlot>& Inventory);

public:
	UPROPERTY(EditAnywhere, BlueprintreadWrite, Category = "InventorySlotWidget")
	TSubclassOf<UInventorySlotWidget> InventorySlotWidgetClass;
protected:
	UPROPERTY()
	TArray<UInventorySlotWidget*> InventorySlotWidgets;
};
