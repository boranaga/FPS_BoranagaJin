
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Items/ItemName.h"
#include "InventorySlotWidget.generated.h"

class UTextBlock;
class UButton;
class UImage;
class UOverlay;

UCLASS()
class FPS_BORANAGAJIN_API UInventorySlotWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime);
public:
	UPROPERTY(meta = (BindWidget))
	UTextBlock* TextItemQuantity;
	UPROPERTY(meta = (BindWidget))
	UButton* ButtonInventorySlot;
	UPROPERTY(meta = (BindWidget))
	UImage* ItemIcon;
	UPROPERTY(meta = (BindWidget))
	UOverlay* OverlayInventorySlot;
public:
	UPROPERTY(EditAnywhere, Category = Weapon)
	TSoftObjectPtr<UDataTable> ItemDataTable;
	UPROPERTY(EditAnywhere, Category = Weapon)
	FName ItemRowName;

	UPROPERTY() UDataTable* LoadedItemTable = nullptr;
protected:
	EItemName ItemName;
	int32 ItemQuantity;
	int32 Index;
protected:
	void SetItemData();
};
