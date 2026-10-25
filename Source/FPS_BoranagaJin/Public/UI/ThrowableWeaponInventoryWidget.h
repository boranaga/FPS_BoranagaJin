#pragma once

#include "CoreMinimal.h"
#include "BaseUIWidget.h"
#include "ThrowableWeaponInventoryWidget.generated.h"

class UWrapBox;
class UInventorySlotWidget;

struct FInventorySlot;

//DECLARE_MULTICAST_DELEGATE_OneParam(FOnThrowableWeaponInventoryCreated, int32);
//DECLARE_MULTICAST_DELEGATE_OneParam(FOnThrowableWeaponInventoryUpdated, const TArray<FInventorySlot>&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnThrowableWeaponEquipRequested, int32);

UCLASS()
class FPS_BORANAGAJIN_API UThrowableWeaponInventoryWidget : public UBaseUIWidget
{
	GENERATED_BODY()
public:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;

	virtual EUIType GetUIType() const { return EUIType::ThrowableWeaponInventory; }
public:
	//FOnThrowableWeaponInventoryCreated OnThrowableWeaponInventoryCreatedDelegate;
	//FOnThrowableWeaponInventoryUpdated OnThrowableWeaponInventoryUpdatedDelegate;
	FOnThrowableWeaponEquipRequested OnThrowableWeaponEquipRequested;
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
	void RequestEquipSlot(int32 SlotIndex);
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InventorySlotWidget")
	TSubclassOf<UInventorySlotWidget> InventorySlotWidgetClass;
protected:
	UPROPERTY()
	TArray<UInventorySlotWidget*> InventorySlotWidgets;
};
