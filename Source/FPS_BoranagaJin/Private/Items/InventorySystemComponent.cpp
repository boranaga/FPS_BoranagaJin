// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/InventorySystemComponent.h"
#//include "Items/Item.h"
#include "Items/InventorySlot.h"
#include "Items/ItemPickUp.h"
#include "Items/Weapons/Weapon.h"
#include "Items/Weapons/ThrowableWeapon/ThrowableWeapon.h"
#include "Items/FlashlightItem.h"
#include "Characters/Player/CharacterPlayer.h"

#include "Items/Weapons/WeaponName.h"

#include "ObjectPoolSubsystem.h"

#include "UI/InteractionWidget.h"

#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

UInventorySystemComponent::UInventorySystemComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UInventorySystemComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializePlayerReference();
	LoadWSCData();

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
	SearchItems(); //TODO: 탐색 조건 넣기
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
				// Inereact With WeaponPickUp
				EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &UInventorySystemComponent::PickUpItem);

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

bool UInventorySystemComponent::RestoreInventoryFromSaveData(const TArray<FInventorySlotSaveData>& SaveData, EInventoryName inventoryname)
{
	TArray<FInventorySlot>* InventoryToRestore = GetInventory(inventoryname);
	if (!InventoryToRestore) { return false; }
	ClearInventoryForRestore(*InventoryToRestore);
	InventoryToRestore->SetNum(SaveData.Num());

	bool bAllItemsRestored = true;

	for (int32 SlotIndex = 0; SlotIndex < SaveData.Num(); ++SlotIndex)
	{
		const FInventorySlotSaveData& SlotSaveData = SaveData[SlotIndex];
		FInventorySlot& RuntimeSlot = (*InventoryToRestore)[SlotIndex];

		RuntimeSlot.ClearSlot();
		RuntimeSlot.MaxStackCount = FMath::Max(SlotSaveData.MaxStackCount, 1);

		for (const FItemInstanceSaveData& ItemSaveData : SlotSaveData.Items)
		{
			AItem* RestoredItem = CreateRuntimeItemFromSaveData(ItemSaveData);

			if (!IsValid(RestoredItem))
			{
				bAllItemsRestored = false;
				continue;
			}

			if (!RuntimeSlot.AddItem(RestoredItem))
			{
				RestoredItem->DeactivateItem();
				bAllItemsRestored = false;
			}
		}
	}

	NotifyInventoryRestored();

	UE_LOG(LogTemp, Warning, TEXT("bool UInventorySystemComponent::RestoreInventoryFromSaveData(const TArray<FInventorySlotSaveData>& SaveData, EInventoryName inventoryname)"));

	return bAllItemsRestored;
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
	if (!&TargetInventory) { return; }
	for (FInventorySlot& Slot : TargetInventory)
	{
		for (AItem* Item : Slot.Items)
		{
			if (!IsValid(Item)) { continue; }
			Item->DeactivateItem();
		}
		Slot.ClearSlot();
	}
	TargetInventory.Reset();
}

void UInventorySystemComponent::NotifyInventoryRestored()
{
	if (PlayerOwner)
	{
		PlayerOwner->OnInventoryUpdatedDelegate.Broadcast(ItemInventory);
		PlayerOwner->OnWeaponInventoryUpdatedDelegate.Broadcast(WeaponInventory);
		PlayerOwner->OnThrowableWeaponInventoryUpdatedDelegate.Broadcast(ThrowableWeaponInventory);
	}

	//if (CurrWeaponIdx != INDEX_NONE)
	//{
	//	SwitchToIndex(CurrWeaponIdx);
	//}

	if (CurrentQuickSlotIndex != INDEX_NONE)
	{
		SelectQuickSlot(CurrentQuickSlotIndex);
	}
}

void UInventorySystemComponent::RestoreEquippedWeapon(int32 WeaponIndex)
{
	if (!WeaponInventory.IsValidIndex(WeaponIndex))
	{
		CurrWeaponIdx = INDEX_NONE;
		return;
	}
	CurrWeaponIdx = WeaponIndex;
}

const TArray<FInventorySlot>* UInventorySystemComponent::GetInventoryConst(EInventoryName InventoryName) const
{
	switch (InventoryName)
	{
	case EInventoryName::Item:
		return &ItemInventory;

	case EInventoryName::Weapon:
		return &WeaponInventory;

	case EInventoryName::ThrowableWeapon:
		return &ThrowableWeaponInventory;

	default:
		return nullptr;
	}
}

TArray<FInventorySlot>* UInventorySystemComponent::GetInventory(EInventoryName InventoryName)
{
	switch (InventoryName)
	{
	case EInventoryName::Item:
		return &ItemInventory;

	case EInventoryName::Weapon:
		return &WeaponInventory;

	case EInventoryName::ThrowableWeapon:
		return &ThrowableWeaponInventory;

	default:
		return nullptr;
	}
}

