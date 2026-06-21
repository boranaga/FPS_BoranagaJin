#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "InventoryDragDropOperation.generated.h"

class UInventorySlotWidget;

UCLASS()
class FPS_BORANAGAJIN_API UInventoryDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	UPROPERTY()
	UInventorySlotWidget* DraggedSlotWidget = nullptr;

	UPROPERTY()
	int32 FromIndex = INDEX_NONE;
};