


#include "Items/ItemPickUp.h"
#include "Items/Item.h"
#include "Items/PickUpComponent.h"
#include "Components/InteractionPointComponent.h"
#include "ObjectPoolSubsystem.h"
#include "Characters/Player/CharacterPlayer.h"
#include "Items/InventorySystemComponent.h"
#include "Engine/World.h"

AItemPickUp::AItemPickUp()
{
    PrimaryActorTick.bCanEverTick = false;
    ItemName = EItemName::ItemName_Base;
    //IntactMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ItemMesh"));
    //IntactMesh->SetupAttachment(RootComponent);

    if (IntactMesh)
    {
        IntactMesh->SetCollisionProfileName(FName("ItemPickUp"));
    }


    PickUpComponent = CreateDefaultSubobject<UPickUpComponent>(TEXT("PickUpComponent"));
    PickUpComponent->SetupAttachment(SceneRoot);
    PickUpComponent->SetCollisionProfileName(FName("ItemPickUp"));

    DefaultPickupInteractionPoint = CreateDefaultSubobject<UInteractionPointComponent>(
        TEXT("DefaultPickupInteractionPoint"));
    DefaultPickupInteractionPoint->SetupAttachment(IntactMesh);
    DefaultPickupInteractionPoint->SetRelativeLocation(FVector(0.f, 0.f, 25.f));

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
    UWorld* World = GetWorld();
    if (!IsValid(World) || !IsValid(Character)) return nullptr;
    const TSubclassOf<AItem> RuntimeClass = GetItemClass();
    if (!RuntimeClass) return nullptr;
    UObjectPoolSubsystem* Pool = World->GetSubsystem<UObjectPoolSubsystem>();
    if (!IsValid(Pool)) return nullptr;

    AItem* NewItem = nullptr;
    {
        // Reuse a matching dormant instance when available; never take an active one.
        const FGuid RequestedID = DroppedItemState.IsValid() ? DroppedItemState.InstanceID : ItemInstanceID;
        AItem* LegacyItem = ItemPtr.Get();
        if (!IsValid(LegacyItem) || LegacyItem->GetInstanceID() != RequestedID)
            LegacyItem = RequestedID.IsValid() ? Cast<AItem>(Pool->FindActorByInstanceIDFromPool(RequestedID, RuntimeClass)) : nullptr;
        bool bAlreadyOwned = false;
        if (IsValid(LegacyItem))
        {
            if (UInventorySystemComponent* Inventory = Character->GetInventorySystemComponent())
            {
                for (EInventoryName Type : { EInventoryName::Item, EInventoryName::PrimaryWeapon, EInventoryName::SecondaryWeapon, EInventoryName::ThrowableWeapon })
                {
                    if (const TArray<FInventorySlot>* Slots = Inventory->GetInventoryConst(Type))
                        for (const FInventorySlot& Slot : *Slots)
                            if (Slot.GetItem() == LegacyItem) bAlreadyOwned = true;
                }
            }
        }
        if (IsValid(LegacyItem) && !bAlreadyOwned && !LegacyItem->IsActiveInPool() && LegacyItem->GetClass() == RuntimeClass.Get())
        {
            NewItem = Cast<AItem>(Pool->GetActorFromAvailablePool(LegacyItem, GetActorLocation(), GetActorRotation()));
            if (IsValid(NewItem)) NewItem->InitItemPost(Character, this);
        }
    }
    if (!IsValid(NewItem))
    {
        NewItem = Pool->SpawnFromPool(RuntimeClass, GetActorLocation(), GetActorRotation(), true);
        if (!IsValid(NewItem)) return nullptr;
        NewItem->InitItem(Character, this);
    }
    if (DroppedItemState.IsValid())
    {
        FItemInstanceSaveData State = DroppedItemState;
        // A pooled actor may have been reused since this pickup was dropped.
        if (State.InstanceID.IsValid())
        {
            AItem* Existing = Cast<AItem>(Pool->FindActorByInstanceIDFromPool(State.InstanceID, RuntimeClass));
            if (IsValid(Existing) && Existing != NewItem) State.InstanceID = FGuid::NewGuid();
        }
        if (!NewItem->LoadItemSaveData(State))
        {
            NewItem->DeactivateItem();
            return nullptr;
        }
    }
    NewItem->SetItemNum(DroppedItemState.IsValid() ? FMath::Max(1, DroppedItemState.ItemNum) : FMath::Max(1, ItemNum));
    // UInteractionComponent returns the pickup to the pool only after commit.
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
    ClearItemLink();
    if (IsValid(NewItem.Get()))
    {
        ItemPtr = NewItem;
        ItemInstanceID = NewItem->GetInstanceID();
    }
}

void AItemPickUp::ClearItemLink()
{
    ItemPtr = nullptr;
    ItemInstanceID.Invalidate();
}

TSubclassOf<AItem> AItemPickUp::GetItemClass() const
{
    return DroppedItemState.IsValid() ? TSubclassOf<AItem>(DroppedItemState.ItemClass.LoadSynchronous()) : ItemClass;
}

FName AItemPickUp::GetItemID() const
{
    if (DroppedItemState.IsValid()) return DroppedItemState.ItemID;
    const TSubclassOf<AItem> Class = GetItemClass();
    const AItem* Defaults = Class ? Class.GetDefaultObject() : nullptr;
    return IsValid(Defaults) ? Defaults->GetItemID() : NAME_None;
}

void AItemPickUp::SetDroppedItemState(const FItemInstanceSaveData& State, int32 InAmmo)
{
    ClearItemLink();
    DroppedItemState = State;
    ItemNum = FMath::Max(1, State.ItemNum);
    if (InAmmo != INDEX_NONE) NumAmmo = FMath::Max(0, InAmmo);
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
    ClearItemLink();
    DroppedItemState.Reset();
    const AItemPickUp* Defaults = GetClass()->GetDefaultObject<AItemPickUp>();
    ItemNum = Defaults ? FMath::Max(1, Defaults->ItemNum) : 1;
    NumAmmo = Defaults ? Defaults->NumAmmo : 10;

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