void UInventorySystemComponent::InitInventory()
{
	ItemInventory.SetNum(MaxItemSlotsCount);
	for (FInventorySlot& Slot : ItemInventory)
	{
		Slot.ClearSlot();
	}

	WeaponInventory.SetNum(MaxWeaponSlotsCount);
	for (FInventorySlot& Slot : WeaponInventory)
	{
		Slot.ClearSlot();
	}

	ThrowableWeaponInventory.SetNum(MaxThrowableWeaponSlotsCount);
	for (FInventorySlot& Slot : ThrowableWeaponInventory)
	{
		Slot.ClearSlot();
	}

	GetWorld()->GetTimerManager().SetTimer(
		InitInventoryUITimerHandle,
		this,
		&UInventorySystemComponent::InitInventoryUI,
		1.0f,
		false
	);

	InitQuickSlotInventory();

	UE_LOG(LogTemp, Warning, TEXT("void UInventorySystemComponent::InitInventory()"));
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
	if (NewItemPickUp == nullptr) return false;
	if (NewItemPickUp->GetItemName() == EItemName::ItemName_None || AddCount <= 0)
	{
		return false;
	}

	//TODO: 이 옵션을 위한 bool 변수 처리가 있어야 Stackable과 다른 처리가 가능할 듯
	int32 WeaponIdx = FindItemSlot(WeaponInventory, NewItemPickUp->GetItemName());
	if (WeaponIdx != INDEX_NONE)
	{
		if (!WeaponInventory[WeaponIdx].IsEmpty())
		{
			AWeapon* Weapon = Cast<AWeapon>(WeaponInventory[WeaponIdx].GetItem(0));
			if (Weapon)
			{
				Weapon->AddAmmo(NewItemPickUp->GetAmmo());
				return true;
			}
		}
	}

	if (NewItemPickUp->IsStackable())
	{
		for (FInventorySlot& Slot : WeaponInventory)
		{
			if (Slot.CanStack(NewItemPickUp->GetItemName()))
			{
				AWeapon* NewWeapon = Cast<AWeapon>(NewItemPickUp->SpawnItem(PlayerOwner));
				if (!NewWeapon) { return false; }

				Slot.AddItem(NewWeapon, AddCount);
				return true;
			}
		}
	}

	const int32 EmptySlotIndex = FindEmptySlot(WeaponInventory);
	if (EmptySlotIndex == INDEX_NONE)
	{
		return false;
	}

	AWeapon* NewWeapon = Cast<AWeapon>(NewItemPickUp->SpawnItem(PlayerOwner));
	if (!NewWeapon) { return false; }

	bool bWeaponAdded = WeaponInventory[EmptySlotIndex].AddItem(NewWeapon, AddCount);


	EQuickSlotType WeaponQuickSlotType = NewWeapon->GetQuickSlotType();
	int32 QuickSlotIdx = -1;
	if (WeaponQuickSlotType == EQuickSlotType::PrimaryWeapon)
	{
		QuickSlotIdx = FindEmptyQuickSlot(EQuickSlotType::PrimaryWeapon);
	}
	else if (WeaponQuickSlotType == EQuickSlotType::SecondaryWeapon)
	{
		QuickSlotIdx = FindEmptyQuickSlot(EQuickSlotType::SecondaryWeapon);
	}

	//int32 QuickSlotIdx = FindEmptyQuickSlot(EQuickSlotType::PrimaryWeapon);
	RegisterQuickSlot(QuickSlotIdx, EInventoryName::Weapon, EmptySlotIndex);

	if (bWeaponAdded)
	{
		if (!CurrItem)
		{
			//CurrWeapon = NewWeapon;
			//CurrItem = CurrWeapon;
			//if (CurrFlashLightItem && bIsUsingFlashLightItem)
			//{
			//	return true;
			//}
			//CurrWeapon->Equip(PlayerOwner);

			SelectQuickSlot(QuickSlotIdx);

		}
		return true;
	}
	else
	{
		NewWeapon->DeactivateItem();
		return false;
	}
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
	PlayerOwner->OnInventoryCreatedDelegate.Broadcast(MaxItemSlotsCount);
	PlayerOwner->OnInventoryUpdatedDelegate.Broadcast(ItemInventory);

	PlayerOwner->OnWeaponInventoryCreatedDelegate.Broadcast(MaxWeaponSlotsCount);
	PlayerOwner->OnWeaponInventoryUpdatedDelegate.Broadcast(WeaponInventory);

	PlayerOwner->OnThrowableWeaponInventoryCreatedDelegate.Broadcast(MaxThrowableWeaponSlotsCount);
	PlayerOwner->OnThrowableWeaponInventoryUpdatedDelegate.Broadcast(ThrowableWeaponInventory);

	PlayerOwner->OnQuickSlotUpdatedDelegate.Broadcast(QuickSlotInventory, CurrentQuickSlotIndex);
}

