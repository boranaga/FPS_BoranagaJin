#pragma once

#include "CoreMinimal.h"
#include "Item.h"
#include "ItemName.h"
#include "InventorySlot.generated.h"

USTRUCT(BlueprintType)
struct FPS_BORANAGAJIN_API FInventorySlot
{
	GENERATED_BODY()
public:
	UPROPERTY(Transient)
	TArray<TObjectPtr<AItem>> Items;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EItemName ItemName = EItemName::ItemName_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ItemID = NAME_None;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//int32 Count = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxStackCount = 99;
public:

	FInventorySlot() { ClearSlot(); }

	void ClearSlot()
	{
		ItemName = EItemName::ItemName_None;
		ItemID = NAME_None;
		//Count = 0;
		MaxStackCount = 99;
		Items.Empty();
		//Items.Reset(); //TODO: Reset하면 AItem이 Destroy 될 수 있기 때문에 일단 그냥 Empty로 놔둠
	}

	bool IsEmpty() const
	{
		return Items.IsEmpty();
	}

	int32 Num() const
	{
		return Items.Num();
	}

	bool IsSameItem(const EItemName& InItemName) const
	{
		return !IsEmpty() && ItemName == InItemName;
	}

	bool CanStack(const EItemName& InItemName) const
	{
		//return IsSameItem(InItemName) && Count < MaxStackCount;
		return IsSameItem(InItemName) && Items.Num() < MaxStackCount;
	}

	bool AddItem(AItem* InItem, int32 AddCount = 1) //TODO: AddCount가 필요한가
	{
		if (!IsValid(InItem))
		{
			return false;
		}

		//if (!CanStack(InItem->GetItemName()))
		//{
		//	return false;
		//}

		if (IsEmpty())
		{
			Items.Add(InItem);
 			ItemName = InItem->GetItemName();
			ItemID = InItem->GetItemID();
			//Count = AddCount;
			return true;
		}


		//if (IsSameItem(InItem->GetItemName()))
		//{
		//	Count = FMath::Clamp(Count + AddCount, 0, MaxStackCount);
		//	return true;
		//}

		Items.Add(InItem);

		//return false;
		return true;
	}

	AItem* RemoveItem(int32 ItemIndex)
	{
		if (IsEmpty())
		{
			return nullptr;
		}

		if (ItemIndex == INDEX_NONE)
		{
			ItemIndex = Items.Num() - 1;
		}

		if (!Items.IsValidIndex(ItemIndex))
		{
			return nullptr;
		}

		AItem* RemovedItem = Items[ItemIndex];

		Items.RemoveAt(ItemIndex);

		if (Items.IsEmpty())
		{
			ClearSlot();
		}

		return RemovedItem;
	}


	AItem* GetItem(int32 ItemIdx = 0)
	{
		//if (!IsEmpty() && Count != 0)
		//{
		//	return Items[ItemIdx];
		//}
		//else
		//{
		//	return nullptr;
		//}

		return Items.IsValidIndex(ItemIdx) ? Items[ItemIdx].Get() : nullptr;
	}
};
