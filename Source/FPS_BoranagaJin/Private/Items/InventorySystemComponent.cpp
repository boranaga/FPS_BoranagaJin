// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/InventorySystemComponent.h"
#include "Items/Item.h"
#include "Items/InventorySlot.h"
#include "Items/ItemPickUp.h"
#include "Items/Weapons/Weapon.h"
#include "Items/Weapons/ThrowableWeapon/ThrowableWeapon.h"
#include "Items/FlashlightItem.h"
#include "Characters/Player/CharacterPlayer.h"

#include "Items/Weapons/WeaponName.h"

#include "ObjectPoolSubsystem.h"


#include "Kismet/GameplayStatics.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"

UInventorySystemComponent::UInventorySystemComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UInventorySystemComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializePlayerReference();

	InitInventory();

	PlayerOwner->OnInventorySwapRequestedDelegate.AddUObject(this, &UInventorySystemComponent::SwapItemInventorySlots);
	PlayerOwner->OnInventorySlotDropRequestedDelegate.AddUObject(this, &UInventorySystemComponent::DropItemInventorySlot);
	PlayerOwner->OnInventorySlotUseRequestedDelegate.AddUObject(this, &UInventorySystemComponent::UseItemInventorySlot);

	PlayerOwner->OnThrowableWeaponEquipRequestedDelegate.AddUObject(this, &UInventorySystemComponent::EquipThrowableWeapon);
	PlayerOwner->OnThrowableWeaponInventoryUpdateRequestedDelegate.AddUObject(this, &UInventorySystemComponent::UpdateThrowableWeaponInventorySlots);
	//PlayerOwner->OnThrowableWeaponReleaseNotify.AddUObject(this, &UInventorySystemComponent::HandleThrowableWeaponReleaseNotify);
}

void UInventorySystemComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	//CalculateScreenCenterWorldPositionAndDirection(ScreenCenterWorldLocation, ScreenCenterWorldDirection);
	CalculateTargetRightHandPosition();
	UpdateOffHandFlashlightAttachment();
}

#pragma region PlayerReference
void UInventorySystemComponent::InitializePlayerReference()
{
	ACharacterPlayer* NewPlayerOwner = Cast<ACharacterPlayer>(GetOwner());
	if (IsValid(NewPlayerOwner))
	{
		PlayerOwner = NewPlayerOwner;
		PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		//PlayerController = Cast<APlayerController>(PlayerOwner->GetController())

		// Set up action bindings
		if (PlayerController)
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
			{
				// Set the priority of the mapping to 1, so that it overrides the Jump action with the Fire action when using touch input
				Subsystem->AddMappingContext(WeaponSystemMappingContext, 1);
			}

			if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent))
			{
				//// Switch Weapon (Legacy)
				//EnhancedInputComponent->BindAction(SwitchWeaponUpAction, ETriggerEvent::Started, this, &UInventorySystemComponent::SwitchToNextWeapon);
				//EnhancedInputComponent->BindAction(SwitchWeaponDownAction, ETriggerEvent::Started, this, &UInventorySystemComponent::SwitchToPreviousWeapon);

				//EnhancedInputComponent->BindActionValueLambda(SwitchWeapon1Action, ETriggerEvent::Started, [this](const FInputActionValue& InputActionValue, int32 idx) {SwitchToIndex(idx); }, 0);
				//EnhancedInputComponent->BindActionValueLambda(SwitchWeapon2Action, ETriggerEvent::Started, [this](const FInputActionValue& InputActionValue, int32 idx) {SwitchToIndex(idx); }, 1);
				//EnhancedInputComponent->BindActionValueLambda(SwitchWeapon3Action, ETriggerEvent::Started, [this](const FInputActionValue& InputActionValue, int32 idx) {SwitchToIndex(idx); }, 2);

				// Flash Light
				EnhancedInputComponent->BindAction(FlashLightAction, ETriggerEvent::Started, this, &UInventorySystemComponent::ToggleFlashLight);

				//----------------------------------------------

				EnhancedInputComponent->BindActionValueLambda(
					QuickSlot1Action, ETriggerEvent::Started,
					[this](const FInputActionValue& Value)
					{
						SelectQuickSlot(0);
					}
				);

				EnhancedInputComponent->BindActionValueLambda(
					QuickSlot2Action, ETriggerEvent::Started,
					[this](const FInputActionValue& Value)
					{
						SelectQuickSlot(1);
					}
				);

				EnhancedInputComponent->BindActionValueLambda(
					QuickSlot3Action, ETriggerEvent::Started,
					[this](const FInputActionValue& Value)
					{
						SelectQuickSlot(2);
					}
				);

				EnhancedInputComponent->BindActionValueLambda(
					QuickSlot4Action, ETriggerEvent::Started,
					[this](const FInputActionValue& Value)
					{
						SelectQuickSlot(3);
					}
				);

				EnhancedInputComponent->BindActionValueLambda(
					QuickSlot5Action, ETriggerEvent::Started,
					[this](const FInputActionValue& Value)
					{
						SelectQuickSlot(4);
					}
				);

				EnhancedInputComponent->BindAction(
					QuickSlotScrollAction,
					ETriggerEvent::Triggered,
					this,
					&UInventorySystemComponent::HandleQuickSlotScroll
				);
			}
		}
	}
}

bool UInventorySystemComponent::WriteInventorySaveData(TArray<FInventorySlotSaveData>& OutSaveData, EInventoryName inventoryname) const
{
	const TArray<FInventorySlot>* InventoryToSave = GetInventoryConst(inventoryname);
	if (!InventoryToSave) { return false; }

	OutSaveData.Reset();
	OutSaveData.Reserve(InventoryToSave->Num());

	bool bAllItemsSaved = true;

	for (int32 SlotIndex = 0; SlotIndex < InventoryToSave->Num(); ++SlotIndex)
	{
		const FInventorySlot& RuntimeSlot = (*InventoryToSave)[SlotIndex];

		FInventorySlotSaveData SlotSaveData;
		SlotSaveData.MaxStackCount = RuntimeSlot.MaxStackCount;
		//SlotSaveData.Count = RuntimeSlot.Count;

		for (AItem* Item : RuntimeSlot.Items)
		{
			if (!IsValid(Item))
			{
				bAllItemsSaved = false;
				continue;
			}

			FItemInstanceSaveData ItemSaveData;

			if (!Item->WriteItemSaveData(ItemSaveData))
			{
				bAllItemsSaved = false;
				continue;
			}

			SlotSaveData.Items.Add(MoveTemp(ItemSaveData));
		}

		OutSaveData.Add(MoveTemp(SlotSaveData));
	}

	return bAllItemsSaved;
}

bool UInventorySystemComponent::RestoreInventoryFromSaveData(const TArray<FInventorySlotSaveData>& SaveData, EInventoryName InventoryName)
{
	TArray<FInventorySlot>* Inventory = GetInventory(InventoryName);
	if (!Inventory) { return false; }

	// Never keep a current equip/pending pointer to an actor that will be returned to the pool.
	for (const FInventorySlot& Slot : *Inventory)
	{
		for (AItem* Item : Slot.Items)
		{
			if (CurrItem == Item) { CurrItem = nullptr; }
			if (NextItem == Item) { NextItem = nullptr; }
			if (CurrWeapon == Item) { CurrWeapon = nullptr; CurrWeaponIdx = INDEX_NONE; }
			if (CurrThrowableWeapon == Item) { CurrThrowableWeapon = nullptr; CurrThrowableWeaponIdx = INDEX_NONE; }
			if (CurrFlashLightItem == Item)
			{
				if (bOffHandFlashlightActive) { DisableOffHandFlashlight(); }
				CurrFlashLightItem = nullptr;
				bIsUsingFlashLightItem = false;
			}
		}
	}
	ClearInventoryForRestore(*Inventory);
	const int32 RequiredSlots = InventoryName == EInventoryName::PrimaryWeapon
		? FMath::Max(MaxPrimaryWeaponSlotsCount, 2)
		: (InventoryName == EInventoryName::SecondaryWeapon
			? FMath::Max(MaxSecondaryWeaponSlotsCount, 1)
			: (InventoryName == EInventoryName::Item
				? FMath::Max(MaxItemSlotsCount, 0)
				: FMath::Max(MaxThrowableWeaponSlotsCount, 0)));
	Inventory->SetNum(FMath::Max(SaveData.Num(), RequiredSlots));
	// Previous QuickSlot weak refs may still be valid pooled actors. Clear
	// their now-missing source ownership before assigning restored instances.
	SynchronizeQuickSlots();

	bool bAllRestored = true;
	for (int32 SlotIndex = 0; SlotIndex < SaveData.Num(); ++SlotIndex)
	{
		const FInventorySlotSaveData& SavedSlot = SaveData[SlotIndex];
		FInventorySlot& Slot = (*Inventory)[SlotIndex];
		Slot.ClearSlot();
		Slot.MaxStackCount = FMath::Max(SavedSlot.MaxStackCount, 1);
		for (const FItemInstanceSaveData& SavedItem : SavedSlot.Items)
		{
			AItem* Item = CreateRuntimeItemFromSaveData(SavedItem);
			if (!IsValid(Item)) { bAllRestored = false; continue; }

			if ((InventoryName == EInventoryName::PrimaryWeapon || InventoryName == EInventoryName::SecondaryWeapon) &&
				(!Item->IsA(AWeapon::StaticClass()) ||
					CastChecked<AWeapon>(Item)->GetQuickSlotType() !=
					(InventoryName == EInventoryName::PrimaryWeapon ? EQuickSlotType::PrimaryWeapon : EQuickSlotType::SecondaryWeapon)))
			{
				Item->DeactivateItem();
				bAllRestored = false;
				continue;
			}
			if (!Slot.AddItem(Item))
			{
				Item->DeactivateItem();
				bAllRestored = false;
			}
		}
	}
	if (InventoryName == EInventoryName::PrimaryWeapon || InventoryName == EInventoryName::SecondaryWeapon)
	{
		const EQuickSlotType SlotType = InventoryName == EInventoryName::PrimaryWeapon
			? EQuickSlotType::PrimaryWeapon : EQuickSlotType::SecondaryWeapon;
		// No persisted quick-slot assignment was available in this two-file input;
		// populate any unassigned matching slots from the restored category.
		for (int32 Index = 0; Index < Inventory->Num(); ++Index)
		{
			if ((*Inventory)[Index].IsEmpty()) { continue; }
			int32 ExistingQuickSlot = INDEX_NONE;
			if (IsItemInQuickSlots((*Inventory)[Index].GetItem(0), ExistingQuickSlot)) { continue; }
			const int32 FreeQuickSlot = FindEmptyQuickSlot(SlotType);
			if (FreeQuickSlot != INDEX_NONE) { RegisterQuickSlot(FreeQuickSlot, InventoryName, Index); }
		}
	}
	NotifyInventoryRestored();
	return bAllRestored;
}