void UInventorySystemComponent::SwapItemInventorySlots(FName InventoryName, int32 FromIndex, int32 ToIndex)
{
	////---------------------------------------------
	//// <Old>

	//if (InventoryName == FName("ItemInventory"))
	//{
	//	if (!SwapSlots(ItemInventory, FromIndex, ToIndex)) { return; }
	//	if (PlayerOwner)
	//	{
	//		PlayerOwner->OnInventoryUpdatedDelegate.Broadcast(ItemInventory);
	//	}
	//}
	//else if (InventoryName == FName("WeaponInventory"))
	//{
	//	if (!SwapSlots(WeaponInventory, FromIndex, ToIndex)) { return; }
	//	if (PlayerOwner)
	//	{
	//		PlayerOwner->OnWeaponInventoryUpdatedDelegate.Broadcast(WeaponInventory);
	//	}
	//}

	//---------------------------------------------
	// <New>

	TArray<FInventorySlot>* TargetInventory = nullptr;
	EInventoryName TargetInventoryName;

	if (InventoryName == FName(TEXT("ItemInventory")))
	{
		TargetInventory = &ItemInventory;
		TargetInventoryName = EInventoryName::Item;
	}
	else if (InventoryName == FName(TEXT("WeaponInventory")))
	{
		TargetInventory = &WeaponInventory;
		TargetInventoryName = EInventoryName::Weapon;
	}
	else if (InventoryName == FName(TEXT("ThrowableWeaponInventory")))
	{
		TargetInventory = &ThrowableWeaponInventory;
		TargetInventoryName = EInventoryName::ThrowableWeapon;
	}

	if (!TargetInventory)
	{
		return;
	}

	if (!SwapSlots(*TargetInventory, FromIndex, ToIndex))
	{
		return;
	}

	//----------------------------------------------------
	int32 ItemIdxInQuickSlot_A = -1;
	int32 ItemIdxInQuickSlot_B = -1;

	IsItemInQuickSlots((*TargetInventory)[FromIndex].GetItem(), ItemIdxInQuickSlot_A);
	IsItemInQuickSlots((*TargetInventory)[ToIndex].GetItem(), ItemIdxInQuickSlot_B);

	if (!QuickSlotInventory.IsValidIndex(ItemIdxInQuickSlot_A) && QuickSlotInventory.IsValidIndex(ItemIdxInQuickSlot_B))
	{
		ClearQuickSlot(ItemIdxInQuickSlot_B);
		RegisterQuickSlot(ItemIdxInQuickSlot_B, TargetInventoryName, FromIndex);

		if (CurrentQuickSlotIndex == ItemIdxInQuickSlot_B)
		{
			//CurrentQuickSlotIndex == INDEX_NONE;
			SelectQuickSlot(ItemIdxInQuickSlot_B);
		}

	}
	else if (QuickSlotInventory.IsValidIndex(ItemIdxInQuickSlot_A) && !QuickSlotInventory.IsValidIndex(ItemIdxInQuickSlot_B))
	{
		ClearQuickSlot(ItemIdxInQuickSlot_A);
		RegisterQuickSlot(ItemIdxInQuickSlot_A, TargetInventoryName, ToIndex);

		if (CurrentQuickSlotIndex == ItemIdxInQuickSlot_A)
		{
			//CurrentQuickSlotIndex == INDEX_NONE;
			SelectQuickSlot(ItemIdxInQuickSlot_A);
		}
	}
	else if (QuickSlotInventory.IsValidIndex(ItemIdxInQuickSlot_A) && QuickSlotInventory.IsValidIndex(ItemIdxInQuickSlot_B))
	{
		if (CurrentQuickSlotIndex == ItemIdxInQuickSlot_A)
		{
			CurrentQuickSlotIndex = ItemIdxInQuickSlot_B;
		}
		else if (CurrentQuickSlotIndex == ItemIdxInQuickSlot_B)
		{
			CurrentQuickSlotIndex = ItemIdxInQuickSlot_A;
		}

		QuickSlotInventory.Swap(ItemIdxInQuickSlot_A, ItemIdxInQuickSlot_B);

	}

	PlayerOwner->OnQuickSlotUpdatedDelegate.Broadcast(QuickSlotInventory, CurrentQuickSlotIndex);
	//----------------------------------------------------

	SynchronizeQuickSlots();

	if (IsValid(CurrWeapon))
	{
		int32 FoundIndex = INDEX_NONE;

		if (IsItemInSourceInventory(EInventoryName::Weapon, CurrWeapon, FoundIndex))
		{
			CurrWeaponIdx = FoundIndex;
		}
		else
		{
			CurrWeaponIdx = INDEX_NONE;
		}
	}

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

	if (!IsValid(PlayerOwner)) { return; }

	if (TargetInventory == &ItemInventory)
	{
		PlayerOwner->OnInventoryUpdatedDelegate.Broadcast(ItemInventory);
	}
	else if (TargetInventory == &WeaponInventory)
	{
		PlayerOwner->OnWeaponInventoryUpdatedDelegate.Broadcast(WeaponInventory);
	}
	else
	{
		PlayerOwner->OnThrowableWeaponInventoryUpdatedDelegate.Broadcast(ThrowableWeaponInventory);
	}
}

