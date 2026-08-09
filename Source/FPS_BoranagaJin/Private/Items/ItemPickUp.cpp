


#include "Items/ItemPickUp.h"
#include "Items/Item.h"
#include "Items/PickUpComponent.h"
#include "ObjectPoolSubsystem.h"

AItemPickUp::AItemPickUp()
{
	PrimaryActorTick.bCanEverTick = false;
	ItemName = EItemName::ItemName_Base;
	//IntactMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ItemMesh"));
	//IntactMesh->SetupAttachment(RootComponent);

	PickUpComponent = CreateDefaultSubobject<UPickUpComponent>(TEXT("PickUpComponent"));
	PickUpComponent->SetupAttachment(SceneRoot);


	bDestroyActorAfterBreak = false;
	bHideBrokenMeshAfterDelay = false;
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

	//if (ItemClass == nullptr) { return nullptr; }
	//UObjectPoolSubsystem* PoolSubsystem = GetWorld()->GetSubsystem<UObjectPoolSubsystem>();
	//if (PoolSubsystem == nullptr) { return nullptr; }
	//AItem* NewItem = nullptr;
	//UWorld* const World = GetWorld();
	//if (World != nullptr && Character != nullptr)
	//{
	//	NewItem = PoolSubsystem->SpawnFromPool(ItemClass, GetActorLocation(), GetActorRotation());
	//	NewItem->InitItem(Character, this);
	//	ItemPtr = NewItem;
	//}

	//return NewItem;

	//---------------------------------------------
	// <New Version>

	if (ItemClass == nullptr) { return nullptr; }
	UObjectPoolSubsystem* PoolSubsystem = GetWorld()->GetSubsystem<UObjectPoolSubsystem>();
	if (PoolSubsystem == nullptr) { return nullptr; }
	AItem* NewItem = nullptr;
	UWorld* const World = GetWorld();
	if (World != nullptr && Character != nullptr)
	{
		if (ItemPtr)
		{
			UE_LOG(LogTemp, Warning, TEXT("AItem* AItemPickUp::SpawnItem(ACharacterPlayer* Character) 1"));

			NewItem = Cast<AItem>(PoolSubsystem->GetActorFromAvailablePool(ItemPtr, GetActorLocation(), GetActorRotation()));
			NewItem->InitItem(Character, this);
			ItemPtr = NewItem;
			ItemInstanceID = NewItem->GetInstanceID();
			return NewItem;
		}

		if (ItemInstanceID.IsValid())
		{
			NewItem = Cast<AItem>(PoolSubsystem->FindActorByInstanceIDFromPool(ItemInstanceID, ItemClass));
			if (NewItem)
			{
				UE_LOG(LogTemp, Warning, TEXT("AItem* AItemPickUp::SpawnItem(ACharacterPlayer* Character) 2"));

				//MEMO: Activate가 안되어있을 경우 Activate 시키기 위한 역할
				Cast<AItem>(PoolSubsystem->GetActorFromAvailablePool(NewItem, GetActorLocation(), GetActorRotation()));
				NewItem->InitItemPost(Character, this);
				ItemPtr = NewItem;
				ItemInstanceID = NewItem->GetInstanceID();
				return NewItem;
			}
			else
			{
				NewItem = PoolSubsystem->SpawnFromPool(ItemClass, GetActorLocation(), GetActorRotation(), true);
				if (NewItem)
				{
					UE_LOG(LogTemp, Warning, TEXT("AItem* AItemPickUp::SpawnItem(ACharacterPlayer* Character) 3"));

					NewItem->InitItem(Character, this);
					ItemPtr = NewItem;
					ItemInstanceID = NewItem->GetInstanceID();
					return NewItem;
				}
			}
		}


		NewItem = PoolSubsystem->SpawnFromPool(ItemClass, GetActorLocation(), GetActorRotation(), true);
		if (NewItem)
		{
			UE_LOG(LogTemp, Warning, TEXT("AItem* AItemPickUp::SpawnItem(ACharacterPlayer* Character) 4"));

			NewItem->InitItem(Character, this);
			ItemPtr = NewItem;
			ItemInstanceID = NewItem->GetInstanceID();
		}
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
	IntactMesh->SetVisibility(true);
}

void AItemPickUp::DestroyItemPickUp()
{
	Destroy();
}

void AItemPickUp::SetItemPtr(TObjectPtr<AItem> NewItem)
{
	if (NewItem)
	{
		ItemPtr = NewItem;
		ItemInstanceID = NewItem->GetInstanceID();
	}
}

void AItemPickUp::OnAfterLoad()
{
	Super::OnAfterLoad();

	if (bIsActiveInPool)
	{
		OnActivateFromPool();
	}
	else
	{
		DeactivateItemPickUp_Pool();
	}
}

void AItemPickUp::SetOwningPool(UObjectPoolSubsystem* NewPool)
{
	OwningPool = NewPool;
}

void AItemPickUp::OnActivateFromPool()
{
	bIsActiveInPool = true;

	SetActorHiddenInGame(false);

	SetActorEnableCollision(true);
	SetActorTickEnabled(true);
}

void AItemPickUp::OnDeactivateToPool()
{
	bIsActiveInPool = false;

	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);
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