AItem* UInventorySystemComponent::CreateRuntimeItemFromSaveData(const FItemInstanceSaveData& SaveData)
{
	//if (!SaveData.IsValid()) { return nullptr; }
	//UWorld* World = GetWorld();
	//if (!IsValid(World)) { return nullptr; }

	////UClass* LoadedItemClass = SaveData.ItemClass.LoadSynchronous();
	//TSubclassOf<AItem> LoadedItemClass = SaveData.ItemClass.LoadSynchronous();

	//if (!IsValid(LoadedItemClass)) { 
	//	UE_LOG(LogTemp, Warning, TEXT("AItem* UInventorySystemComponent::CreateRuntimeItemFromSaveData(const FItemInstanceSaveData& SaveData) 1")); return nullptr; }
	//if (!LoadedItemClass->IsChildOf(AItem::StaticClass()))
	//{
	//	UE_LOG(LogTemp, Warning, TEXT("AItem* UInventorySystemComponent::CreateRuntimeItemFromSaveData(const FItemInstanceSaveData& SaveData) 2"));
	//	return nullptr;
	//}


	////TODO: PoolSubsystem 사용
	////TODO: ItemPickUp과 결합도가 높은데 얘는 어디서 관리해야할까
	//UObjectPoolSubsystem* PoolSubsystem = World->GetSubsystem<UObjectPoolSubsystem>();
	//if (!PoolSubsystem) { 
	//	UE_LOG(LogTemp, Warning, TEXT("AItem* UInventorySystemComponent::CreateRuntimeItemFromSaveData(const FItemInstanceSaveData& SaveData) 3")); return nullptr; }
	//AItem* NewItem = nullptr;
	//if (PlayerOwner)
	//{
	//	NewItem = PoolSubsystem->SpawnFromPool(LoadedItemClass, PlayerOwner->GetActorLocation(), PlayerOwner->GetActorRotation());
	//	NewItem->InitItem(PlayerOwner, nullptr); //MEMO: 일단 PickUp은 ullptr로 넣어줬음
	//	// ItemPtr = NewItem; //TODO: 향후 생성할 ItemPick과 1대1 매칭을 위한 것인데, 아직 아이템 픽업에 대한 처리가 없음
	//}
	//if (!IsValid(NewItem)) { 
	//	UE_LOG(LogTemp, Warning, TEXT("AItem* UInventorySystemComponent::CreateRuntimeItemFromSaveData(const FItemInstanceSaveData& SaveData) 4")); return nullptr; }

	//if (!NewItem->LoadItemSaveData(SaveData))
	//{
	//	UE_LOG(LogTemp, Warning, TEXT("AItem* UInventorySystemComponent::CreateRuntimeItemFromSaveData(const FItemInstanceSaveData& SaveData) 5"));
	//	NewItem->DeactivateItem();
	//	return nullptr;
	//}

	//UE_LOG(LogTemp, Warning, TEXT("AItem* UInventorySystemComponent::CreateRuntimeItemFromSaveData(const FItemInstanceSaveData& SaveData) 6"));

	//return NewItem;

	//----------------------------------------------------------------------------------
	// <New Version>


	if (!SaveData.IsValid()) { return nullptr; }
	UWorld* World = GetWorld();
	if (!IsValid(World)) { return nullptr; }

	//UClass* LoadedItemClass = SaveData.ItemClass.LoadSynchronous();
	TSubclassOf<AItem> LoadedItemClass = SaveData.ItemClass.LoadSynchronous();

	if (!IsValid(LoadedItemClass)) {
		UE_LOG(LogTemp, Warning, TEXT("AItem* UInventorySystemComponent::CreateRuntimeItemFromSaveData(const FItemInstanceSaveData& SaveData) 1")); return nullptr;
	}
	if (!LoadedItemClass->IsChildOf(AItem::StaticClass()))
	{
		UE_LOG(LogTemp, Warning, TEXT("AItem* UInventorySystemComponent::CreateRuntimeItemFromSaveData(const FItemInstanceSaveData& SaveData) 2"));
		return nullptr;
	}


	//TODO: PoolSubsystem 사용
	//TODO: ItemPickUp과 결합도가 높은데 얘는 어디서 관리해야할까
	UObjectPoolSubsystem* PoolSubsystem = World->GetSubsystem<UObjectPoolSubsystem>();
	if (!PoolSubsystem) {
		UE_LOG(LogTemp, Warning, TEXT("AItem* UInventorySystemComponent::CreateRuntimeItemFromSaveData(const FItemInstanceSaveData& SaveData) 3")); return nullptr;
	}
	AItem* NewItem = nullptr;
	if (PlayerOwner)
	{
		if (SaveData.InstanceID.IsValid())
		{
			NewItem = Cast<AItem>(PoolSubsystem->FindActorByInstanceIDFromPool(SaveData.InstanceID, LoadedItemClass));
			if (NewItem)
			{
				NewItem->InitItem(PlayerOwner, nullptr);
			}
			else
			{
				NewItem = PoolSubsystem->SpawnFromPool(LoadedItemClass, PlayerOwner->GetActorLocation(), PlayerOwner->GetActorRotation());
				if (NewItem)
				{
					NewItem->InitItem(PlayerOwner, nullptr);
				}
			}
		}
		else
		{
			NewItem = PoolSubsystem->SpawnFromPool(LoadedItemClass, PlayerOwner->GetActorLocation(), PlayerOwner->GetActorRotation());
			if (NewItem)
			{
				NewItem->InitItem(PlayerOwner, nullptr);
			}
			//MEMO: 일단 PickUp은 ullptr로 넣어줬음
			// ItemPtr = NewItem; //TODO: 향후 생성할 ItemPick과 1대1 매칭을 위한 것인데, 아직 아이템 픽업에 대한 처리가 없음
		}
	}
	if (!IsValid(NewItem))
	{
		UE_LOG(LogTemp, Warning, TEXT("AItem* UInventorySystemComponent::CreateRuntimeItemFromSaveData(const FItemInstanceSaveData& SaveData) 4"));
		return nullptr;
	}

	if (!NewItem->LoadItemSaveData(SaveData))
	{
		UE_LOG(LogTemp, Warning, TEXT("AItem* UInventorySystemComponent::CreateRuntimeItemFromSaveData(const FItemInstanceSaveData& SaveData) 5"));
		NewItem->DeactivateItem();
		return nullptr;
	}

	UE_LOG(LogTemp, Warning, TEXT("AItem* UInventorySystemComponent::CreateRuntimeItemFromSaveData(const FItemInstanceSaveData& SaveData) 6"));

	return NewItem;
}

void UInventorySystemComponent::ClearInventoryForRestore(TArray<FInventorySlot>& TargetInventory)
{
	// A throwable stack may contain the same actor pointer multiple times.
	// Return every actual actor to the pool only once.
	TSet<AItem*> UniqueItems;
	for (FInventorySlot& Slot : TargetInventory)
	{
		for (AItem* Item : Slot.Items)
		{
			if (IsValid(Item)) { UniqueItems.Add(Item); }
		}
		Slot.ClearSlot();
	}
	TargetInventory.Reset();
	for (AItem* Item : UniqueItems)
	{
		if (IsValid(Item)) { Item->DeactivateItem(); }
	}
}

void UInventorySystemComponent::NotifyInventoryRestored()
{
	//RefreshLegacyWeaponInventoryView();
	SynchronizeQuickSlots();
	if (!IsValid(PlayerOwner)) { return; }
	PlayerOwner->OnInventoryUpdatedDelegate.Broadcast(ItemInventory);
	PlayerOwner->OnThrowableWeaponInventoryUpdatedDelegate.Broadcast(ThrowableWeaponInventory);
	NotifyWeaponInventoriesChanged();
	PlayerOwner->OnQuickSlotUpdatedDelegate.Broadcast(QuickSlotInventory, CurrentQuickSlotIndex);
	// Do not re-equip here: callers may still be restoring another inventory.
}


void UInventorySystemComponent::RestoreEquippedQuickSlot(int32 QuickSlotIndex)
{
	if (!IsQuickSlotSelectable(QuickSlotIndex))
	{
		CurrentQuickSlotIndex = INDEX_NONE;
	}
	else
	{
		SelectQuickSlot(QuickSlotIndex);
	}
}

// -------- Split firearm inventory helpers --------
TArray<FInventorySlot>* UInventorySystemComponent::GetWeaponInventoryForType(EQuickSlotType SlotType)
{
	if (SlotType == EQuickSlotType::PrimaryWeapon) { return &PrimaryWeaponInventory; }
	if (SlotType == EQuickSlotType::SecondaryWeapon) { return &SecondaryWeaponInventory; }
	return nullptr;
}

const TArray<FInventorySlot>* UInventorySystemComponent::GetWeaponInventoryForType(EQuickSlotType SlotType) const
{
	if (SlotType == EQuickSlotType::PrimaryWeapon) { return &PrimaryWeaponInventory; }
	if (SlotType == EQuickSlotType::SecondaryWeapon) { return &SecondaryWeaponInventory; }
	return nullptr;
}

void UInventorySystemComponent::NotifyWeaponInventoriesChanged()
{
	if (IsValid(PlayerOwner))
	{
		PlayerOwner->OnPrimaryWeaponInventoryUpdated.Broadcast(PrimaryWeaponInventory);
		PlayerOwner->OnSecondaryWeaponInventoryUpdated.Broadcast(SecondaryWeaponInventory);
	}
}

bool UInventorySystemComponent::FindOwnedWeapon(
	EItemName ItemName, AWeapon*& OutWeapon, EInventoryName& OutInventory, int32& OutIndex) const
{
	OutWeapon = nullptr;
	OutIndex = INDEX_NONE;
	const EInventoryName Types[] = { EInventoryName::PrimaryWeapon, EInventoryName::SecondaryWeapon };
	for (const EInventoryName Type : Types)
	{
		const TArray<FInventorySlot>* Inventory = GetInventoryConst(Type);
		if (!Inventory) { continue; }
		const int32 Index = FindItemSlot(*Inventory, ItemName);
		if (Inventory->IsValidIndex(Index))
		{
			AWeapon* Found = Cast<AWeapon>((*Inventory)[Index].Items[0].Get());
			if (IsValid(Found))
			{
				OutWeapon = Found;
				OutInventory = Type;
				OutIndex = Index;
				return true;
			}
		}
	}
	return false;
}