void UInventorySystemComponent::DropItemInventorySlot(FName InventoryName, int32 SlotIndex)
{
	if (InventoryName == FName("ItemInventory"))
	{
		if (!ItemInventory.IsValidIndex(SlotIndex)) { return; }
		if (ItemInventory[SlotIndex].IsEmpty()) { return; }

		//-----------------------------------------------------------
		// TODO: 나중에 월드에 Pickup Actor Spawn하려면 여기에서 처리
		// TODO: Object pooling으로 ItemPickUp 불러오기

		AItem* ItemToDrop = ItemInventory[SlotIndex].GetItem(0);
		ItemInventory[SlotIndex].ClearSlot();

		if (!IsValid(ItemToDrop)) { return; }

		if (ItemToDrop == CurrFlashLightItem && bOffHandFlashlightActive)
		{
			DisableOffHandFlashlight();
		}

		if (ItemToDrop->GetItemName() == EItemName::ItemName_FlashLight && ItemToDrop == CurrFlashLightItem)
		{
			//TODO: Inventory 내에 또 다른 Flash Light이 있다면 그 것을 장착해야함
			if (bIsUsingFlashLightItem)
			{

				int32 idx = FindItemSlot(ItemInventory, EItemName::ItemName_FlashLight);
				if (idx != INDEX_NONE)
				{
					if (AFlashlightItem* nextflashlight = Cast<AFlashlightItem>(ItemInventory[idx].GetItem(0)))
					{
						NextItem = nextflashlight;
						CurrFlashLightItem->Unequip(PlayerOwner);
						bIsUsingFlashLightItem = false;
						CurrFlashLightItem = nextflashlight;
					}
				}
				else
				{
					NextItem = CurrWeapon;
					CurrFlashLightItem->Unequip(PlayerOwner);
					bIsUsingFlashLightItem = false;
					CurrFlashLightItem = nullptr;
				}
			}
			else
			{

				CurrFlashLightItem = nullptr;
			}
		}



		//for (int32 i = 0; i < ItemInventory[SlotIndex].Count; i++)
		//{
		//	ItemInventory[SlotIndex].GetItem(i)->DeactivateItemPickUp_Pool();
		//}
		//-----------------------------------------------------------

		SynchronizeQuickSlots();

		ItemToDrop->DeactivateItemAndGetItemPickUp();

		if (PlayerOwner)
		{
			PlayerOwner->OnInventoryUpdatedDelegate.Broadcast(ItemInventory);
			PlayerOwner->OnQuickSlotUpdatedDelegate.Broadcast(QuickSlotInventory, CurrentQuickSlotIndex);
		}
	}
	else if (InventoryName == FName("WeaponInventory"))
	{
		if (!WeaponInventory.IsValidIndex(SlotIndex)) { return; }
		if (WeaponInventory[SlotIndex].IsEmpty()) { return; }

		if (CurrWeapon)
		{
			if (CurrWeapon->GetCurrentState() != EWeaponStateType::WeaponStateType_Idle
				&& CurrWeapon->GetCurrentState() != EWeaponStateType::WeaponStateType_Unequipped)
			{
				return;
			}
		}


		// TODO: GetItem(0)이 아니라 더 깔끔한 방식이 필요함
		AWeapon* Weapon = Cast<AWeapon>(WeaponInventory[SlotIndex].GetItem(0));
		if (Weapon == CurrWeapon && bOffHandFlashlightActive)
		{
			DisableOffHandFlashlight();
		}
		Weapon->UnequipWeapon_Legacy(PlayerOwner);
		if (Weapon == CurrWeapon)
		{
			CurrWeapon = nullptr;
			CurrItem = nullptr;
		}

		WeaponInventory[SlotIndex].GetItem(0)->DeactivateItemAndGetItemPickUp();

		WeaponInventory[SlotIndex].ClearSlot();

		SynchronizeQuickSlots();
		if (PlayerOwner)
		{
			PlayerOwner->OnWeaponInventoryUpdatedDelegate.Broadcast(WeaponInventory);
			PlayerOwner->OnQuickSlotUpdatedDelegate.Broadcast(QuickSlotInventory, CurrentQuickSlotIndex);
			//TODO: 여기서 무기 버릴 때 처리를 해야함
		}
	}
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

bool UInventorySystemComponent::SearchItems()
{
	TArray<TEnumAsByte<EObjectTypeQuery>> traceObjectTypes;
	traceObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Visibility)); //TODO: Customize Collision Channel
	TArray<AActor*> ignoreActors;
	ignoreActors.Init(PlayerOwner, 1);
	FVector sphereSpwanLocation = PlayerOwner->GetActorLocation();
	TArray<AActor*> overlappedActors;
	bool bIsItemInViewPort = UKismetSystemLibrary::SphereOverlapActors(GetWorld(), sphereSpwanLocation, SearchItemRadius, traceObjectTypes, nullptr, ignoreActors, overlappedActors);

	float MinDistanceToItem = SearchItemRadius;
	AItemPickUp* NearestItem = nullptr;

	for (AActor* overlappedActor : overlappedActors)
	{
		AItemPickUp* ItemObject = Cast<AItemPickUp>(overlappedActor);
		if (ItemObject != nullptr && !ItemObject->IsDead())
		{
			if (IsInViewport(GetScreenPositionOfWorldLocation(ItemObject->GetActorLocation()).Get<0>(), SearchItemViewportRatio_Width, SearchItemViewportRatio_Height))
			{
				float DistanceToItem = PlayerOwner->GetDistanceTo(ItemObject);
				if (DistanceToItem < MinDistanceToItem)
				{
					MinDistanceToItem = DistanceToItem;
					NearestItem = ItemObject;
				}
			}
		}
	}

	if (OverlappedItem == nullptr && NearestItem != nullptr)
	{
		PlayerOwner->OnInteractionUIPopUpDelegate.Broadcast();
	}

	OverlappedItem = NearestItem;

	if (OverlappedItem != nullptr)
	{
		PlayerOwner->OnInteractionUIUpdatedDelegate.Broadcast(true, OverlappedItem->GetActorLocation());
	}
	else
	{
		PlayerOwner->OnInteractionUIUpdatedDelegate.Broadcast(false, FVector::ZeroVector);
	}
	return bIsItemInViewPort;
}

