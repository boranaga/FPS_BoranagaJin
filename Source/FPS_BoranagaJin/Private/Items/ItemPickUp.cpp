


#include "Items/ItemPickUp.h"
#include "Items/Item.h"
#include "Items/PickUpComponent.h"
#include "ObjectPoolSubsystem.h"

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

void AItemPickUp::DeactivateItemPickUp()
{
	SetActorLocation(PoolLocation);
	ItemMesh->SetVisibility(false);
}

void AItemPickUp::ActivateItemPickUp(FVector location)
{
	SetActorLocation(location);
	ItemMesh->SetVisibility(true);
}

void AItemPickUp::DestroyItemPickUp()
{
	Destroy();
}

void AItemPickUp::SetOwningPool(UObjectPoolSubsystem* NewPool)
{
	OwningPool = NewPool;
}

void AItemPickUp::OnActivateFromPool()
{
	bIsActiveInPool = true;
}

void AItemPickUp::OnDeactivateToPool()
{
	bIsActiveInPool = false;
}

bool AItemPickUp::IsActiveInPool() const
{
	return bIsActiveInPool;
}

void AItemPickUp::DeactivateItemPickUp_Pool()
{
	if (OwningPool)
	{
		OwningPool->ReturnToPool(this);
	}
}