bool UInventorySystemComponent::ResolveInventoryName(FName Name, EInventoryName& OutType) const
{
	if (Name == TEXT("ItemInventory")) { OutType = EInventoryName::Item; return true; }
	if (Name == TEXT("PrimaryWeaponInventory")) { OutType = EInventoryName::PrimaryWeapon; return true; }
	if (Name == TEXT("SecondaryWeaponInventory")) { OutType = EInventoryName::SecondaryWeapon; return true; }
	if (Name == TEXT("ThrowableWeaponInventory")) { OutType = EInventoryName::ThrowableWeapon; return true; }
	return false;
}

void UInventorySystemComponent::RefreshCurrentWeaponIndex()
{
	CurrWeaponIdx = INDEX_NONE;
	if (!IsValid(CurrWeapon)) { return; }
	for (EInventoryName Type : { EInventoryName::PrimaryWeapon, EInventoryName::SecondaryWeapon })
	{
		int32 FoundIndex = INDEX_NONE;
		if (IsItemInSourceInventory(Type, CurrWeapon, FoundIndex))
		{
			CurrWeaponInventory = Type;
			CurrWeaponIdx = FoundIndex;
			return;
		}
	}
}

int32 UInventorySystemComponent::GetCurrentWeaponIndex() const
{
	if (CurrWeaponIdx == INDEX_NONE) { return INDEX_NONE; }
	return CurrWeaponInventory == EInventoryName::SecondaryWeapon
		? PrimaryWeaponInventory.Num() + CurrWeaponIdx : CurrWeaponIdx;
}

const TArray<FInventorySlot>* UInventorySystemComponent::GetInventoryConst(EInventoryName InventoryName) const
{
	switch (InventoryName)
	{
	case EInventoryName::Item:            return &ItemInventory;
	case EInventoryName::PrimaryWeapon:   return &PrimaryWeaponInventory;
	case EInventoryName::SecondaryWeapon: return &SecondaryWeaponInventory;
	case EInventoryName::ThrowableWeapon: return &ThrowableWeaponInventory;
	default:                             return nullptr;
	}
}


TArray<FInventorySlot>* UInventorySystemComponent::GetInventory(EInventoryName InventoryName)
{
	switch (InventoryName)
	{
	case EInventoryName::Item:            return &ItemInventory;
	case EInventoryName::PrimaryWeapon:   return &PrimaryWeaponInventory;
	case EInventoryName::SecondaryWeapon: return &SecondaryWeaponInventory;
	case EInventoryName::ThrowableWeapon: return &ThrowableWeaponInventory;
	default:                             return nullptr;
	}
}

void UInventorySystemComponent::InitInventory()
{
	ItemInventory.SetNum(FMath::Max(MaxItemSlotsCount, 0));
	PrimaryWeaponInventory.SetNum(FMath::Max(MaxPrimaryWeaponSlotsCount, 2));
	SecondaryWeaponInventory.SetNum(FMath::Max(MaxSecondaryWeaponSlotsCount, 1));
	ThrowableWeaponInventory.SetNum(FMath::Max(MaxThrowableWeaponSlotsCount, 0));

	for (FInventorySlot& Slot : ItemInventory) { Slot.ClearSlot(); }
	for (FInventorySlot& Slot : PrimaryWeaponInventory) { Slot.ClearSlot(); }
	for (FInventorySlot& Slot : SecondaryWeaponInventory) { Slot.ClearSlot(); }
	for (FInventorySlot& Slot : ThrowableWeaponInventory) { Slot.ClearSlot(); }

	CurrWeaponIdx = INDEX_NONE;
	CurrWeaponInventory = EInventoryName::PrimaryWeapon;
	InitQuickSlotInventory();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			InitInventoryUITimerHandle, this,
			&UInventorySystemComponent::InitInventoryUI, 1.0f, false);
	}
}

bool UInventorySystemComponent::AddItem(TArray<FInventorySlot>& TargetInventory, AItem* NewItem, bool bIsStackable, int32 AddCount)
{
	if (NewItem == nullptr) return false;

	if (NewItem->GetItemName() == EItemName::ItemName_None || AddCount <= 0)
	{
		return false;
	}

	if (bIsStackable)
	{
		for (FInventorySlot& Slot : TargetInventory)
		{
			if (Slot.CanStack(NewItem->GetItemName()))
			{
				Slot.AddItem(NewItem, AddCount);
				return true;
			}
		}
	}

	const int32 EmptySlotIndex = FindEmptySlot(TargetInventory);

	if (EmptySlotIndex == INDEX_NONE)
	{
		return false;
	}

	TargetInventory[EmptySlotIndex].AddItem(NewItem, AddCount);
	return true;
}

bool UInventorySystemComponent::AddItemFromPickUp(AItemPickUp* NewItemPickUp, int32 AddCount)
{
	if (NewItemPickUp == nullptr) return false;
	if (NewItemPickUp->GetItemName() == EItemName::ItemName_None || AddCount <= 0)
	{
		return false;
	}

	if (NewItemPickUp->IsStackable())
	{
		for (FInventorySlot& Slot : ItemInventory)
		{
			if (Slot.CanStack(NewItemPickUp->GetItemName()))
			{
				AItem* NewItem = Cast<AItem>(NewItemPickUp->SpawnItem(PlayerOwner));
				if (!NewItem) { return false; }
				Slot.AddItem(NewItem, AddCount);
				return true;
			}
		}
	}

	const int32 EmptySlotIndex = FindEmptySlot(ItemInventory);
	if (EmptySlotIndex == INDEX_NONE)
	{
		return false;
	}

	AItem* NewItem = Cast<AItem>(NewItemPickUp->SpawnItem(PlayerOwner));
	if (!NewItem) { return false; }

	bool bItemAdded = ItemInventory[EmptySlotIndex].AddItem(NewItem, AddCount);

	if (bItemAdded)
	{
		if (NewItem->GetItemName() == EItemName::ItemName_FlashLight)
		{
			if (CurrFlashLightItem == nullptr)
			{
				CurrFlashLightItem = Cast<AFlashlightItem>(NewItem);

				int32 QuickSlotIdx = FindEmptyQuickSlot(EQuickSlotType::Flashlight);
				RegisterQuickSlot(QuickSlotIdx, EInventoryName::Item, EmptySlotIndex);

				if (!CurrItem)
				{
					SelectQuickSlot(QuickSlotIdx);
				}
			}
		}
	}
	else
	{
		NewItem->DeactivateItem();
	}

	return bItemAdded;
}

bool UInventorySystemComponent::AddWeaponFromPickUp(AItemPickUp* NewItemPickUp, int32 AddCount)
{
	if (!IsValid(NewItemPickUp) || !IsValid(PlayerOwner) ||
		NewItemPickUp->GetItemName() == EItemName::ItemName_None || AddCount <= 0)
	{
		return false;
	}

	// Duplicate weapon pickups replenish ammo in either category; they do not
	// occupy an additional slot (matches the original implementation).
	AWeapon* ExistingWeapon = nullptr;
	EInventoryName ExistingInventory = EInventoryName::PrimaryWeapon;
	int32 ExistingIndex = INDEX_NONE;
	if (FindOwnedWeapon(NewItemPickUp->GetItemName(), ExistingWeapon, ExistingInventory, ExistingIndex))
	{
		return ExistingWeapon->AddAmmo(NewItemPickUp->GetAmmo());
	}

	// Classification comes from the actual spawned weapon (the source project
	// already uses AWeapon::GetQuickSlotType()).
	AWeapon* NewWeapon = Cast<AWeapon>(NewItemPickUp->SpawnItem(PlayerOwner));
	if (!IsValid(NewWeapon)) { return false; }

	const EQuickSlotType WeaponType = NewWeapon->GetQuickSlotType();
	TArray<FInventorySlot>* Target = GetWeaponInventoryForType(WeaponType);
	if (!Target)
	{
		UE_LOG(LogTemp, Warning, TEXT("Pickup rejected: weapon has neither Primary nor Secondary QuickSlotType"));
		NewWeapon->DeactivateItem();
		return false;
	}

	const int32 FreeIndex = FindEmptySlot(*Target);
	if (FreeIndex == INDEX_NONE)
	{
		NewWeapon->DeactivateItem();
		return false;
	}

	if (!(*Target)[FreeIndex].AddItem(NewWeapon, 1))
	{
		NewWeapon->DeactivateItem();
		return false;
	}

	const EInventoryName SourceName = (WeaponType == EQuickSlotType::PrimaryWeapon)
		? EInventoryName::PrimaryWeapon : EInventoryName::SecondaryWeapon;
	const int32 QuickIndex = FindEmptyQuickSlot(WeaponType);
	if (QuickIndex != INDEX_NONE && RegisterQuickSlot(QuickIndex, SourceName, FreeIndex))
	{
		if (!IsValid(CurrItem))
		{
			SelectQuickSlot(QuickIndex);
		}
	}

	// The pickup inventory transaction publishes the updated inventory snapshots.
	return true;
}


bool UInventorySystemComponent::AddThrowableWeaponFromPickUp(AItemPickUp* NewItemPickUp, int32 AddCount)
{
	if (NewItemPickUp == nullptr) return false;
	if (NewItemPickUp->GetItemName() == EItemName::ItemName_None || AddCount <= 0)
	{
		return false;
	}

	int32 WeaponIdx = FindItemSlot(ThrowableWeaponInventory, NewItemPickUp->GetItemName());
	if (WeaponIdx != INDEX_NONE)
	{
		if (!ThrowableWeaponInventory[WeaponIdx].IsEmpty())
		{
			AThrowableWeapon* Weapon = Cast<AThrowableWeapon>(ThrowableWeaponInventory[WeaponIdx].GetItem(0));
			if (Weapon)
			{
				Weapon->AddAmmo(NewItemPickUp->GetAmmo());
				ThrowableWeaponInventory[WeaponIdx].AddItem(Weapon, AddCount);
				return true;
			}
		}
	}

	if (NewItemPickUp->IsStackable())
	{
		for (FInventorySlot& Slot : ThrowableWeaponInventory)
		{
			if (Slot.CanStack(NewItemPickUp->GetItemName()))
			{
				AThrowableWeapon* NewWeapon = Cast<AThrowableWeapon>(NewItemPickUp->SpawnItem(PlayerOwner));
				if (!NewWeapon) { return false; }

				Slot.AddItem(NewWeapon, AddCount);
				return true;
			}
		}
	}

	const int32 EmptySlotIndex = FindEmptySlot(ThrowableWeaponInventory);
	if (EmptySlotIndex == INDEX_NONE)
	{
		return false;
	}

	AThrowableWeapon* NewWeapon = Cast<AThrowableWeapon>(NewItemPickUp->SpawnItem(PlayerOwner));
	if (!NewWeapon) { return false; }

	bool bWeaponAdded = ThrowableWeaponInventory[EmptySlotIndex].AddItem(NewWeapon, AddCount);

	int32 QuickSlotIdx = FindEmptyQuickSlot(EQuickSlotType::ThrowableWeapon);
	RegisterQuickSlot(QuickSlotIdx, EInventoryName::ThrowableWeapon, EmptySlotIndex);

	if (bWeaponAdded)
	{
		if (!CurrItem)
		{
			SelectQuickSlot(QuickSlotIdx);
		}
	}
	else
	{
		NewWeapon->DeactivateItem();
	}

	return bWeaponAdded;
}

