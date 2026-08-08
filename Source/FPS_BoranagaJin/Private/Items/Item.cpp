


#include "Items/Item.h"
#include "Items/ItemPickUp.h"
#include "Data/ItemData.h"
#include "Characters/Player/CharacterPlayer.h"
#include "ObjectPoolSubsystem.h"
#include "SaveSystem/InventorySaveData.h"
#include "SaveSystem/SaveGameArchive.h"


// Sets default values
AItem::AItem()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AItem::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AItem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AItem::InitItem(ACharacterPlayer* NewCharacter, AItemPickUp* PickUpActor)
{
	Character = NewCharacter;

	if (!bWasInitialized)
	{
		bWasInitialized = true;
		ItemPickUp = PickUpActor;
		LoadItemData();
	}
}

bool AItem::UseItem(ACharacterPlayer* UsingCharacter)
{
	return false;
}

void AItem::Equip(ACharacterPlayer* TargetCharacter)
{

}

void AItem::Unequip(ACharacterPlayer* TargetCharacter)
{

}

void AItem::LoadItemData()
{
	if (ItemDataTable.IsNull() || ItemRowName.IsNone()) return;
	LoadedItemTable = ItemDataTable.LoadSynchronous();
	if (!LoadedItemTable) return;

	FItemData* ItemData = LoadedItemTable->FindRow<FItemData>(ItemRowName, TEXT("LoadItemData"));
	if (!ItemData) return;

	ItemImage = ItemData->ItemImage;
}

bool AItem::AttachItemToPlayer(ACharacterPlayer* TargetCharacter)
{
	return false;
}

void AItem::SetOwningPool(UObjectPoolSubsystem* NewPool)
{
	OwningPool = NewPool;
}

void AItem::OnActivateFromPool()
{
	bIsActiveInPool = true;
}

void AItem::OnDeactivateToPool()
{
	bIsActiveInPool = false;
}

bool AItem::IsActiveInPool() const
{
	return false;
}

void AItem::DeactivateItemAndGetItemPickUp()
{
	if (OwningPool)
	{
		FVector Offset = Character->GetActorLocation() + Character->GetActorForwardVector() * 100.f;

		OwningPool->GetActorFromAvailablePool(ItemPickUp, Offset, Character->GetActorRotation());
		OwningPool->ReturnToPool(this);
	}
}

void AItem::DeactivateItem()
{
	if (!OwningPool) { return; }
	OwningPool->ReturnToPool(this);
}

bool AItem::WriteItemSaveData(FItemInstanceSaveData& OutSaveData)
{
	OutSaveData.Reset();
	if (ItemID.IsNone() || ItemName == EItemName::ItemName_None)
	{
		return false;
	}

	if (!InstanceID.IsValid())
	{
		InstanceID = FGuid::NewGuid();
	}

	OutSaveData.ItemClass = GetClass();
	OutSaveData.InstanceID = InstanceID;
	OutSaveData.ItemID = ItemID;
	OutSaveData.ItemName = ItemName;

	FMemoryWriter MemoryWriter(OutSaveData.SerializedState, true);
	FSaveGameArchive Archive(MemoryWriter);

	Serialize(Archive);

	MemoryWriter.Close();

	UE_LOG(LogTemp, Error, TEXT("bool AItem::WriteItemSaveData(FItemInstanceSaveData& OutSaveData)"));

	return true;
}

bool AItem::LoadItemSaveData(const FItemInstanceSaveData& SaveData)
{
	if (!SaveData.IsValid()) { return false; }

	InstanceID = SaveData.InstanceID;
	ItemID = SaveData.ItemID;
	ItemName = SaveData.ItemName;

	if (!SaveData.SerializedState.IsEmpty())
	{
		FMemoryReader MemoryReader(SaveData.SerializedState, true);
		FSaveGameArchive Archive(MemoryReader);
		Serialize(Archive);
		MemoryReader.Close();
	}

	OnItemStateRestored();

	return true;
}

void AItem::OnItemStateRestored()
{
	// BaseItem의 경우 후처리 안함
	// Child Class에서 Override 예정
}



