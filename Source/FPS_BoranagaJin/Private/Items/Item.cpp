


#include "Items/Item.h"
#include "Items/ItemPickUp.h"
#include "Data/ItemData.h"
#include "Characters/Player/CharacterPlayer.h"
#include "ObjectPoolSubsystem.h"


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

void AItem::LoadItemData()
{
	if (ItemDataTable.IsNull() || ItemRowName.IsNone()) return;
	LoadedItemTable = ItemDataTable.LoadSynchronous();
	if (!LoadedItemTable) return;

	FItemData* ItemData = LoadedItemTable->FindRow<FItemData>(ItemRowName, TEXT("LoadItemData"));
	if (!ItemData) return;

	ItemImage = ItemData->ItemImage;
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
	OwningPool->ReturnToPool(this);
}