bool UInventorySystemComponent::RemoveItem(TArray<FInventorySlot>& TargetInventory, EItemName ItemName, int32 RemoveCount)
{
	if (ItemName == EItemName::ItemName_None || RemoveCount <= 0)
	{
		return false;
	}

	const int32 SlotIndex = FindItemSlot(TargetInventory, ItemName);

	if (SlotIndex == INDEX_NONE)
	{
		return false;
	}

	return RemoveItemAtSlot(TargetInventory, SlotIndex, RemoveCount);
}

bool UInventorySystemComponent::RemoveItemAtSlot(TArray<FInventorySlot>& TargetInventory, int32 SlotIndex, int32 RemoveCount)
{
	// <Old>

	//if (!IsValidSlotIndex(TargetInventory, SlotIndex) || RemoveCount <= 0)
	//{
	//	return false;
	//}

	//FInventorySlot& Slot = TargetInventory[SlotIndex];

	//if (Slot.IsEmpty())
	//{
	//	return false;
	//}

	//return Slot.RemoveItem(RemoveCount) == nullptr ? false : true;

	//-------------------------------------------------

	// <New>

	if (!TargetInventory.IsValidIndex(SlotIndex) || RemoveCount <= 0)
	{
		return false;
	}

	FInventorySlot& Slot = TargetInventory[SlotIndex];

	if (Slot.Num() < RemoveCount)
	{
		return false;
	}

	for (int32 i = 0; i < RemoveCount; ++i)
	{
		AItem* RemovedItem = Slot.RemoveItem(INDEX_NONE);

		if (!RemovedItem)
		{
			return false;
		}

		// Actual consume/deactivation policy belongs to the caller.
	}

	SynchronizeQuickSlots();

	return true;

}

bool UInventorySystemComponent::SwapSlots(TArray<FInventorySlot>& TargetInventory, int32 FromIndex, int32 ToIndex)
{
	if (!IsValidSlotIndex(TargetInventory, FromIndex) || !IsValidSlotIndex(TargetInventory, ToIndex))
	{
		return false;
	}

	if (FromIndex == ToIndex)
	{
		return false;
	}

	TargetInventory.Swap(FromIndex, ToIndex);
	return true;
}

bool UInventorySystemComponent::IsValidSlotIndex(TArray<FInventorySlot>& TargetInventory, int32 SlotIndex) const
{
	return TargetInventory.IsValidIndex(SlotIndex);
}

int32 UInventorySystemComponent::FindItemSlot(const TArray<FInventorySlot>& TargetInventory, EItemName ItemName) const
{
	for (int32 i = 0; i < TargetInventory.Num(); ++i)
	{
		if (TargetInventory[i].IsSameItem(ItemName))
		{
			return i;
		}
	}
	return INDEX_NONE;
}

int32 UInventorySystemComponent::FindEmptySlot(const TArray<FInventorySlot>& TargetInventory) const
{
	for (int32 i = 0; i < TargetInventory.Num(); ++i)
	{
		if (TargetInventory[i].IsEmpty())
		{
			return i;
		}
	}
	return INDEX_NONE;
}

int32 UInventorySystemComponent::GetOccupiedSlotCount(const TArray<FInventorySlot>& TargetInventory) const
{
	int32 OccupiedCount = 0;
	for (const FInventorySlot& Slot : TargetInventory)
	{
		if (!Slot.IsEmpty())
		{
			++OccupiedCount;
		}
	}
	return OccupiedCount;
}

const TArray<FInventorySlot>& UInventorySystemComponent::GetInventorySlots() const
{
	return ItemInventory;
}

void UInventorySystemComponent::InitInventoryUI()
{
	if (!IsValid(PlayerOwner)) { return; }
	PlayerOwner->OnInventoryCreatedDelegate.Broadcast(ItemInventory.Num());
	PlayerOwner->OnInventoryUpdatedDelegate.Broadcast(ItemInventory);
	PlayerOwner->OnThrowableWeaponInventoryCreatedDelegate.Broadcast(ThrowableWeaponInventory.Num());
	PlayerOwner->OnThrowableWeaponInventoryUpdatedDelegate.Broadcast(ThrowableWeaponInventory);
	PlayerOwner->OnPrimaryWeaponInventoryCreatedDelegate.Broadcast(PrimaryWeaponInventory.Num());
	PlayerOwner->OnPrimaryWeaponInventoryUpdated.Broadcast(PrimaryWeaponInventory);
	PlayerOwner->OnSecondaryWeaponInventoryCreatedDelegate.Broadcast(SecondaryWeaponInventory.Num());
	PlayerOwner->OnSecondaryWeaponInventoryUpdated.Broadcast(SecondaryWeaponInventory);

	NotifyWeaponInventoriesChanged();
	PlayerOwner->OnQuickSlotUpdatedDelegate.Broadcast(QuickSlotInventory, CurrentQuickSlotIndex);
}


void UInventorySystemComponent::SwapItemInventorySlots(FName InventoryName, int32 FromIndex, int32 ToIndex)
{
	if (FromIndex == ToIndex)
	{
		return;
	}

	EInventoryName Type = EInventoryName::Item;
	if (!ResolveInventoryName(InventoryName, Type))
	{
		return;
	}

	int32 LocalFrom = FromIndex;
	int32 LocalTo = ToIndex;
	EInventoryName ResolvedType = Type;

	TArray<FInventorySlot>* Inventory = GetInventory(ResolvedType);
	if (!Inventory ||
		!Inventory->IsValidIndex(LocalFrom) ||
		!Inventory->IsValidIndex(LocalTo))
	{
		return;
	}

	// Capture bindings before swapping. The primary inventory's two slots
	// have corresponding QuickSlots (0 and 1). Keep their fixed slot metadata.
	struct FPrimaryBinding
	{
		TWeakObjectPtr<AItem> Item;
		int32 SourceIndex = INDEX_NONE;
	};

	FPrimaryBinding Before[2];

	bool bRemapPrimaryQuickSlots =
		ResolvedType == EInventoryName::PrimaryWeapon &&
		LocalFrom >= 0 && LocalFrom < 2 &&
		LocalTo >= 0 && LocalTo < 2 &&
		QuickSlotInventory.IsValidIndex(0) &&
		QuickSlotInventory.IsValidIndex(1) &&
		QuickSlotInventory[0].SlotType == EQuickSlotType::PrimaryWeapon &&
		QuickSlotInventory[1].SlotType == EQuickSlotType::PrimaryWeapon &&
		QuickSlotInventory[0].SourceInventory == EInventoryName::PrimaryWeapon &&
		QuickSlotInventory[1].SourceInventory == EInventoryName::PrimaryWeapon;

	if (bRemapPrimaryQuickSlots)
	{
		for (int32 QuickIndex = 0; QuickIndex < 2; ++QuickIndex)
		{
			AItem* RegisteredItem = QuickSlotInventory[QuickIndex].GetItem();
			if (!IsValid(RegisteredItem))
			{
				continue;
			}

			int32 SourceIndex = INDEX_NONE;
			if (!IsItemInSourceInventory(
				EInventoryName::PrimaryWeapon, RegisteredItem, SourceIndex) ||
				SourceIndex < 0 || SourceIndex >= 2)
			{
				// If a registration does not refer to either mirrored source
				// slot, leave all registrations in actor-following mode.
				bRemapPrimaryQuickSlots = false;
				break;
			}

			Before[QuickIndex].Item = RegisteredItem;
			Before[QuickIndex].SourceIndex = SourceIndex;
		}
	}

	const int32 PreviousSelectedIndex = CurrentQuickSlotIndex;
	TWeakObjectPtr<AItem> PreviouslySelectedItem;
	if (QuickSlotInventory.IsValidIndex(PreviousSelectedIndex))
	{
		PreviouslySelectedItem = QuickSlotInventory[PreviousSelectedIndex].GetItem();
	}

	if (!SwapSlots(*Inventory, LocalFrom, LocalTo))
	{
		return;
	}

	if (bRemapPrimaryQuickSlots)
	{
		// Do NOT call ClearQuickSlot() or RegisterQuickSlot() here: those
		// helpers alter selection and may broadcast intermediate UI states.
		// Clear() preserves SlotType and SourceInventory.
		QuickSlotInventory[0].Clear();
		QuickSlotInventory[1].Clear();

		for (const FPrimaryBinding& OldBinding : Before)
		{
			AItem* RegisteredItem = OldBinding.Item.Get();
			if (!IsValid(RegisteredItem))
			{
				continue;
			}

			const int32 NewSourceIndex =
				OldBinding.SourceIndex == LocalFrom ? LocalTo :
				OldBinding.SourceIndex == LocalTo ? LocalFrom :
				OldBinding.SourceIndex;

			// Primary source index 0 -> QuickSlot 0; index 1 -> QuickSlot 1.
			const int32 NewQuickIndex = NewSourceIndex;
			if (!QuickSlotInventory.IsValidIndex(NewQuickIndex) ||
				!IsQuickSlotCompatible(
					NewQuickIndex, EInventoryName::PrimaryWeapon, RegisteredItem))
			{
				continue;
			}

			FQuickSlot& Destination = QuickSlotInventory[NewQuickIndex];
			Destination.Item = RegisteredItem;
			Destination.SourceSlotIndex = NewSourceIndex;
			// Destination.SlotType / SourceInventory remain fixed.
		}

		// The highlight moves with the selected item, but neither Equip nor
		// Unequip is requested. Other categories' selections stay unchanged.
		if (PreviousSelectedIndex == 0 || PreviousSelectedIndex == 1)
		{
			CurrentQuickSlotIndex = INDEX_NONE;
			for (int32 QuickIndex = 0; QuickIndex < 2; ++QuickIndex)
			{
				if (PreviouslySelectedItem.IsValid() &&
					QuickSlotInventory[QuickIndex].GetItem() ==
					PreviouslySelectedItem.Get())
				{
					CurrentQuickSlotIndex = QuickIndex;
					break;
				}
			}
		}
	}

	// For Item/Throwable/Secondary (and non-mirrored primary slots), follow
	// the same actor and update its cached source index as before.
	SynchronizeQuickSlots();
	RefreshCurrentWeaponIndex();

	if (IsValid(CurrThrowableWeapon))
	{
		int32 FoundIndex = INDEX_NONE;
		if (IsItemInSourceInventory(EInventoryName::ThrowableWeapon, CurrThrowableWeapon, FoundIndex))
		{
			CurrThrowableWeaponIdx = FoundIndex;
		}
		else
		{
			CurrThrowableWeaponIdx = INDEX_NONE;
		}
	}

	if (!IsValid(PlayerOwner))
	{
		return;
	}

	if (ResolvedType == EInventoryName::Item)
	{
		PlayerOwner->OnInventoryUpdatedDelegate.Broadcast(ItemInventory);
	}
	else if (ResolvedType == EInventoryName::ThrowableWeapon)
	{
		PlayerOwner->OnThrowableWeaponInventoryUpdatedDelegate.Broadcast(ThrowableWeaponInventory);
	}
	else
	{
		NotifyWeaponInventoriesChanged();
	}

	PlayerOwner->OnQuickSlotUpdatedDelegate.Broadcast(QuickSlotInventory, CurrentQuickSlotIndex);
}


