
#include "UI/QuickSlotEntryWidget.h"

#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

void UQuickSlotEntryWidget::UpdateSlot(
    int32 SlotIndex,
    const FText& ItemName,
    const FText& SlotTypeName,
    UTexture2D* IconTexture,
    int32 ItemCount,
    bool bHasItem,
    bool bSelected
)
{
    if (!SlotBorder || !ItemIcon || !SlotNumberText ||
        !ItemNameText || !ItemCountText || !SlotTypeText)
    {
        return;
    }

    const FLinearColor BackgroundColor =
        bSelected ? SelectedBackgroundColor : NormalBackgroundColor;

    const FLinearColor TextColor =
        bSelected ? SelectedTextColor : NormalTextColor;

    SlotBorder->SetBrushColor(BackgroundColor);

    SlotNumberText->SetText(FText::AsNumber(SlotIndex + 1));
    SlotNumberText->SetColorAndOpacity(FSlateColor(TextColor));

    SlotTypeText->SetText(SlotTypeName);

    if (!bHasItem)
    {
        ItemNameText->SetText(FText::FromString(TEXT("EMPTY")));
        ItemNameText->SetColorAndOpacity(FSlateColor(NormalTextColor));

        ItemIcon->SetVisibility(ESlateVisibility::Hidden);
        ItemCountText->SetVisibility(ESlateVisibility::Collapsed);

        return;
    }

    ItemNameText->SetText(ItemName);
    ItemNameText->SetColorAndOpacity(FSlateColor(FLinearColor::White));

    if (IconTexture)
    {
        ItemIcon->SetBrushFromTexture(IconTexture, true);
        ItemIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
    else
    {
        ItemIcon->SetVisibility(ESlateVisibility::Hidden);
    }

    if (ItemCount > 1)
    {
        ItemCountText->SetText(
            FText::Format(
                FText::FromString(TEXT("x{0}")),
                FText::AsNumber(ItemCount)
            )
        );

        ItemCountText->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
    else
    {
        ItemCountText->SetVisibility(ESlateVisibility::Collapsed);
    }

    ItemCountText->SetColorAndOpacity(FSlateColor(TextColor));
}
