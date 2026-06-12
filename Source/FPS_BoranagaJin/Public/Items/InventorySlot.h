#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Item.h"
#include "ItemName.h"

#include "InventorySlot.generated.h"

USTRUCT(Atomic, BlueprintType)
struct FInventorySlot
{
	GENERATED_USTRUCT_BODY()
public:
	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//FName ItemID = NAME_None;
	UPROPERTY()
	TArray<AItem*> Items;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EItemName ItemName = EItemName::ItemName_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Count = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bOccupied = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxStackCount = 99;
public:

	FInventorySlot()
	{
		ClearSlot();
	}

	void ClearSlot()
	{
		ItemName = EItemName::ItemName_None;
		Count = 0;
		bOccupied = false;
		MaxStackCount = 99;
		Items.Empty();
	}

	bool IsEmpty() const
	{
		return !bOccupied;
	}

	bool IsSameItem(const EItemName& InItemName) const
	{
		return bOccupied && ItemName == InItemName;
	}

	bool CanStack(const EItemName& InItemName) const
	{
		return IsSameItem(InItemName) && Count < MaxStackCount;
	}

	bool AddItem(AItem* InItem, int32 AddCount = 1)
	{
		if (IsEmpty())
		{
			Items.Add(InItem);
 			ItemName = InItem->GetItemName();
			ItemID = InItem->GetItemID();
			Count = AddCount;
			bOccupied = true;
			return true;
		}

		if (IsSameItem(InItem->GetItemName()))
		{
			Count = FMath::Clamp(Count + AddCount, 0, MaxStackCount);
			return true;
		}

		return false;
	}

	bool RemoveItem(int32 RemoveCount = 1)
	{
		if (IsEmpty())
		{
			return false;
		}

		Count -= RemoveCount;

		if (Count <= 0)
		{
			ClearSlot();
		}

		return true;
	}

	AItem* GetItem(int32 ItemIdx = 0)
	{
		if (!IsEmpty() && Count != 0)
		{
			return Items[ItemIdx];
		}
		else
		{
			return nullptr;
		}
	}
};

UCLASS()
class FPS_BORANAGAJIN_API AInventorySlotStruct : public AActor
{
	GENERATED_BODY()
public:
	AInventorySlotStruct();
};