void UInventorySystemComponent::DropItemInventorySlot(FName InventoryName, int32 SlotIndex)
{
	if (!IsValid(PlayerOwner)) { return; }

	EInventoryName Type = EInventoryName::Item;
	if (!ResolveInventoryName(InventoryName, Type)) { return; }
	int32 LocalIndex = SlotIndex;

	TArray<FInventorySlot>* Inventory = GetInventory(Type);
	if (!Inventory || !Inventory->IsValidIndex(LocalIndex) ||
		(*Inventory)[LocalIndex].IsEmpty())
	{
		return;
	}

	// Firearms can be dropped only when the active firearm is not busy.
	if ((Type == EInventoryName::PrimaryWeapon || Type == EInventoryName::SecondaryWeapon) &&
		IsValid(CurrWeapon) && CurrItem == CurrWeapon &&
		CurrWeapon->GetCurrentState() != EWeaponStateType::WeaponStateType_Idle &&
		CurrWeapon->GetCurrentState() != EWeaponStateType::WeaponStateType_Unequipped)
	{
		return;
	}

	FInventorySlot& Slot = (*Inventory)[LocalIndex];
	AItem* FirstItem = Slot.GetItem(0);
	if (!IsValid(FirstItem)) { return; }

	const bool bDroppedSelected = (CurrItem == FirstItem);
	if (AWeapon* DroppedWeapon = Cast<AWeapon>(FirstItem))
	{
		if (DroppedWeapon == CurrWeapon)
		{
			if (bOffHandFlashlightActive) { DisableOffHandFlashlight(); }
			DroppedWeapon->UnequipWeapon_Legacy(PlayerOwner);
			CurrWeapon = nullptr;
			CurrWeaponIdx = INDEX_NONE;
		}
	}
	if (AFlashlightItem* DroppedLight = Cast<AFlashlightItem>(FirstItem))
	{
		if (DroppedLight == CurrFlashLightItem)
		{
			if (bOffHandFlashlightActive) { DisableOffHandFlashlight(); }
			if (DroppedLight->IsEquipped()) { DroppedLight->Unequip(PlayerOwner); }
			CurrFlashLightItem = nullptr;
			bIsUsingFlashLightItem = false;
		}
	}
	if (FirstItem == CurrThrowableWeapon)
	{
		CurrThrowableWeapon = nullptr;
		CurrThrowableWeaponIdx = INDEX_NONE;
	}
	if (bDroppedSelected) { CurrItem = nullptr; }
	if (NextItem == FirstItem) { NextItem = nullptr; }
	if (EquippedQuickSlotItem.Get() == FirstItem) { EquippedQuickSlotItem.Reset(); }

	// Drop all actors from the slot. A stack may contain multiple items.
	// Some throwable stacks in this project store the same actor pointer
	// multiple times; avoid returning one actor to the pool repeatedly.
	TSet<AItem*> UniqueItems;
	for (AItem* Item : Slot.Items)
	{
		if (IsValid(Item)) { UniqueItems.Add(Item); }
	}
	Slot.ClearSlot();
	SynchronizeQuickSlots();

	for (AItem* Item : UniqueItems)
	{
		if (IsValid(Item)) { Item->DeactivateItemAndGetItemPickUp(); }
	}

	if (Type == EInventoryName::Item)
	{
		PlayerOwner->OnInventoryUpdatedDelegate.Broadcast(ItemInventory);
	}
	else if (Type == EInventoryName::ThrowableWeapon)
	{
		PlayerOwner->OnThrowableWeaponInventoryUpdatedDelegate.Broadcast(ThrowableWeaponInventory);
	}
	else
	{
		NotifyWeaponInventoriesChanged();
	}
	PlayerOwner->OnQuickSlotUpdatedDelegate.Broadcast(QuickSlotInventory, CurrentQuickSlotIndex);
}


void UInventorySystemComponent::UseItemInventorySlot(FName InventoryName, int32 SlotIndex)
{
	UE_LOG(LogTemp, Error, TEXT("void UInventorySystemComponent::UseItemInventorySlot(FName InventoryName, int32 SlotIndex)"));

	if (!IsValid(PlayerOwner)) { return; }

	// 현재는 일반 아이템 인벤토리만 Use 처리
	if (InventoryName != FName(TEXT("ItemInventory")))
	{
		UE_LOG(LogTemp, Warning, TEXT("UseItemInventorySlot: Unsupported inventory: %s"), *InventoryName.ToString());
		return;
	}

	if (!ItemInventory.IsValidIndex(SlotIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("UseItemInventorySlot: Invalid slot index: %d"), SlotIndex);
		return;
	}

	FInventorySlot& TargetSlot = ItemInventory[SlotIndex];
	if (TargetSlot.IsEmpty()) { return; }
	AItem* ItemToUse = TargetSlot.GetItem(0);

	if (!IsValid(ItemToUse))
	{
		UE_LOG(LogTemp, Warning, TEXT("UseItemInventorySlot: Item is invalid. SlotIndex: %d"), SlotIndex);
		return;
	}

	// 사용할 수 없는 상황이면 수량을 차감하지 않음
	const bool bUseSucceeded = ItemToUse->UseItem(PlayerOwner);
	if (!bUseSucceeded)
	{
		UE_LOG(LogTemp, Error, TEXT("UseItemInventorySlot: Use Item Failed!!!"));

		return;
	}

	// 사용에 성공한 경우 한 개 제거
	if (!RemoveItemAtSlot(ItemInventory, SlotIndex, 1))
	{
		UE_LOG(LogTemp, Warning, TEXT("UseItemInventorySlot: Failed to remove item. SlotIndex: %d"), SlotIndex);
		return;
	}

	// 인벤토리 UI 갱신
	PlayerOwner->OnInventoryUpdatedDelegate.Broadcast(ItemInventory);

	// 필요하다면 ToolWidget을 닫는 Delegate도 Broadcast
	// PlayerOwner->OnItemToolWidgetCloseRequestedDelegate.Broadcast();
}

bool UInventorySystemComponent::TryStorePickup(AItemPickUp* Pickup)
{
	// Only inventory mutations belong here. Detection/input/world actor lifetime
	// are handled by UInteractionComponent.
	if (!IsValid(Pickup) || !IsValid(PlayerOwner) || Pickup->IsDead() ||
		Pickup->GetItemName() == EItemName::ItemName_None)
	{
		return false;
	}

	if (Pickup->IsMagazine())
	{
		return TryStoreMagazineAmmo(Pickup);
	}

	if (Pickup->IsWeapon())
	{
		if (!AddWeaponFromPickUp(Pickup)) { return false; }
		SynchronizeQuickSlots();
		NotifyWeaponInventoriesChanged();
	}
	else if (Pickup->IsThrowableWeapon())
	{
		if (!AddThrowableWeaponFromPickUp(Pickup)) { return false; }
		SynchronizeQuickSlots();
		PlayerOwner->OnThrowableWeaponInventoryUpdatedDelegate.Broadcast(ThrowableWeaponInventory);
	}
	else
	{
		if (!AddItemFromPickUp(Pickup)) { return false; }
		SynchronizeQuickSlots();
		PlayerOwner->OnInventoryUpdatedDelegate.Broadcast(ItemInventory);
	}

	PlayerOwner->OnQuickSlotUpdatedDelegate.Broadcast(QuickSlotInventory, CurrentQuickSlotIndex);
	return true;
}

bool UInventorySystemComponent::TryStoreMagazineAmmo(AItemPickUp* MagazinePickup)
{
	if (!IsValid(MagazinePickup) || MagazinePickup->IsDead() ||
		!MagazinePickup->IsMagazine())
	{
		return false;
	}

	AWeapon* Weapon = nullptr;
	EInventoryName FoundInventory = EInventoryName::PrimaryWeapon;
	int32 FoundIndex = INDEX_NONE;
	if (!FindOwnedWeapon(MagazinePickup->GetItemName(), Weapon, FoundInventory, FoundIndex))
	{
		return false;
	}

	if (!IsValid(Weapon) || !Weapon->AddAmmo(MagazinePickup->GetAmmo()))
	{
		return false;
	}

	NotifyWeaponInventoriesChanged();
	return true;
}

void UInventorySystemComponent::ZoomIn(bool bZoomIn)
{
	bIsZoomIn = bZoomIn;
}

bool UInventorySystemComponent::IsWeaponModifyingCamFov()
{
	if (CurrWeapon)
	{
		if (CurrWeapon->IsModifyingPlayerCamFov() || bIsZoomIn)
		{
			return true;
		}
	}
	return false;
}

#pragma region Aiming

void UInventorySystemComponent::SetRightHandToAimSocketOffset(FVector offset)
{
	RightHandToAimSocketOffset = offset;
}

