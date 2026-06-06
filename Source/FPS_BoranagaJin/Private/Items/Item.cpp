


#include "Items/Item.h"
#include "Data/ItemData.h"
#include "Characters/Player/CharacterPlayer.h"


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

void AItem::InitItem(ACharacterPlayer* NewCharacter)
{
	Character = NewCharacter;

	LoadItemData();
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


