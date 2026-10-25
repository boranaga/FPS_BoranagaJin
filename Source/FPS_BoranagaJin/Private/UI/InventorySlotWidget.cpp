#include "UI/InventorySlotWidget.h"
#include "UI/InventoryUIWidget.h"
#include "UI/ThrowableWeaponInventoryWidget.h"
#include "UI/ItemToolWidget.h"
#include "UI/UIType.h"
#include "UI/InventoryDragDropOperation.h"
#include "Data/ItemData.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"

#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

void UInventorySlotWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (OverlayInventorySlot)
	{
		OverlayInventorySlot->SetVisibility(ESlateVisibility::Visible);
	}

	LoadItemDataTable();

	// ItemToolWidget은 여기서 생성하지 않는다.
	// 일반 Inventory Slot이 실제로 Hover 되었을 때 필요한 경우에만 생성한다.
}

void UInventorySlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Blueprint 등에서 설정되어 있을 수 있는 원래 Tint를 저장한다.
	DefaultSlotColor = GetColorAndOpacity();
}

void UInventorySlotWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
}

void UInventorySlotWidget::ConfigureAsThrowableWeaponSlot(UThrowableWeaponInventoryWidget* InOwner)
{
	bIsThrowableWeaponSlot = true;
	OwnerThrowableWeaponInventoryWidget = InOwner;

	// 혹시 이미 만들어진 ItemToolWidget이 있다면 제거한다.
	if (IsValid(ItemToolWidget))
	{
		ItemToolWidget->RemoveFromParent();
		ItemToolWidget = nullptr;
	}
}

FReply UInventorySlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UInventorySlotWidget::NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		// -----------------------------------------------------
		// Throwable Weapon Inventory
		// -----------------------------------------------------
		if (bIsThrowableWeaponSlot)
		{
			PlayUISound(ESoundID::UI_Click);

			if (IsValid(OwnerThrowableWeaponInventoryWidget))
			{
				OwnerThrowableWeaponInventoryWidget->RequestEquipSlot(Index);
			}

			return FReply::Handled();
		}

		// -----------------------------------------------------
		// 기존 Inventory Slot
		// 기존 Drag & Drop 기능 유지
		// -----------------------------------------------------
		return UWidgetBlueprintLibrary::DetectDragIfPressed(
			InMouseEvent,
			this,
			EKeys::LeftMouseButton
		).NativeReply;
	}

	PlayUISound(ESoundID::UI_Click);

	return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
}

void UInventorySlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	// Throwable Weapon Slot은 클릭 선택 전용이므로 Drag하지 않는다.
	if (bIsThrowableWeaponSlot) { return; }

	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	UInventoryDragDropOperation* DragOperation = NewObject<UInventoryDragDropOperation>();

	if (!DragOperation) { return; }

	DragOperation->DraggedSlotWidget = this;
	DragOperation->FromIndex = Index;

	DragOperation->DefaultDragVisual = this;
	DragOperation->Pivot = EDragPivot::MouseDown;

	OutOperation = DragOperation;
}

void UInventorySlotWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	if (bIsThrowableWeaponSlot) { return; }

	Super::NativeOnDragCancelled(InDragDropEvent, InOperation);

	UInventoryDragDropOperation* DragOperation = Cast<UInventoryDragDropOperation>(InOperation);

	if (!DragOperation) { return; }
	if (!OwnerInventoryWidget) { return; }

	const FVector2D ScreenPosition = InDragDropEvent.GetScreenSpacePosition();
	const bool bInsideInventory = OwnerInventoryWidget->IsScreenPositionInsideInventory(ScreenPosition);
	if (!bInsideInventory)
	{
		OwnerInventoryWidget->RequestDropInventorySlot(DragOperation->FromIndex);
	}
}

bool UInventorySlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	if (bIsThrowableWeaponSlot) { return false; }

	UInventoryDragDropOperation* DragOperation = Cast<UInventoryDragDropOperation>(InOperation);

	if (!DragOperation) { return false; }

	if (!OwnerInventoryWidget) { return false; }

	const int32 FromIndex = DragOperation->FromIndex;
	const int32 ToIndex = Index;

	OwnerInventoryWidget->RequestSwapInventorySlots(FromIndex, ToIndex);

	return true;
}

void UInventorySlotWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	bMouseHoveredOnSlotWidget = true;

	// -----------------------------------------------------
	// Throwable Weapon Slot
	// -----------------------------------------------------
	if (bIsThrowableWeaponSlot)
	{
		SetColorAndOpacity(ThrowableWeaponHoveredColor);

		PlayUISound(ESoundID::UI_Hover);

		return;
	}

	// -----------------------------------------------------
	// 기존 Inventory Slot
	// -----------------------------------------------------
	InitializeItemToolWidget();

	if (ItemToolWidget)
	{
		SetItemToolPosition(InGeometry);
		DisplayItemTool();
	}

	PlayUISound(ESoundID::UI_Hover);
}

void UInventorySlotWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	bMouseHoveredOnSlotWidget = false;

	// -----------------------------------------------------
	// Throwable Weapon Slot
	// -----------------------------------------------------
	if (bIsThrowableWeaponSlot)
	{
		SetColorAndOpacity(DefaultSlotColor);
		return;
	}

	// -----------------------------------------------------
	// 기존 Inventory Slot
	// -----------------------------------------------------
	GetWorld()->GetTimerManager().SetTimer(
		HideToolWidgetTimerHandle,
		this,
		&UInventorySlotWidget::CheckHideToolWidget,
		0.05f,
		false
	);
}

void UInventorySlotWidget::InitializeItemToolWidget()
{
	if (bIsThrowableWeaponSlot) { return; }
	if (IsValid(ItemToolWidget)) { return; }
	if (!ItemToolWidgetClass) { return; }

	ItemToolWidget = CreateWidget<UItemToolWidget>(GetWorld(), ItemToolWidgetClass);
	if (!ItemToolWidget) { return; }
	ItemToolWidget->AddToViewport(static_cast<int32>(EUIZOrder::ItemTool));
	ItemToolWidget->SetVisibility(ESlateVisibility::Hidden);

	ItemToolWidget->OnToolWidgetEnter.AddUObject(this, &UInventorySlotWidget::OnMouseEnterToToolWidget);
	ItemToolWidget->OnToolWidgetLeave.AddUObject(this, &UInventorySlotWidget::OnMouseLeaveFromToolWidget);
	ItemToolWidget->OnUseItemRequested.AddUObject(this, &UInventorySlotWidget::OnUseItemRequested);
	ItemToolWidget->OnDropItemRequested.AddUObject(this, &UInventorySlotWidget::OnDropItemRequested);
}

void UInventorySlotWidget::LoadItemDataTable()
{
	if (ItemDataTable.IsNull()) return;
	LoadedItemTable = ItemDataTable.LoadSynchronous();
}

void UInventorySlotWidget::ClearItemSlotData()
{
	ItemQuantity = 0;
	ItemName = EItemName::ItemName_None;

	if (TextItemQuantity)
	{
		TextItemQuantity->SetText(FText::GetEmpty());
	}

	if (ItemIcon)
	{
		ItemIcon->SetBrushFromTexture(nullptr);
	}

	if (OverlayInventorySlot)
	{
		OverlayInventorySlot->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UInventorySlotWidget::SetItemSlotData(FName ItemDataRowName, int32 InItemQuantity)
{
	if (!LoadedItemTable) { return; }
	FItemData* ItemData = LoadedItemTable->FindRow<FItemData>(ItemDataRowName, TEXT("LoadItemData"));
	if (!ItemData) { return; }

	if (ItemIcon)
	{
		ItemIcon->SetBrushFromTexture(ItemData->ItemImage);
	}

	ItemQuantity = InItemQuantity;

	if (TextItemQuantity)
	{
		TextItemQuantity->SetText(FText::FromString(FString::Printf(TEXT("%d"), ItemQuantity)));
	}

	if (OverlayInventorySlot)
	{
		OverlayInventorySlot->SetVisibility(ESlateVisibility::Visible);
	}
}

void UInventorySlotWidget::SetItemData()
{
}

void UInventorySlotWidget::SetItemToolPosition(const FGeometry& InGeometry)
{
	if (!ItemToolWidget) { return; }

	const FVector2D SlotSize = InGeometry.GetLocalSize();
	const FVector2D SlotRightBottomAbsolute = InGeometry.LocalToAbsolute(SlotSize);

	FVector2D PixelPosition;
	FVector2D ViewportPosition;

	USlateBlueprintLibrary::AbsoluteToViewport(
		GetWorld(),
		SlotRightBottomAbsolute,
		PixelPosition,
		ViewportPosition
	);

	const FVector2D ToolPosition = ViewportPosition + ItemToolOffset;
	ItemToolWidget->SetPositionInViewport(ToolPosition, false);
}

void UInventorySlotWidget::DisplayItemTool()
{
	if (bIsThrowableWeaponSlot)
	{
		return;
	}

	if (ItemToolWidget)
	{
		ItemToolWidget->SetVisibility(
			ESlateVisibility::Visible
		);
	}
}

void UInventorySlotWidget::HideItemTool()
{
	if (ItemToolWidget)
	{
		ItemToolWidget->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UInventorySlotWidget::CheckHideToolWidget()
{
	// ItemTool이 없는 슬롯에서도 안전하게 동작하도록 한다.
	if (!ItemToolWidget) { return; }
	if (!bMouseHoveredOnSlotWidget && !ItemToolWidget->IsHoveredToolWidget())
	{
		HideItemTool();
	}
}

void UInventorySlotWidget::OnMouseEnterToToolWidget()
{
}

void UInventorySlotWidget::OnMouseLeaveFromToolWidget()
{
	if (!bMouseHoveredOnSlotWidget)
	{
		HideItemTool();
	}
}

void UInventorySlotWidget::OnUseItemRequested()
{
	if (!OwnerInventoryWidget)
	{
		return;
	}

	OwnerInventoryWidget->RequestUseInventorySlot(Index);
}

void UInventorySlotWidget::OnDropItemRequested()
{
	if (!OwnerInventoryWidget)
	{
		return;
	}

	OwnerInventoryWidget->RequestDropInventorySlot(Index);
}
