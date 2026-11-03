
#pragma once

#include "CoreMinimal.h"
#include "UI/BaseUIWidget.h"
#include "Items/ItemName.h"
#include "QuickSlotWidget.generated.h"

class UQuickSlotEntryWidget;
//class UInventorySystemComponent;
class UTexture2D;

struct FQuickSlot;

UCLASS()
class FPS_BORANAGAJIN_API UQuickSlotWidget : public UBaseUIWidget
{
    GENERATED_BODY()
public:
    virtual EUIType GetUIType() const { return EUIType::QuickSlot; }
protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UQuickSlotEntryWidget> QuickSlot1;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UQuickSlotEntryWidget> QuickSlot2;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UQuickSlotEntryWidget> QuickSlot3;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UQuickSlotEntryWidget> QuickSlot4;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UQuickSlotEntryWidget> QuickSlot5;

    // Configure textures for each item type in WBP_QuickSlot.
    UPROPERTY(EditDefaultsOnly, Category = "QuickSlot|Icons")
    TMap<EItemName, TObjectPtr<UTexture2D>> ItemIcons;

    //UPROPERTY(Transient)
    //TWeakObjectPtr<UInventorySystemComponent> InventoryComponent;

public:
    //void InitializeQuickSlotWidget(UInventorySystemComponent* InInventory);

    UFUNCTION(BlueprintCallable, Category = "QuickSlot")
    void RefreshQuickSlots();

    void UpdateQuickSlots(const TArray<FQuickSlot>& QuickSlots, int32 CurrIdx);


    void HandleQuickSlotSelectionChanged(int32 PreviousIndex, int32 NewIndex);

protected:
    FText GetSlotTypeText(int32 SlotIndex) const;
    UTexture2D* FindItemIcon(EItemName ItemName) const;
};