FVector UInventorySystemComponent::CalculateScreenCenterWorldPositionAndDirection(FVector& OutWorldPosition, FVector& OutWorldDirection) const
{
	//APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (!PlayerController || !PlayerController->PlayerCameraManager)
	{
		return FVector::ZeroVector;
	}

	FVector2D ViewportSize = GEngine->GameViewport->Viewport->GetSizeXY();
	FVector2D ScreenCenter(ViewportSize.X / 2.0f, ViewportSize.Y / 2.0f);
	FVector WorldPosition, WorldDirection;
	if (PlayerController->DeprojectScreenPositionToWorld(ScreenCenter.X, ScreenCenter.Y, WorldPosition, WorldDirection))
	{
		OutWorldPosition = WorldPosition;
		OutWorldDirection = WorldDirection;
		return WorldPosition + (WorldDirection * 15.0f); // TODO: 
	}

	return FVector::ZeroVector;
}

FVector UInventorySystemComponent::CalculateTargetRightHandPosition()
{
	const FVector AimLocation = CalculateScreenCenterWorldPositionAndDirection(ScreenCenterWorldLocation, ScreenCenterWorldDirection);

	const FRotator MeshRotation = PlayerOwner->GetArmMesh()->GetComponentRotation();

	const FVector TargetRightHandLocation = AimLocation - MeshRotation.RotateVector(RightHandToAimSocketOffset);

	TargetRightHandWorldLocation = TargetRightHandLocation;

	return TargetRightHandLocation;
}

FTransform UInventorySystemComponent::GetWeaponAimSocketRelativeTransform()
{
	return FTransform();
}
#pragma endregion


#pragma region SwitchWeapon
bool UInventorySystemComponent::IsCurrentSkillWeaponTargeting()
{
	if (CurrWeapon && CurrWeapon->GetCurrentState() == EWeaponStateType::WeaponStateType_Waiting) { return true; }
	else { return false; }
}
EWeaponStateType UInventorySystemComponent::GetCurrWeaponStateType() const
{
	if (CurrWeapon) return CurrWeapon->GetCurrentState();
	else return EWeaponStateType::WeaponStateType_None;
}

void UInventorySystemComponent::SwitchToNextItem()
{
	if (!IsValid(NextItem) || !IsValid(PlayerOwner)) { return; }
	AItem* ItemToEquip = NextItem;
	NextItem = nullptr;

	if (AWeapon* Weapon = Cast<AWeapon>(ItemToEquip))
	{
		CurrWeapon = Weapon;
		CurrThrowableWeapon = nullptr;
		CurrItem = Weapon;
		RefreshCurrentWeaponIndex();
	}
	else if (AThrowableWeapon* ThrowableWeapon = Cast<AThrowableWeapon>(ItemToEquip))
	{
		CurrThrowableWeapon = ThrowableWeapon;
		CurrItem = ThrowableWeapon;
		int32 Index = INDEX_NONE;
		CurrThrowableWeaponIdx = IsItemInSourceInventory(
			EInventoryName::ThrowableWeapon, ThrowableWeapon, Index) ? Index : INDEX_NONE;
	}
	else if (AFlashlightItem* Flashlight = Cast<AFlashlightItem>(ItemToEquip))
	{
		CurrFlashLightItem = Flashlight;
		CurrItem = Flashlight;
		bIsUsingFlashLightItem = true;
	}
	else
	{
		CurrItem = ItemToEquip;
	}
	ItemToEquip->Equip(PlayerOwner);
	EquippedQuickSlotItem = ItemToEquip;
	PlayerOwner->OnQuickSlotUpdatedDelegate.Broadcast(QuickSlotInventory, CurrentQuickSlotIndex);
}

EThrowableWeaponStateType UInventorySystemComponent::GetCurrThrowableWeaponStateType() const
{
	if (CurrThrowableWeapon) return CurrThrowableWeapon->GetCurrentState();
	else return EThrowableWeaponStateType::Idle;
}
void UInventorySystemComponent::SwitchToPreviousThrowableWeapon()
{
	if (CurrThrowableWeapon->GetCurrentState() == EThrowableWeaponStateType::Switching
		|| CurrThrowableWeapon->GetCurrentState() == EThrowableWeaponStateType::Unequiped) {
		return;
	}
	if (ThrowableWeaponInventory.Num() > 1)
	{
		const int32 PrevIndex = CurrThrowableWeaponIdx;

		CurrThrowableWeaponIdx--;
		if (CurrThrowableWeaponIdx < 0)
		{
			CurrThrowableWeaponIdx = ThrowableWeaponInventory.Num() + CurrThrowableWeaponIdx;
		}
		ChangeThrowableWeapon(CurrThrowableWeaponIdx);

		//UE_LOG(LogTemp, Warning, TEXT("Broadcasting weapon switch: %d -> %d"), PrevIndex, CurrentThrowableWeaponIndex);
		//OnWeaponSwitched.Broadcast(PrevIndex, CurrentThrowableWeaponIndex);
	}
}
void UInventorySystemComponent::SwitchToNextThrowableWeapon()
{
	if (CurrThrowableWeapon->GetCurrentState() == EThrowableWeaponStateType::Switching
		|| CurrThrowableWeapon->GetCurrentState() == EThrowableWeaponStateType::Unequiped) {
		return;
	}
	if (ThrowableWeaponInventory.Num() > 1)
	{
		const int32 PrevIndex = CurrThrowableWeaponIdx;

		CurrThrowableWeaponIdx = (CurrThrowableWeaponIdx + 1) % ThrowableWeaponInventory.Num();
		ChangeThrowableWeapon(CurrThrowableWeaponIdx);

		//UE_LOG(LogTemp, Warning, TEXT("Broadcasting weapon switch: %d -> %d"), PrevIndex, CurrentThrowableWeaponIndex);
		OnWeaponSwitched.Broadcast(PrevIndex, CurrThrowableWeaponIdx);
	}
}
void UInventorySystemComponent::SwitchToIndexThrowableWeapon(int32 idx)
{
	//if (CurrThrowableWeapon->GetCurrentState()->GetWeaponStateType() == EWeaponStateType::WeaponStateType_Switching
	//	|| CurrThrowableWeapon->GetCurrentState()->GetWeaponStateType() == EWeaponStateType::WeaponStateType_Unequipped) {
	//	return;
	//}
	//if (ThrowableWeaponInventory.IsValidIndex(idx) && CurrThrowableWeaponIdx != idx)
	//{
	//	const int32 PrevIndex = CurrThrowableWeaponIdx;
	//	CurrThrowableWeaponIdx = idx;
	//	ChangeWeapon(CurrThrowableWeaponIdx);

	//	UE_LOG(LogTemp, Warning, TEXT("Broadcasting weapon switch: %d -> %d"), PrevIndex, CurrThrowableWeaponIdx);
	//	OnWeaponSwitched.Broadcast(PrevIndex, CurrThrowableWeaponIdx);
	//}
}
//void UInventorySystemComponent::SwitchToOtherThrowableWeapon()
//{
//	//ThrowableWeaponInventory[CurrThrowableWeaponIdx]->SwitchWeapon(PlayerOwner, true);
//	//CurrThrowableWeapon = ThrowableWeaponInventory[CurrThrowableWeaponIdx];
//}
void UInventorySystemComponent::ChangeThrowableWeapon(int32 WeaponIndex)
{
	if (ThrowableWeaponInventory.IsValidIndex(WeaponIndex))
	{
		if (IsValid(CurrThrowableWeapon))
		{
			//CurrThrowableWeapon->SwitchWeapon(PlayerOwner, false);
			CurrThrowableWeapon->Unequip(PlayerOwner);
		}
	}
}
void UInventorySystemComponent::EquipThrowableWeapon(int32 WeaponIdx)
{
	UE_LOG(LogTemp, Error, TEXT("void UInventorySystemComponent::EquipThrowableWeapon(int32 WeaponIdx)"));

	if (!ThrowableWeaponInventory.IsValidIndex(WeaponIdx)) return;
	if (ThrowableWeaponInventory[WeaponIdx].IsEmpty()) return;

	if (bOffHandFlashlightActive)
	{
		DisableOffHandFlashlight();
	}
	if (CurrFlashLightItem)
	{
		bIsUsingFlashLightItem = false;
	}

	//-------------------------------

	//NextItem = ThrowableWeaponInventory[WeaponIdx].GetItem(0);

	//if (CurrItem)
	//{
	//	CurrItem->Unequip(PlayerOwner);
	//}
	//else
	//{
	//	NextItem->Equip(PlayerOwner);
	//}

	//-------------------------------
	ClearQuickSlot(3);
	RegisterQuickSlot(3, EInventoryName::ThrowableWeapon, WeaponIdx);
	SelectQuickSlot(3);
}

void UInventorySystemComponent::UpdateThrowableWeaponInventorySlots()
{
	PlayerOwner->OnThrowableWeaponInventoryUpdatedDelegate.Broadcast(ThrowableWeaponInventory);
}
#pragma endregion

#pragma region Control
bool UInventorySystemComponent::TryTakeControl(AWeapon* NewWeapon)
{
	if (!CurrWeapon)
	{
		ControllingWeapon = NewWeapon;
		return true;
	}
	else
	{
		EWeaponStateType CurrWeaponStateType = CurrWeapon->GetCurrentState();
		if (CurrWeaponStateType == EWeaponStateType::WeaponStateType_Idle) //TODO: 다른 state일 때 스킬 어떻게 작동해야 하는지
		{
			CurrWeapon->SetState(EWeaponStateType::WeaponStateType_Waiting);
			ControllingWeapon = NewWeapon;
			return true;
		}
		else
		{
			return false;
		}
	}
}
void UInventorySystemComponent::ReleaseControl()
{
	if (!CurrWeapon)
	{
		ControllingWeapon = nullptr;
	}
	else
	{
		EWeaponStateType CurrWeaponStateType = CurrWeapon->GetCurrentState();
		if (CurrWeaponStateType == EWeaponStateType::WeaponStateType_Waiting)
		{
			CurrWeapon->SetState(EWeaponStateType::WeaponStateType_Idle);
			ControllingWeapon = CurrWeapon;
		}
		else
		{
		}
	}
}
#pragma endregion

