


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

	if (UObjectPoolSubsystem* PoolSubsystem = GetWorld()->GetSubsystem<UObjectPoolSubsystem>())
	{
		const bool bRegistered = PoolSubsystem->RegisterActor(this, true);

		if (!bRegistered)
		{
			UE_LOG(LogTemp, Warning, TEXT("AItemPickUp Pool 등록 실패 또는 이미 등록됨: %s"), *GetName());
		}
	}
}

AItem* AItemPickUp::SpawnItem(ACharacterPlayer* Character)
{
	// <Old Version>

	//AItem* NewItem = nullptr;
	//if (ItemClass != nullptr)
	//{
	//	UWorld* const World = GetWorld();
	//	if (World != nullptr && Character != nullptr)
	//	{
	//		FActorSpawnParameters ActorSpawnParams;
	//		ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	//		NewItem = GetWorld()->SpawnActor<AItem>(ItemClass, GetActorTransform(), ActorSpawnParams);
	//		NewItem->InitItem(Character, this);
	//	}
	//}
	//return NewItem;

	//---------------------------------------------
	// <Object Pooling Version>

	if (ItemClass == nullptr) { return nullptr; }
	UObjectPoolSubsystem* PoolSubsystem = GetWorld()->GetSubsystem<UObjectPoolSubsystem>();
	if (PoolSubsystem == nullptr) { return nullptr; }
	AItem* NewItem = nullptr;
	UWorld* const World = GetWorld();
	if (World != nullptr && Character != nullptr)
	{
		NewItem = PoolSubsystem->SpawnFromPool(ItemClass, GetActorLocation(), GetActorRotation());
		NewItem->InitItem(Character, this);
		ItemPtr = NewItem;
	}

	return NewItem;
}

void AItemPickUp::DeactivateItemPickUp()
{
	//// <Old Version>
	//SetActorLocation(PoolLocation);
	//ItemMesh->SetVisibility(false);

	//---------------------------------------------
	// <Object Pooling Version>
	DeactivateItemPickUp_Pool();
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
