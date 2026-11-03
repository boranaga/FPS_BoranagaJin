
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "QuickSlotEntryWidget.generated.h"

class UBorder;
class UImage;
class UTextBlock;
class UTexture2D;

UCLASS()
class FPS_BORANAGAJIN_API UQuickSlotEntryWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UBorder> SlotBorder;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UImage> ItemIcon;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> SlotNumberText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ItemNameText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ItemCountText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> SlotTypeText;

    UPROPERTY(EditDefaultsOnly, Category = "QuickSlot|Style")
    FLinearColor NormalBackgroundColor = FLinearColor(0.08f, 0.09f, 0.10f, 0.78f);

    UPROPERTY(EditDefaultsOnly, Category = "QuickSlot|Style")
    FLinearColor SelectedBackgroundColor = FLinearColor(0.20f, 0.21f, 0.18f, 0.95f);

    UPROPERTY(EditDefaultsOnly, Category = "QuickSlot|Style")
    FLinearColor NormalTextColor = FLinearColor(0.85f, 0.85f, 0.85f, 1.0f);

    UPROPERTY(EditDefaultsOnly, Category = "QuickSlot|Style")
    FLinearColor SelectedTextColor = FLinearColor(1.0f, 0.83f, 0.38f, 1.0f);

public:
    void UpdateSlot(
        int32 SlotIndex,
        const FText& ItemName,
        const FText& SlotTypeName,
        UTexture2D* IconTexture,
        int32 ItemCount,
        bool bHasItem,
        bool bSelected
    );
};
