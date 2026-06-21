
#pragma once

#include "CoreMinimal.h"
#include "BaseUIWidget.h"
#include "Items/ItemName.h"
#include "InventorySlotWidget.generated.h"

class UTextBlock;
class UButton;
class UImage;
class UOverlay;
class UCanvasPanel;
class UCanvasPanelSlot;

class UItemToolWidget;

UCLASS()
class FPS_BORANAGAJIN_API UInventorySlotWidget : public UBaseUIWidget
{
	GENERATED_BODY()
public:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime);

	virtual void NativeOnMouseEnter(
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent) override;

	virtual void NativeOnMouseLeave(
		const FPointerEvent& InMouseEvent) override;

	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
public:
	UPROPERTY(meta = (BindWidget))
	UTextBlock* TextItemQuantity;
	UPROPERTY(meta = (BindWidget))
	UButton* ButtonInventorySlot;
	UPROPERTY(meta = (BindWidget))
	UImage* ItemIcon;
	UPROPERTY(meta = (BindWidget))
	UOverlay* OverlayInventorySlot;
	UPROPERTY(meta = (BindWidget))
	UCanvasPanel* ItemToolCanvas;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ItemTool")
	FVector2D ItemToolOffset = FVector2D(-50.f, -50.f);
public:
	UPROPERTY(EditAnywhere, Category = Weapon)
	TSoftObjectPtr<UDataTable> ItemDataTable;
	UPROPERTY() UDataTable* LoadedItemTable = nullptr;
protected:
	void LoadItemDataTable();
public:
	void SetItemSlotData(FName ItemDataRowName, int32 InItemQuantity = 1);
protected:
	EItemName ItemName;
	int32 ItemQuantity;
	int32 Index;

	UPROPERTY(EditAnywhere, BlueprintreadWrite, Category = "ItemInventoryUIWidget")
	TSubclassOf<UItemToolWidget> ItemToolWidgetClass;
	UItemToolWidget* ItemToolWidget = nullptr;

protected:
	bool bMouseHoveredOnSlotWidget = false;
	bool bMouseHoveredOnToolWidget = false;

	FTimerHandle HideToolWidgetTimerHandle;
protected:
	void SetItemData();
	void SetItemToolPosition(const FGeometry& InGeometry);
	void DisplayItemTool();
	void HideItemTool();
	void CheckHideToolWidget();
	void OnMouseEnterToToolWidget();
	void OnMouseLeaveFromToolWidget();
};
