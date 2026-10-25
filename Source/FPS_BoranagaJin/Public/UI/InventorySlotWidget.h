#pragma once

#include "CoreMinimal.h"
#include "BaseUIWidget.h"
#include "Items/ItemName.h"
#include "InventorySlotWidget.generated.h"

class UInventoryUIWidget;
class UThrowableWeaponInventoryWidget;

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
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual void NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

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
	void SetIndex(int32 InIndex) { Index = InIndex; }
	int32 GetIndex() const { return Index; }

	void SetOwnerInventoryWidget(UInventoryUIWidget* InOwner) { OwnerInventoryWidget = InOwner; }
	void SetOwnerThrowableWeaponInventoryWidget() {}

protected:
	UPROPERTY()
	UInventoryUIWidget* OwnerInventoryWidget = nullptr;

public:
	UPROPERTY(EditAnywhere, Category = Weapon)
	TSoftObjectPtr<UDataTable> ItemDataTable;

	UPROPERTY()
	UDataTable* LoadedItemTable = nullptr;

protected:
	void LoadItemDataTable();

public:
	void ClearItemSlotData();
	void SetItemSlotData(FName ItemDataRowName, int32 InItemQuantity = 1);

protected:
	EItemName ItemName = EItemName::ItemName_None;
	int32 ItemQuantity = 0;
	int32 Index = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ItemInventoryUIWidget")
	TSubclassOf<UItemToolWidget> ItemToolWidgetClass;

	UPROPERTY()
	UItemToolWidget* ItemToolWidget = nullptr;

#pragma region ThrowableWeaponSlot
public:
	void ConfigureAsThrowableWeaponSlot(UThrowableWeaponInventoryWidget* InOwner);

protected:
	UPROPERTY()
	UThrowableWeaponInventoryWidget* OwnerThrowableWeaponInventoryWidget = nullptr;

	bool bIsThrowableWeaponSlot = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ThrowableWeaponSlot")
	FLinearColor ThrowableWeaponHoveredColor = FLinearColor(0.45f, 0.45f, 0.45f, 1.0f);

	FLinearColor DefaultSlotColor = FLinearColor::White;
#pragma endregion

protected:
	bool bMouseHoveredOnSlotWidget = false;
	bool bMouseHoveredOnToolWidget = false;

	FTimerHandle HideToolWidgetTimerHandle;

protected:
	void SetItemData();

	void InitializeItemToolWidget();
	void SetItemToolPosition(const FGeometry& InGeometry);
	void DisplayItemTool();
	void HideItemTool();
	void CheckHideToolWidget();

	void OnMouseEnterToToolWidget();
	void OnMouseLeaveFromToolWidget();

	void OnUseItemRequested();
	void OnDropItemRequested();
};