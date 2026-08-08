#pragma once

#include "CoreMinimal.h"
#include "Items/ItemName.h"
#include "InventorySaveData.generated.h"

class AItem;

/**
 * 인벤토리 안에 있는 아이템 인스턴스 하나의 저장 데이터입니다.
 */
USTRUCT(BlueprintType)
struct FPS_BORANAGAJIN_API FItemInstanceSaveData
{
	GENERATED_BODY()

public:
	/**
	 * 저장된 아이템을 복원할 때 Spawn할 클래스입니다.
	 *
	 * 예:
	 * BP_Pistol
	 * BP_Flashlight
	 * BP_Tape
	 */
	UPROPERTY(SaveGame, BlueprintReadOnly)
	TSoftClassPtr<AItem> ItemClass;

	/**
	 * 동일한 종류의 아이템 인스턴스를 구분하는 고유 ID입니다.
	 */
	UPROPERTY(SaveGame, BlueprintReadOnly)
	FGuid InstanceID;

	/**
	 * DataTable 또는 DataAsset 검색용 아이템 ID입니다.
	 */
	UPROPERTY(SaveGame, BlueprintReadOnly)
	FName ItemID = NAME_None;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	EItemName ItemName = EItemName::ItemName_None;

	/**
	 * AItem 및 파생 클래스의 UPROPERTY(SaveGame) 변수들이
	 * 직렬화되어 저장되는 바이트 데이터입니다.
	 */
	UPROPERTY(SaveGame, BlueprintReadOnly)
	TArray<uint8> SerializedState;

public:
	bool IsValid() const
	{
		return
			!ItemClass.IsNull() &&
			!ItemID.IsNone() &&
			ItemName != EItemName::ItemName_None;
	}

	void Reset()
	{
		ItemClass.Reset();
		InstanceID.Invalidate();
		ItemID = NAME_None;
		ItemName = EItemName::ItemName_None;
		SerializedState.Reset();
	}
};

/**
 * 인벤토리 슬롯 하나의 저장 데이터입니다.
 */
USTRUCT(BlueprintType)
struct FPS_BORANAGAJIN_API FInventorySlotSaveData
{
	GENERATED_BODY()

public:
	UPROPERTY(SaveGame, BlueprintReadOnly)
	TArray<FItemInstanceSaveData> Items;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	int32 MaxStackCount = 99;

	//UPROPERTY(SaveGame)
	//EItemName ItemName = EItemName::ItemName_None;

	//UPROPERTY(SaveGame)
	//FName ItemID = NAME_None;

	UPROPERTY(SaveGame)
	int32 Count = 0;

//public:
//	bool IsEmpty() const
//	{
//		return ItemName == EItemName::ItemName_None
//			|| Count <= 0;
//	}



public:
	bool IsEmpty() const
	{
		return Items.IsEmpty();
	}

	int32 GetCount() const
	{
		return Items.Num();
	}

	void Reset()
	{
		Items.Reset();
		MaxStackCount = 99;
	}
};