void UInventorySystemComponent::PickUpItem()
{
	if (OverlappedItem != nullptr)
	{
		if (CurrWeapon)
		{
			if (CurrWeapon->GetCurrentState() != EWeaponStateType::WeaponStateType_Unequipped
				&& CurrWeapon->GetCurrentState() != EWeaponStateType::WeaponStateType_Idle)
			{
				return;
			}
		}

		if (OverlappedItem->IsMagazine())
		{
			ObtainAmmo(OverlappedItem);
		}
		else
		{
			//ObtainItem(OverlappedItem);
			ObtainItem(OverlappedItem);
		}
	}
}

bool UInventorySystemComponent::ObtainItem(AItemPickUp* NewItemPickUp)
{
	if (!NewItemPickUp || !PlayerOwner) { return false; }
	if (NewItemPickUp->GetItemName() == EItemName::ItemName_None) { return false; }

	if (NewItemPickUp->IsWeapon())
	{
		if (AddWeaponFromPickUp(NewItemPickUp))
		{
			NewItemPickUp->DeactivateItemPickUp();
			SynchronizeQuickSlots();
			PlayerOwner->OnWeaponInventoryUpdatedDelegate.Broadcast(WeaponInventory);
			PlayerOwner->OnQuickSlotUpdatedDelegate.Broadcast(QuickSlotInventory, CurrentQuickSlotIndex);
			return true;
		}
		else
		{
			return false;
		}
	}
	else if (NewItemPickUp->IsThrowableWeapon())
	{
		if (AddThrowableWeaponFromPickUp(NewItemPickUp))
		{
			NewItemPickUp->DeactivateItemPickUp();
			SynchronizeQuickSlots();
			PlayerOwner->OnThrowableWeaponInventoryUpdatedDelegate.Broadcast(ThrowableWeaponInventory);
			PlayerOwner->OnQuickSlotUpdatedDelegate.Broadcast(QuickSlotInventory, CurrentQuickSlotIndex);

			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		if (AddItemFromPickUp(NewItemPickUp))
		{
			NewItemPickUp->DeactivateItemPickUp();
			SynchronizeQuickSlots();
			PlayerOwner->OnInventoryUpdatedDelegate.Broadcast(ItemInventory);
			PlayerOwner->OnQuickSlotUpdatedDelegate.Broadcast(QuickSlotInventory, CurrentQuickSlotIndex);

			return true;
		}
		else
		{
			return false;
		}
	}
}

void UInventorySystemComponent::LoadWSCData()
{
	//DTWSC = WSCDataTableHandle.GetRow<FWeaponSystemComponentData>("");
	//if (DTWSC)
	//{
	//	bUseSceneCapture = DTWSC->bUseSceneCapture;
	//}

	//if (bUseSceneCapture)
	//{
	//	if (!PlayerOwner) { return; }
	//	USceneCaptureComponent2D* FPSceneCapture = PlayerOwner->GetSceneCaptureComponent();
	//	if (!FPSceneCapture) { return; }
	//	FPSceneCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	//	FPSceneCapture->ShowOnlyComponent(PlayerOwner->GetArmMesh());
	//	FPSceneCapture->ShowOnlyComponent(PlayerOwner->GetHandsMesh());

	//	PlayerOwner->GetArmMesh()->SetVisibleInSceneCaptureOnly(true);
	//	PlayerOwner->GetHandsMesh()->SetVisibleInSceneCaptureOnly(true);

	//	if (FPHUD) { FPHUD->AddToViewport(); }
	//}
}

void UInventorySystemComponent::SaveInventory()
{
	// MEMO: 급해서 일단 비활성화
	//UCheckpointSubsystem* CheckpointSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UCheckpointSubsystem>();
	//check(CheckpointSubsystem);
	//FName CurrentMapName = FName(*UGameplayStatics::GetCurrentLevelName(this, true));
	//CheckpointSubsystem->SaveCheckpoint(CurrentMapName, PlayerOwner->GetActorTransform(), -1);
	////GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, TEXT("Saved Checkpoint"));

	//------------------------------------
}


TTuple<FVector2D, bool> UInventorySystemComponent::GetScreenPositionOfWorldLocation(const FVector& SearchLocation) const
{
	FVector2D ScreenLocation = FVector2D::ZeroVector;
	bool bResult = UGameplayStatics::ProjectWorldToScreen(PlayerController, SearchLocation, ScreenLocation);

	return MakeTuple(ScreenLocation, bResult);
}

bool UInventorySystemComponent::IsInViewport(FVector2D ActorScreenPosition, float ScreenRatio_Width, float ScreenRatio_Height) const
{
	FVector2D ViewportSize = GEngine->GameViewport->Viewport->GetSizeXY();

	bool bIsInWidth = true;
	bool bIsInHeight = true;

	// Check Width
	if (ScreenRatio_Width == 0.0f || UKismetMathLibrary::Abs(ScreenRatio_Width) > 1.0f || (ScreenRatio_Width == (1.0f - ScreenRatio_Width)))
	{
		if (ActorScreenPosition.X >= 0.0f && ActorScreenPosition.X <= ViewportSize.X)
		{
			bIsInWidth = true;
		}
		else
		{
			bIsInWidth = false;
		}
	}
	else
	{
		float LargeScreenRatio_Width;
		float SmallScreenRatio_Width;

		if (ScreenRatio_Width < (1.0f - ScreenRatio_Width))
		{
			LargeScreenRatio_Width = 1.0f - ScreenRatio_Width;
			SmallScreenRatio_Width = ScreenRatio_Width;
		}
		else
		{
			LargeScreenRatio_Width = ScreenRatio_Width;
			SmallScreenRatio_Width = 1.0f - ScreenRatio_Width;
		}

		if (ActorScreenPosition.X >= ViewportSize.X * SmallScreenRatio_Width && ActorScreenPosition.X <= ViewportSize.X * LargeScreenRatio_Width)
		{
			bIsInWidth = true;
		}
		else
		{
			bIsInWidth = false;
		}
	}

	// Check Height
	if (ScreenRatio_Height == 0.0f || UKismetMathLibrary::Abs(ScreenRatio_Height) > 1.0f || (ScreenRatio_Height == (1.0f - ScreenRatio_Height)))
	{
		if (ActorScreenPosition.Y >= 0.0f && ActorScreenPosition.Y <= ViewportSize.Y)
		{
			bIsInHeight = true;
		}
		else
		{
			bIsInHeight = false;
		}
	}
	else
	{
		float LargeScreenRatio_Height;
		float SmallScreenRatio_Height;

		if (ScreenRatio_Height < (1.0f - ScreenRatio_Height))
		{
			LargeScreenRatio_Height = 1.0f - ScreenRatio_Height;
			SmallScreenRatio_Height = ScreenRatio_Height;
		}
		else
		{
			LargeScreenRatio_Height = ScreenRatio_Height;
			SmallScreenRatio_Height = 1.0f - ScreenRatio_Height;
		}

		if (ActorScreenPosition.Y >= ViewportSize.Y * SmallScreenRatio_Height && ActorScreenPosition.Y <= ViewportSize.Y * LargeScreenRatio_Height)
		{
			bIsInHeight = true;
		}
		else
		{
			bIsInHeight = false;
		}
	}

	// Return
	if (bIsInWidth && bIsInHeight)
	{
		return true;
	}
	else
	{
		return false;
	}
}


bool UInventorySystemComponent::ObtainAmmo(AItemPickUp* MagazinePickUp)
{
	//UE_LOG(LogTemp, Error, TEXT("UInventorySystemComponent::ObtainAmmo(AItemPickUp* MagazinePickUp)"));

	int32 WeaponIdx = FindItemSlot(WeaponInventory, MagazinePickUp->GetItemName());
	if (WeaponIdx != INDEX_NONE)
	{
		if (!WeaponInventory[WeaponIdx].IsEmpty())
		{
			AWeapon* Weapon = Cast<AWeapon>(WeaponInventory[WeaponIdx].GetItem(0));
			if (Weapon)
			{
				if (Weapon->AddAmmo(MagazinePickUp->GetAmmo()))
				{
					//MagazinePickUp->DestroyItemPickUp();
					MagazinePickUp->DeactivateItemPickUp();
					return true;
				}
			}
		}
	}

	return false;
}

void UInventorySystemComponent::ZoomIn(bool bZoomIn)
{
	bIsZoomIn = bZoomIn;

	//UE_LOG(LogTemp, Warning, TEXT("ZOOM Toggled!!!"));
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

//void UInventorySystemComponent::SwitchToPreviousWeapon()
//{
//	if (!CurrWeapon) { return; }
//	if (CurrWeapon->GetCurrentState()->GetWeaponStateType() == EWeaponStateType::WeaponStateType_Switching
//		|| CurrWeapon->GetCurrentState()->GetWeaponStateType() == EWeaponStateType::WeaponStateType_Unequipped) {
//		return;
//	}
//
//	int32 NextIdx = CurrWeaponIdx;
//
//	for (int32 i = 1; i < WeaponInventory.Num(); i++)
//	{
//		NextIdx -= 1;
//		if (NextIdx < 0)
//		{
//			NextIdx = WeaponInventory.Num() + NextIdx;
//		}
//
//		if (!WeaponInventory[NextIdx].IsEmpty())
//		{
//			CurrWeaponIdx = NextIdx;
//			ChangeWeapon(CurrWeaponIdx);
//			break;
//		}
//	}
//}

//void UInventorySystemComponent::SwitchToNextWeapon()
//{
//	if (!CurrWeapon) { return; }
//	if (CurrWeapon->GetCurrentState()->GetWeaponStateType() == EWeaponStateType::WeaponStateType_Switching
//		|| CurrWeapon->GetCurrentState()->GetWeaponStateType() == EWeaponStateType::WeaponStateType_Unequipped) {
//		return;
//	}
//
//	int32 NextIdx = CurrWeaponIdx;
//
//	for (int32 i = 1; i < WeaponInventory.Num(); i++)
//	{
//		NextIdx += 1;
//		NextIdx = NextIdx % WeaponInventory.Num();
//
//		if (!WeaponInventory[NextIdx].IsEmpty())
//		{
//			CurrWeaponIdx = NextIdx;
//			ChangeWeapon(CurrWeaponIdx);
//			break;
//		}
//	}
//}

//void UInventorySystemComponent::SwitchToIndex(int32 idx)
//{
//	if (CurrWeapon)
//	{
//		if (CurrWeapon->GetCurrentState()->GetWeaponStateType() == EWeaponStateType::WeaponStateType_Switching
//			|| CurrWeapon->GetCurrentState()->GetWeaponStateType() == EWeaponStateType::WeaponStateType_Unequipped) {
//			return;
//		}
//	}
//	if (WeaponInventory.IsValidIndex(idx) && CurrWeaponIdx != idx)
//	{
//		const int32 PrevIndex = CurrWeaponIdx;
//		CurrWeaponIdx = idx;
//		ChangeWeapon(CurrWeaponIdx);
//
//		//UE_LOG(LogTemp, Warning, TEXT("Broadcasting weapon switch: %d -> %d"), PrevIndex, CurrWeaponIdx);
//		OnWeaponSwitched.Broadcast(PrevIndex, CurrWeaponIdx);
//	}
//}

void UInventorySystemComponent::SwitchToNextItem()
{
	//if (!NextItem) { return; }
	//if (NextItem->IsWeapon())
	//{
	//	CurrWeapon = Cast<AWeapon>(NextItem);
	//	CurrItem = CurrWeapon;
	//}
	//else
	//{
	//	bIsUsingFlashLightItem = true; //TODO: 이 변수에 대한 처리가 필요함
	//}

	//NextItem->Equip(PlayerOwner);

	//----------------------------------------

	if (!IsValid(NextItem) || !IsValid(PlayerOwner))
	{
		return;
	}

	AItem* ItemToEquip = NextItem;
	NextItem = nullptr;

	if (AWeapon* Weapon = Cast<AWeapon>(ItemToEquip))
	{
		CurrWeapon = Weapon;
		CurrThrowableWeapon = nullptr;
		CurrItem = Weapon;

		int32 WeaponSlotIndex = INDEX_NONE;

		if (IsItemInSourceInventory(
			EInventoryName::Weapon,
			Weapon,
			WeaponSlotIndex))
		{
			CurrWeaponIdx = WeaponSlotIndex;
		}
	}
	else if (AThrowableWeapon* ThrowableWeapon =
		Cast<AThrowableWeapon>(ItemToEquip))
	{
		CurrThrowableWeapon = ThrowableWeapon;
		CurrItem = ThrowableWeapon;

		int32 ThrowableSlotIndex = INDEX_NONE;

		if (IsItemInSourceInventory(
			EInventoryName::ThrowableWeapon,
			ThrowableWeapon,
			ThrowableSlotIndex))
		{
			CurrThrowableWeaponIdx = ThrowableSlotIndex;
		}

		// CurrWeapon remains available as the last selected firearm.
		// Its equipped state must be treated separately.
	}
	else if (AFlashlightItem* Flashlight =
		Cast<AFlashlightItem>(ItemToEquip))
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

	//SynchronizeQuickSlots();
	PlayerOwner->OnQuickSlotUpdatedDelegate.Broadcast(QuickSlotInventory, CurrentQuickSlotIndex);

}

//void UInventorySystemComponent::ChangeWeapon(int32 WeaponIndex)
//{
//	if (WeaponInventory.IsValidIndex(WeaponIndex) && !WeaponInventory[WeaponIndex].IsEmpty())
//	{
//		NextItem = WeaponInventory[WeaponIndex].GetItem(0);
//
//		if (IsValid(CurrWeapon))
//		{
//			//CurrWeapon->SwitchWeapon(PlayerOwner, false);
//			CurrWeapon->Unequip(PlayerOwner);
//		}
//		else
//		{
//			SwitchToNextItem();
//		}
//	}
//}


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
	bool bInventoryChanged = false;
	bool bSelectionChanged = false;

	const int32 PreviousSelection = CurrentQuickSlotIndex;

	for (int32 QuickIndex = 0; QuickIndex < QuickSlotInventory.Num(); ++QuickIndex)
	{
		FQuickSlot& QuickSlot = QuickSlotInventory[QuickIndex];

		if (QuickSlot.IsEmpty())
		{
			if (QuickSlot.SourceSlotIndex != INDEX_NONE)
			{
				QuickSlot.Clear();
				bInventoryChanged = true;
			}

			continue;
		}

		AItem* ReferencedItem = QuickSlot.GetItem();
		int32 FoundSourceIndex = INDEX_NONE;

		const bool bStillOwned = IsItemInSourceInventory(
			QuickSlot.SourceInventory,
			ReferencedItem,
			FoundSourceIndex
		);

		if (!bStillOwned)
		{
			QuickSlot.Clear();
			bInventoryChanged = true;

			if (CurrentQuickSlotIndex == QuickIndex)
			{
				CurrentQuickSlotIndex = INDEX_NONE;
				bSelectionChanged = true;
			}

			continue;
		}

		if (QuickSlot.SourceSlotIndex != FoundSourceIndex)
		{
			QuickSlot.SourceSlotIndex = FoundSourceIndex;
			bInventoryChanged = true;
		}
	}

	if (bInventoryChanged)
	{
		//OnQuickSlotInventoryChanged.Broadcast();
	}

	if (bSelectionChanged)
	{
		//OnQuickSlotSelectionChanged.Broadcast(
		//	PreviousSelection,
		//	CurrentQuickSlotIndex
		//);
	}
}

void UInventorySystemComponent::InitQuickSlotInventory()
{
	QuickSlotInventory.SetNum(QuickSlotCount);

	QuickSlotInventory[0].SlotType = EQuickSlotType::PrimaryWeapon;
	QuickSlotInventory[0].SourceInventory = EInventoryName::Weapon;

	QuickSlotInventory[1].SlotType = EQuickSlotType::PrimaryWeapon;
	QuickSlotInventory[1].SourceInventory = EInventoryName::Weapon;

	QuickSlotInventory[2].SlotType = EQuickSlotType::SecondaryWeapon;
	QuickSlotInventory[2].SourceInventory = EInventoryName::Weapon;

	QuickSlotInventory[3].SlotType = EQuickSlotType::ThrowableWeapon;
	QuickSlotInventory[3].SourceInventory = EInventoryName::ThrowableWeapon;

	QuickSlotInventory[4].SlotType = EQuickSlotType::Flashlight;
	QuickSlotInventory[4].SourceInventory = EInventoryName::Item;

	for (FQuickSlot& Slot : QuickSlotInventory)
	{
		Slot.Clear();
	}

	CurrentQuickSlotIndex = INDEX_NONE;
	EquippedQuickSlotItem.Reset();

	//OnQuickSlotInventoryChanged.Broadcast();
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

bool UInventorySystemComponent::IsQuickSlotCompatible(int32 QuickSlotIndex, EInventoryName SourceInventory, const AItem* Item) const
{
	if (!QuickSlotInventory.IsValidIndex(QuickSlotIndex) || !IsValid(Item))
	{
		return false;
	}

	const EQuickSlotType SlotType = QuickSlotInventory[QuickSlotIndex].SlotType;


	// TODO: Item Type 확인 방법 다른 방법 생각해보기
	switch (SlotType)
	{
	case EQuickSlotType::PrimaryWeapon:
	case EQuickSlotType::SecondaryWeapon:
		return SourceInventory == EInventoryName::Weapon
			&& Item->IsWeapon()
			&& Item->IsA(AWeapon::StaticClass());

	case EQuickSlotType::ThrowableWeapon:
		return SourceInventory == EInventoryName::ThrowableWeapon
			&& Item->IsA(AThrowableWeapon::StaticClass());

	case EQuickSlotType::Flashlight:
		return SourceInventory == EInventoryName::Item
			&& Item->IsA(AFlashlightItem::StaticClass());

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
	QuickSlotInventory;

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

	QuickSlot.Count; //TODO: 이부분에 대한 구현 필요함. Item 종류에 따라서 Count 자체의 정의가 달라지기 때문에 별도의 처리가 필요함.

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
