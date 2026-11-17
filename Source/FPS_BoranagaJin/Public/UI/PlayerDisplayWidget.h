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
	virtual EUIType GetUIType() const { return EUIType::Inventory; }
	UInventoryUIWidget* GetItemInventoryUIWidget() { return ItemInventoryUIWidget; }
	UInventoryUIWidget* GetPrimaryWeaponInventoryUIWidget() { return PrimaryWeaponInventoryUIWidget; }
	UInventoryUIWidget* GetSecondaryWeaponInventoryUIWidget() { return SecondaryWeaponInventoryUIWidget; }
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
	void CreatePrimaryWeaponInventorySlots(int32 InventorySlotCount);
	UFUNCTION()
	void UpdatePrimaryWeaponInventorySlots(const TArray<FInventorySlot>& Inventory);
	UFUNCTION()
	void CreateSecondaryWeaponInventorySlots(int32 InventorySlotCount);
	UFUNCTION()
	void UpdateSecondaryWeaponInventorySlots(const TArray<FInventorySlot>& Inventory);
protected:
	UPROPERTY(EditAnywhere, BlueprintreadWrite, Category = "ItemInventoryUIWidget")
	TSubclassOf<UInventoryUIWidget> ItemInventoryUIWidgetClass;
	UPROPERTY(meta = (BindWidget))
	UInventoryUIWidget* ItemInventoryUIWidget = nullptr;
	UPROPERTY(EditAnywhere, BlueprintreadWrite, Category = "PrimaryWeaponInventoryUIWidget")
	TSubclassOf<UInventoryUIWidget> PrimaryWeaponInventoryUIWidgetClass;
	UPROPERTY(meta = (BindWidget))
	UInventoryUIWidget* PrimaryWeaponInventoryUIWidget = nullptr;
	UPROPERTY(EditAnywhere, BlueprintreadWrite, Category = "SecondaryWeaponInventoryUIWidget")
	TSubclassOf<UInventoryUIWidget> SecondaryWeaponInventoryUIWidgetClass;
	UPROPERTY(meta = (BindWidget))
	UInventoryUIWidget* SecondaryWeaponInventoryUIWidget = nullptr;
};
