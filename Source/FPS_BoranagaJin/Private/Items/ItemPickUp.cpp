


#include "Items/ItemPickUp.h"
#include "Items/Item.h"
#include "Items/PickUpComponent.h"

AItemPickUp::AItemPickUp()
{
	PrimaryActorTick.bCanEverTick = false;
	ItemName = EItemName::ItemName_Base;
	ItemMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ItemMesh"));
	ItemMesh->SetupAttachment(RootComponent);

	PickUpComponent = CreateDefaultSubobject<UPickUpComponent>(TEXT("PickUpComponent"));
	PickUpComponent->SetupAttachment(ItemMesh);
}

void AItemPickUp::BeginPlay()
{
	Super::BeginPlay();
}

AItem* AItemPickUp::SpawnItem(ACharacterPlayer* Character)
{
	AItem* NewItem = nullptr;
	if (ItemClass != nullptr)
	{
		UWorld* const World = GetWorld();
		if (World != nullptr && Character != nullptr)
		{
			FActorSpawnParameters ActorSpawnParams;
			ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			NewItem = GetWorld()->SpawnActor<AItem>(ItemClass, GetActorTransform(), ActorSpawnParams);
			NewItem->InitItem(Character);
		}
	}
	return NewItem;
}

void AItemPickUp::DestroyItemPickUp()
{
	Destroy();
}