#pragma region FlashLight
void UInventorySystemComponent::ToggleFlashLight()
{
	UE_LOG(LogTemp, Warning, TEXT("void UInventorySystemComponent::ToggleFlashLight()"));

	if (!CurrFlashLightItem)
	{
		if (!FindFlashLightInInventory())
		{
			return;
		}
	}

	if (bOffHandFlashlightActive)
	{
		DisableOffHandFlashlight();
		return;
	}

	if (IsValid(CurrWeapon) && CurrItem == CurrWeapon && !CurrWeapon->IsTwoHandedWeapon())
	{
		// Do not fall through to the legacy QuickSlot replacement path for pistols.
		// A reload or weapon switch is not a valid time to enter dual-wield stance.
		TryEnableOffHandFlashlight();
		return;
	}

	//------------------------

	//if (CurrFlashLightItem->IsFlashlightOn())
	//{
	//	if (CurrWeapon)
	//	{
	//		NextItem = CurrWeapon;
	//		CurrFlashLightItem->Unequip(PlayerOwner);
	//		bIsUsingFlashLightItem = false; //TODO: Item->Unequip 하면 자동으로 bIsUsingFlashLightItem = false가 되어야함
	//	}
	//	else
	//	{
	//		CurrFlashLightItem->Unequip(PlayerOwner);
	//		bIsUsingFlashLightItem = false;
	//	}
	//}
	//else
	//{
	//	if (CurrWeapon)
	//	{
	//		if (CurrWeapon->GetCurrentState() == EWeaponStateType::WeaponStateType_Switching
	//			|| CurrWeapon->GetCurrentState() == EWeaponStateType::WeaponStateType_Unequipped) {
	//			return;
	//		}

	//		NextItem = CurrFlashLightItem;

	//		if (CurrWeapon->IsTwoHandedWeapon())
	//		{
	//			CurrWeapon->Unequip(PlayerOwner);
	//		}
	//		else
	//		{

	//		}
	//	}
	//	else
	//	{
	//		CurrFlashLightItem->Equip(PlayerOwner);
	//		bIsUsingFlashLightItem = true;
	//	}
	//}

	//------------------------

	if (CurrFlashLightItem->IsEquipped())
	{
		if (CurrWeapon)
		{
			int32 WeaponQuickSlotIdx = -1;
			if (IsItemInQuickSlots(CurrWeapon, WeaponQuickSlotIdx))
			{
				SelectQuickSlot(WeaponQuickSlotIdx);
				bIsUsingFlashLightItem = false; //TODO: Item->Unequip 하면 자동으로 bIsUsingFlashLightItem = false가 되어야함
			}
		}
		else
		{
			//TODO: 아무것도 장착하지 않은 상태가 필요한가?
		}
	}
	else
	{
		if (SelectQuickSlot(4))
		{
			bIsUsingFlashLightItem = true;
		}
	}
}
bool UInventorySystemComponent::FindFlashLightInInventory()
{
	int32 idx = FindItemSlot(ItemInventory, EItemName::ItemName_FlashLight);
	if (idx != INDEX_NONE)
	{
		if (AFlashlightItem* flashlight = Cast<AFlashlightItem>(ItemInventory[idx].GetItem(0)))
		{
			CurrFlashLightItem = Cast<AFlashlightItem>(flashlight);

			int32 QuickSlotIdx = FindEmptyQuickSlot(EQuickSlotType::Flashlight);
			RegisterQuickSlot(QuickSlotIdx, EInventoryName::Item, idx);

			return true;
		}
	}
	return false;
}

bool UInventorySystemComponent::TryEnableOffHandFlashlight()
{
	if (!IsValid(PlayerOwner) || !IsValid(CurrWeapon) || !IsValid(CurrFlashLightItem))
	{
		return false;
	}

	if (CurrItem != CurrWeapon
		|| CurrWeapon->IsTwoHandedWeapon()
		|| CurrWeapon->GetCurrentState() != EWeaponStateType::WeaponStateType_Idle)
	{
		return false;
	}

	// Change the gun first; an animation notify can now release the left hand.
	if (!CurrWeapon->SetGripMode(EWeaponGripMode::OneHanded))
	{
		return false;
	}

	// The flashlight's own Equip() still handles its existing light/zoom/audio logic.
	// The off-hand attachment is enforced after its regular equip completes.
	bOffHandFlashlightActive = true;
	bIsUsingFlashLightItem = true;
	if (!CurrFlashLightItem->IsEquipped())
	{
		CurrFlashLightItem->Equip(PlayerOwner);
	}
	UpdateOffHandFlashlightAttachment();
	return true;
}

void UInventorySystemComponent::DisableOffHandFlashlight()
{
	if (!bOffHandFlashlightActive)
	{
		return;
	}

	bOffHandFlashlightActive = false;
	bIsUsingFlashLightItem = false;

	if (IsValid(CurrFlashLightItem) && CurrFlashLightItem->IsEquipped())
	{
		CurrFlashLightItem->Unequip(PlayerOwner);
	}

	if (IsValid(CurrWeapon))
	{
		CurrWeapon->SetGripMode(EWeaponGripMode::TwoHanded);
	}
}

void UInventorySystemComponent::UpdateOffHandFlashlightAttachment()
{
	if (!bOffHandFlashlightActive)
	{
		return;
	}

	if (!IsValid(PlayerOwner) || !IsValid(CurrWeapon) || CurrItem != CurrWeapon
		|| CurrWeapon->IsTwoHandedWeapon() || !IsValid(CurrFlashLightItem))
	{
		DisableOffHandFlashlight();
		return;
	}

	// Do not override the flashlight's pending equip animation.  Its equip may
	// be timer/notify-driven, so this check retries after the actor becomes equipped.
	if (!CurrFlashLightItem->IsEquipped())
	{
		return;
	}

	USkeletalMeshComponent* ArmMesh = PlayerOwner->GetArmMesh();
	USceneComponent* LightRoot = CurrFlashLightItem->GetRootComponent();
	if (!IsValid(ArmMesh) || !IsValid(LightRoot))
	{
		return;
	}

	if (LightRoot->GetAttachParent() != ArmMesh
		|| LightRoot->GetAttachSocketName() != OffHandFlashlightSocketName)
	{
		CurrFlashLightItem->AttachToComponent(
			ArmMesh,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			OffHandFlashlightSocketName);
	}

	LightRoot->SetRelativeTransform(OffHandFlashlightRelativeTransform);
}
#pragma endregion

void UInventorySystemComponent::SynchronizeQuickSlots()
{
	bool bChanged = false;
	const int32 BeforeSelection = CurrentQuickSlotIndex;

	for (int32 Index = 0; Index < QuickSlotInventory.Num(); ++Index)
	{
		FQuickSlot& Slot = QuickSlotInventory[Index];
		AItem* ReferencedItem = Slot.GetItem();
		int32 SourceIndex = INDEX_NONE;

		if (!IsValid(ReferencedItem) ||
			!IsQuickSlotCompatible(Index, Slot.SourceInventory, ReferencedItem) ||
			!IsItemInSourceInventory(Slot.SourceInventory, ReferencedItem, SourceIndex))
		{
			if (Slot.SourceSlotIndex != INDEX_NONE || IsValid(ReferencedItem))
			{
				Slot.Clear();
				bChanged = true;
			}
			if (CurrentQuickSlotIndex == Index && !IsValid(ReferencedItem))
			{
				CurrentQuickSlotIndex = INDEX_NONE;
			}
			else if (CurrentQuickSlotIndex == Index && SourceIndex == INDEX_NONE)
			{
				CurrentQuickSlotIndex = INDEX_NONE;
			}
			continue;
		}

		if (Slot.SourceSlotIndex != SourceIndex)
		{
			Slot.SourceSlotIndex = SourceIndex;
			bChanged = true;
		}
	}
	if (IsValid(PlayerOwner) && (bChanged || BeforeSelection != CurrentQuickSlotIndex))
	{
		PlayerOwner->OnQuickSlotUpdatedDelegate.Broadcast(QuickSlotInventory, CurrentQuickSlotIndex);
	}
}


void UInventorySystemComponent::InitQuickSlotInventory()
{
	QuickSlotInventory.SetNum(QuickSlotCount);
	QuickSlotInventory[0].SlotType = EQuickSlotType::PrimaryWeapon;
	QuickSlotInventory[0].SourceInventory = EInventoryName::PrimaryWeapon;
	QuickSlotInventory[1].SlotType = EQuickSlotType::PrimaryWeapon;
	QuickSlotInventory[1].SourceInventory = EInventoryName::PrimaryWeapon;
	QuickSlotInventory[2].SlotType = EQuickSlotType::SecondaryWeapon;
	QuickSlotInventory[2].SourceInventory = EInventoryName::SecondaryWeapon;
	QuickSlotInventory[3].SlotType = EQuickSlotType::ThrowableWeapon;
	QuickSlotInventory[3].SourceInventory = EInventoryName::ThrowableWeapon;
	QuickSlotInventory[4].SlotType = EQuickSlotType::Flashlight;
	QuickSlotInventory[4].SourceInventory = EInventoryName::Item;

	for (FQuickSlot& Slot : QuickSlotInventory) { Slot.Clear(); }
	CurrentQuickSlotIndex = INDEX_NONE;
	EquippedQuickSlotItem.Reset();
}


const TArray<FInventorySlot>* UInventorySystemComponent::GetQuickSlotSourceInventory(EInventoryName InventoryName) const
{
	return GetInventoryConst(InventoryName);
}

bool UInventorySystemComponent::IsItemInSourceInventory(EInventoryName InventoryName, const AItem* Item, int32& OutSlotIndex) const
{
	OutSlotIndex = INDEX_NONE;
	if (!IsValid(Item)) { return false; }
	const TArray<FInventorySlot>* Inventory = GetQuickSlotSourceInventory(InventoryName);
	if (!Inventory) { return false; }

	for (int32 SlotIndex = 0; SlotIndex < Inventory->Num(); ++SlotIndex)
	{
		const FInventorySlot& Slot = (*Inventory)[SlotIndex];

		for (const TObjectPtr<AItem>& StoredItem : Slot.Items)
		{
			if (StoredItem.Get() == Item)
			{
				OutSlotIndex = SlotIndex;
				return true;
			}
		}
	}

	return false;
}

bool UInventorySystemComponent::IsItemInQuickSlots(const AItem* Item, int32& OutSlotIndex) const
{
	OutSlotIndex = INDEX_NONE;
	if (!IsValid(Item)) { return false; }

	for (int32 SlotIndex = 0; SlotIndex < QuickSlotInventory.Num(); ++SlotIndex)
	{
		const FQuickSlot& Slot = QuickSlotInventory[SlotIndex];
		if (Slot.GetItem() == Item)
		{
			OutSlotIndex = SlotIndex;
			return true;
		}
	}
	return false;
}

