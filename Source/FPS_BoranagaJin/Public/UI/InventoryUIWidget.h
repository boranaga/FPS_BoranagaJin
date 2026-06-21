#pragma once

#include "CoreMinimal.h"
#include "BaseUIWidget.h"
#include "InventoryUIWidget.generated.h"

class UInventorySlotWidget;

class UWrapBox;

struct FInventorySlot;

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
	UPROPERTY()
	TArray<UInventorySlotWidget*> InventorySlotWidgets;

//	UPROPERTY(EditAnywhere, Category = Weapon)
//	TSoftObjectPtr<UDataTable> ItemDataTable;
//	UPROPERTY() UDataTable* LoadedItemTable = nullptr;
//protected:
//	void LoadItemDataTable();
public:
	UFUNCTION()
	void CreateInventorySlots(int32 InventorySlotCount);
	UFUNCTION()
	void UpdateInventorySlots(const TArray<FInventorySlot>& Inventory);
	void SwapInventorySlots(int32 FromIndex, int32 ToIndex);
	void RequestSwapInventorySlots(FName InventoryName, int32 FromIndex, int32 ToIndex);
};
