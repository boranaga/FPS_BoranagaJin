#pragma once

#include "CoreMinimal.h"
#include "BaseUIWidget.h"
#include "PlayerDisplayWidget.generated.h"

class UInventoryUIWidget;

struct FInventorySlot;

UCLASS()
class FPS_BORANAGAJIN_API UPlayerDisplayWidget : public UBaseUIWidget
{
	GENERATED_BODY()
public:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual EUIType GetUIType() const { return EUIType::UIType_Inventory; }
public:
	void OpenInventory();
	void CloseInventory();
protected:
	void ShowMouseCursor();
	void HideMouseCursor();
	void DisplayInventory();
public:
	UFUNCTION()
	void CreateItemInventorySlots(int32 InventorySlotCount);
	UFUNCTION()
	void UpdateItemInventorySlots(const TArray<FInventorySlot>& Inventory);
	UFUNCTION()
	void CreateWeaponInventorySlots(int32 InventorySlotCount);
	UFUNCTION()
	void UpdateWeaponInventorySlots(const TArray<FInventorySlot>& Inventory);
protected:
	UPROPERTY(EditAnywhere, BlueprintreadWrite, Category = "ItemInventoryUIWidget")
	TSubclassOf<UInventoryUIWidget> ItemInventoryUIWidgetClass;
	UPROPERTY(meta = (BindWidget))
	UInventoryUIWidget* ItemInventoryUIWidget = nullptr;
	UPROPERTY(EditAnywhere, BlueprintreadWrite, Category = "WeaponInventoryUIWidget")
	TSubclassOf<UInventoryUIWidget> WeaponInventoryUIWidgetClass;
	UPROPERTY(meta = (BindWidget))
	UInventoryUIWidget* WeaponInventoryUIWidget = nullptr;
};