bool UInventorySystemComponent::IsQuickSlotCompatible(
	int32 QuickSlotIndex, EInventoryName SourceInventory, AItem* Item) const
{
	if (!QuickSlotInventory.IsValidIndex(QuickSlotIndex) || !IsValid(Item))
	{
		return false;
	}

	switch (QuickSlotInventory[QuickSlotIndex].SlotType)
	{
	case EQuickSlotType::PrimaryWeapon:
	{
		AWeapon* Weapon = Cast<AWeapon>(Item);
		return SourceInventory == EInventoryName::PrimaryWeapon &&
			IsValid(Weapon) && Weapon->GetQuickSlotType() == EQuickSlotType::PrimaryWeapon;
	}
	case EQuickSlotType::SecondaryWeapon:
	{
		AWeapon* Weapon = Cast<AWeapon>(Item);
		return SourceInventory == EInventoryName::SecondaryWeapon &&
			IsValid(Weapon) && Weapon->GetQuickSlotType() == EQuickSlotType::SecondaryWeapon;
	}
	case EQuickSlotType::ThrowableWeapon:
		return SourceInventory == EInventoryName::ThrowableWeapon && Item->IsA(AThrowableWeapon::StaticClass());
	case EQuickSlotType::Flashlight:
		return SourceInventory == EInventoryName::Item && Item->IsA(AFlashlightItem::StaticClass());
	default:
		return false;
	}
}


bool UInventorySystemComponent::IsQuickSlotSelectable(int32 QuickSlotIndex) const
{
	if (!QuickSlotInventory.IsValidIndex(QuickSlotIndex))
	{
		UE_LOG(LogTemp, Error, TEXT("bool UInventorySystemComponent::IsQuickSlotSelectable(int32 QuickSlotIndex) const 1"));

		return false;
	}

	const FQuickSlot& Slot = QuickSlotInventory[QuickSlotIndex];

	int32 SourceIndex = INDEX_NONE;

	return IsItemInSourceInventory(Slot.SourceInventory, Slot.GetItem(), SourceIndex);
}

//TODO: 기존 Equip 요청 함수와 비교
bool UInventorySystemComponent::RequestQuickSlotEquip(AItem* Item)
{
	if (!IsValid(Item) || !IsValid(PlayerOwner)) { return false; }

	if (CurrItem == Item)
	{
		return true;
	}

	// Prevent changing weapon during an incompatible state.
	if (IsValid(CurrWeapon) && CurrItem == CurrWeapon)
	{
		const EWeaponStateType State = GetCurrWeaponStateType();

		if (State != EWeaponStateType::WeaponStateType_Idle)
		{
			return false;
		}
	}

	// Off-hand flashlight is an additional equipped item, not the selected quick slot.
	// Any switch away from the current pistol ends the dual-wield stance first.
	if (bOffHandFlashlightActive)
	{
		DisableOffHandFlashlight();
	}

	NextItem = Item;

	if (IsValid(CurrItem))
	{
		// Existing unequip flow calls SwitchToNextItem.
		CurrItem->Unequip(PlayerOwner);
	}
	else
	{
		SwitchToNextItem();
	}

	return true;
}

void UInventorySystemComponent::HandleQuickSlotScroll(const FInputActionValue& Value)
{
	const float ScrollValue = Value.Get<float>();
	if (ScrollValue > 0.0f)
	{
		SelectNextQuickSlot();
	}
	else if (ScrollValue < 0.0f)
	{
		SelectPreviousQuickSlot();
	}
}

AItem* UInventorySystemComponent::GetCurrentQuickSlotItem() const
{
	if (!QuickSlotInventory.IsValidIndex(CurrentQuickSlotIndex))
	{
		return nullptr;
	}

	const FQuickSlot& Slot = QuickSlotInventory[CurrentQuickSlotIndex];

	int32 FoundSlotIndex = INDEX_NONE;

	if (!IsItemInSourceInventory(
		Slot.SourceInventory,
		Slot.GetItem(),
		FoundSlotIndex))
	{
		return nullptr;
	}

	return Slot.GetItem();
}

int32 UInventorySystemComponent::FindEmptyQuickSlot(EQuickSlotType InQuickSlotType)
{
	for (int32 QuickSlotIdx = 0; QuickSlotIdx < QuickSlotInventory.Num(); QuickSlotIdx++)
	{
		if (QuickSlotInventory[QuickSlotIdx].SlotType == InQuickSlotType)
		{
			if (QuickSlotInventory[QuickSlotIdx].IsEmpty())
			{
				return QuickSlotIdx;
			}
		}
	}
	return -1;
}

bool UInventorySystemComponent::RegisterQuickSlot(int32 QuickSlotIndex, EInventoryName SourceInventory, int32 SourceSlotIndex)
{
	if (!QuickSlotInventory.IsValidIndex(QuickSlotIndex)) { return false; }
	const TArray<FInventorySlot>* Inventory = GetQuickSlotSourceInventory(SourceInventory);

	if (!Inventory || !Inventory->IsValidIndex(SourceSlotIndex)) { return false; }
	const FInventorySlot& SourceSlot = (*Inventory)[SourceSlotIndex];
	if (SourceSlot.IsEmpty()) { return false; }

	AItem* Item = SourceSlot.Items[0].Get();

	if (!IsQuickSlotCompatible(QuickSlotIndex, SourceInventory, Item)) { return false; }

	// Prevent one item instance being assigned to multiple quick slots.
	for (int32 Index = 0; Index < QuickSlotInventory.Num(); ++Index)
	{
		if (Index == QuickSlotIndex)
		{
			continue;
		}

		if (QuickSlotInventory[Index].GetItem() == Item)
		{
			ClearQuickSlot(Index);
		}
	}

	FQuickSlot& QuickSlot = QuickSlotInventory[QuickSlotIndex];

	QuickSlot.SourceInventory = SourceInventory;
	QuickSlot.SourceSlotIndex = SourceSlotIndex;
	QuickSlot.Item = Item;

	// Count display is derived by the UI from the owning FInventorySlot, not cached here.

	//OnQuickSlotInventoryChanged.Broadcast();

	PlayerOwner->OnQuickSlotUpdatedDelegate.Broadcast(QuickSlotInventory, CurrentQuickSlotIndex);

	return true;
}

bool UInventorySystemComponent::ClearQuickSlot(int32 QuickSlotIndex)
{
	if (!QuickSlotInventory.IsValidIndex(QuickSlotIndex)) { return false; }
	FQuickSlot& Slot = QuickSlotInventory[QuickSlotIndex];

	if (Slot.IsEmpty())
	{
		return false;
	}

	Slot.Clear();

	if (CurrentQuickSlotIndex == QuickSlotIndex)
	{
		const int32 PreviousIndex = CurrentQuickSlotIndex;
		CurrentQuickSlotIndex = INDEX_NONE;
		//OnQuickSlotSelectionChanged.Broadcast(PreviousIndex, INDEX_NONE);
	}

	//OnQuickSlotInventoryChanged.Broadcast();

	return true;
}

bool UInventorySystemComponent::SelectQuickSlot(int32 QuickSlotIndex)
{
	if (QuickSlotInventory.IsValidIndex(QuickSlotIndex)
		&& IsQuickSlotSelectable(QuickSlotIndex)
		&& QuickSlotInventory[QuickSlotIndex].GetItem() == CurrFlashLightItem
		&& IsValid(CurrWeapon)
		&& CurrItem == CurrWeapon
		&& !CurrWeapon->IsTwoHandedWeapon())
	{
		// Keep the selected weapon slot highlighted.  This is an off-hand toggle.
		const bool bWasActive = bOffHandFlashlightActive;
		ToggleFlashLight();
		return bWasActive || bOffHandFlashlightActive;
	}

	if (!IsQuickSlotSelectable(QuickSlotIndex))
	{
		UE_LOG(LogTemp, Error, TEXT("bool UInventorySystemComponent::SelectQuickSlot(int32 QuickSlotIndex) 1"));

		return false;
	}

	if (CurrentQuickSlotIndex == QuickSlotIndex)
	{
		UE_LOG(LogTemp, Error, TEXT("bool UInventorySystemComponent::SelectQuickSlot(int32 QuickSlotIndex) 2"));

		return true;
	}

	AItem* TargetItem = QuickSlotInventory[QuickSlotIndex].GetItem();

	if (!RequestQuickSlotEquip(TargetItem))
	{
		UE_LOG(LogTemp, Error, TEXT("bool UInventorySystemComponent::SelectQuickSlot(int32 QuickSlotIndex) 3"));

		return false;
	}

	const int32 PreviousIndex = CurrentQuickSlotIndex;

	CurrentQuickSlotIndex = QuickSlotIndex;

	//OnQuickSlotSelectionChanged.Broadcast(
	//	PreviousIndex,
	//	CurrentQuickSlotIndex
	//);

	PlayerOwner->OnQuickSlotUpdatedDelegate.Broadcast(QuickSlotInventory, CurrentQuickSlotIndex);

	UE_LOG(LogTemp, Error, TEXT("bool UInventorySystemComponent::SelectQuickSlot(int32 QuickSlotIndex) 4"));

	return true;
}

void UInventorySystemComponent::SwapQuickSlots(int32 FromIndex, int32 ToIndex)
{
	//TODO: 구현 고려


	return;
}

void UInventorySystemComponent::SelectNextQuickSlot()
{
	const int32 NumSlots = QuickSlotInventory.Num();
	if (NumSlots == 0) { return; }

	const int32 StartIndex = CurrentQuickSlotIndex == INDEX_NONE ? NumSlots - 1 : CurrentQuickSlotIndex;

	for (int32 Step = 1; Step <= NumSlots; ++Step)
	{
		const int32 NextIndex = (StartIndex + Step) % NumSlots;

		if (bSkipEmptyQuickSlots && !IsQuickSlotSelectable(NextIndex))
		{
			continue;
		}

		if (SelectQuickSlot(NextIndex))
		{
			return;
		}
	}
}

void UInventorySystemComponent::SelectPreviousQuickSlot()
{
	const int32 NumSlots = QuickSlotInventory.Num();
	if (NumSlots == 0) { return; }
	const int32 StartIndex = CurrentQuickSlotIndex == INDEX_NONE ? 0 : CurrentQuickSlotIndex;

	for (int32 Step = 1; Step <= NumSlots; ++Step)
	{
		const int32 PreviousIndex = (StartIndex - Step + NumSlots * 2) % NumSlots;

		if (bSkipEmptyQuickSlots && !IsQuickSlotSelectable(PreviousIndex))
		{
			continue;
		}

		if (SelectQuickSlot(PreviousIndex))
		{
			return;
		}
	}
}
