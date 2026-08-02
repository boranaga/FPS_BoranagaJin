// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/InventorySystemComponent.h"
#//include "Items/Item.h"
#include "Items/InventorySlot.h"
#include "Items/ItemPickUp.h"
#include "Items/Weapons/Weapon.h"
#include "Items/Weapons/ThrowableWeapon.h"
#include "Items/FlashlightItem.h"
#include "Characters/Player/CharacterPlayer.h"

#include "Items/Weapons/WeaponName.h"
#include "Items/WeaponState/WeaponBaseState.h"
#include "Items/WeaponState/WeaponIdleState.h"
#include "Items/WeaponState/WeaponFiringState.h"
#include "Items/WeaponState/WeaponUnequippedState.h"
#include "Items/WeaponState/WeaponReloadingState.h"
#include "Items/WeaponState/WeaponPumpActionReloadState.h"
#include "Items/WeaponState/WeaponSwitchingState.h"
//#include "Items/WeaponState/WeaponTargetingState.h"
#include "Items/WeaponState/WeaponChargingState.h"
#include "Items/WeaponState/WeaponWaitingState.h"

#include "UI/InteractionWidget.h"

#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

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

	PlayerOwner->OnInventorySwapRequestedDelegate.AddDynamic(this, &UInventorySystemComponent::SwapItemInventorySlots);
	PlayerOwner->OnInventorySlotDropRequestedDelegate.AddUObject(this, &UInventorySystemComponent::DropItemInventorySlot);
	PlayerOwner->OnInventorySlotUseRequestedDelegate.AddUObject(this, &UInventorySystemComponent::UseItemInventorySlot);
}

void UInventorySystemComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	SearchItems(); //TODO: 탐색 조건 넣기
	//CalculateScreenCenterWorldPositionAndDirection(ScreenCenterWorldLocation, ScreenCenterWorldDirection);
	CalculateTargetRightHandPosition();
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

				// Switch Weapon
				EnhancedInputComponent->BindAction(SwitchWeaponUpAction, ETriggerEvent::Started, this, &UInventorySystemComponent::SwitchToNextWeapon);
				EnhancedInputComponent->BindAction(SwitchWeaponDownAction, ETriggerEvent::Started, this, &UInventorySystemComponent::SwitchToPreviousWeapon);

				EnhancedInputComponent->BindActionValueLambda(SwitchWeapon1Action, ETriggerEvent::Started, [this](const FInputActionValue& InputActionValue, int32 idx) {SwitchToIndex(idx); }, 0);
				EnhancedInputComponent->BindActionValueLambda(SwitchWeapon2Action, ETriggerEvent::Started, [this](const FInputActionValue& InputActionValue, int32 idx) {SwitchToIndex(idx); }, 1);
				EnhancedInputComponent->BindActionValueLambda(SwitchWeapon3Action, ETriggerEvent::Started, [this](const FInputActionValue& InputActionValue, int32 idx) {SwitchToIndex(idx); }, 2);

				// Flash Light
				EnhancedInputComponent->BindAction(FlashLightAction, ETriggerEvent::Started, this, &UInventorySystemComponent::ToggleFlashLight);
			}
		}
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

	if (bWeaponAdded)
	{
		if (!CurrWeapon)
		{
			CurrWeapon = NewWeapon;
			if (CurrFlashLightItem && bIsUsingFlashLightItem)
			{
				return true;
			}
			CurrWeapon->Equip(PlayerOwner);
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

	if (!bWeaponAdded)
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
	if (!IsValidSlotIndex(TargetInventory, SlotIndex) || RemoveCount <= 0)
	{
		return false;
	}

	FInventorySlot& Slot = TargetInventory[SlotIndex];

	if (Slot.IsEmpty())
	{
		return false;
	}

	return Slot.RemoveItem(RemoveCount);
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

void UInventorySystemComponent::PrintInventory() const
{
	UE_LOG(LogTemp, Warning, TEXT("========== Inventory =========="));

	for (int32 i = 0; i < ItemInventory.Num(); ++i)
	{
		const FInventorySlot& Slot = ItemInventory[i];

		if (Slot.IsEmpty())
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[%02d] Empty"),
				i);
		}
		else
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[%02d] ItemID=%s Count=%d"),
				i,
				*Slot.ItemID.ToString(),
				Slot.Count);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("==============================="));
}

void UInventorySystemComponent::InitInventoryUI()
{
	PlayerOwner->OnInventoryCreatedDelegate.Broadcast(MaxItemSlotsCount);
	PlayerOwner->OnInventoryUpdatedDelegate.Broadcast(ItemInventory);

	PlayerOwner->OnWeaponInventoryCreatedDelegate.Broadcast(MaxWeaponSlotsCount);
	PlayerOwner->OnWeaponInventoryUpdatedDelegate.Broadcast(WeaponInventory);

	PlayerOwner->OnThrowableWeaponInventoryCreatedDelegate.Broadcast(MaxThrowableWeaponSlotsCount);
	PlayerOwner->OnThrowableWeaponInventoryUpdatedDelegate.Broadcast(ThrowableWeaponInventory);
}

void UInventorySystemComponent::SwapItemInventorySlots(FName InventoryName, int32 FromIndex, int32 ToIndex)
{
	if (InventoryName == FName("ItemInventory"))
	{
		if (!SwapSlots(ItemInventory, FromIndex, ToIndex)) { return; }
		if (PlayerOwner)
		{
			PlayerOwner->OnInventoryUpdatedDelegate.Broadcast(ItemInventory);
		}
	}
	else if (InventoryName == FName("WeaponInventory"))
	{
		if (!SwapSlots(WeaponInventory, FromIndex, ToIndex)) { return; }
		if (PlayerOwner)
		{
			PlayerOwner->OnWeaponInventoryUpdatedDelegate.Broadcast(WeaponInventory);
		}
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


		ItemToDrop->DeactivateItemAndGetItemPickUp();

		if (PlayerOwner)
		{
			PlayerOwner->OnInventoryUpdatedDelegate.Broadcast(ItemInventory);
		}

		PrintInventory();
	}
	else if (InventoryName == FName("WeaponInventory"))
	{
		if (!WeaponInventory.IsValidIndex(SlotIndex)) { return; }
		if (WeaponInventory[SlotIndex].IsEmpty()) { return; }

		if (CurrWeapon)
		{
			if (CurrWeapon->GetCurrentState()->GetWeaponStateType() != EWeaponStateType::WeaponStateType_Idle
				&& CurrWeapon->GetCurrentState()->GetWeaponStateType() != EWeaponStateType::WeaponStateType_Unequipped)
			{
				return;
			}
		}


		// TODO: GetItem(0)이 아니라 더 깔끔한 방식이 필요함
		AWeapon* Weapon = Cast<AWeapon>(WeaponInventory[SlotIndex].GetItem(0));
		Weapon->UnequipWeapon_Legacy(PlayerOwner);
		if (Weapon == CurrWeapon) { CurrWeapon = nullptr; }

		WeaponInventory[SlotIndex].GetItem(0)->DeactivateItemAndGetItemPickUp();

		WeaponInventory[SlotIndex].ClearSlot();
		if (PlayerOwner)
		{
			PlayerOwner->OnWeaponInventoryUpdatedDelegate.Broadcast(WeaponInventory);
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

	PrintInventory();
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
			if (CurrWeapon->GetCurrentState()->GetWeaponStateType() != EWeaponStateType::WeaponStateType_Unequipped
				&& CurrWeapon->GetCurrentState()->GetWeaponStateType() != EWeaponStateType::WeaponStateType_Idle)
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
			PlayerOwner->OnWeaponInventoryUpdatedDelegate.Broadcast(WeaponInventory);
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
			PlayerOwner->OnThrowableWeaponInventoryUpdatedDelegate.Broadcast(ThrowableWeaponInventory);
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
			PlayerOwner->OnInventoryUpdatedDelegate.Broadcast(ItemInventory);
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
	if (CurrWeapon && CurrWeapon->GetCurrentState()->GetWeaponStateType() == EWeaponStateType::WeaponStateType_Waiting) { return true; }
	else { return false; }
}
EWeaponStateType UInventorySystemComponent::GetCurrWeaponStateType() const
{
	if (CurrWeapon) return CurrWeapon->GetCurrentState()->GetWeaponStateType();
	else return EWeaponStateType::WeaponStateType_None;
}

void UInventorySystemComponent::SwitchToPreviousWeapon()
{
	//if (!CurrWeapon) { return; }
	//if (CurrWeapon->GetCurrentState()->GetWeaponStateType() == EWeaponStateType::WeaponStateType_Switching
	//	|| CurrWeapon->GetCurrentState()->GetWeaponStateType() == EWeaponStateType::WeaponStateType_Unequipped) {
	//	return;
	//}
	//if (GetOccupiedSlotCount(WeaponInventory) > 1)
	//{
	//	const int32 PrevIndex = CurrWeaponIdx;

	//	CurrWeaponIdx--;
	//	if (CurrWeaponIdx < 0)
	//	{
	//		CurrWeaponIdx = WeaponInventory.Num() + CurrWeaponIdx;
	//	}
	//	ChangeWeapon(CurrWeaponIdx);

	//	//UE_LOG(LogTemp, Warning, TEXT("Broadcasting weapon switch: %d -> %d"), PrevIndex, CurrWeaponIdx);
	//	OnWeaponSwitched.Broadcast(PrevIndex, CurrWeaponIdx);
	//}

	//-----------------------------------------------------

	if (!CurrWeapon) { return; }
	if (CurrWeapon->GetCurrentState()->GetWeaponStateType() == EWeaponStateType::WeaponStateType_Switching
		|| CurrWeapon->GetCurrentState()->GetWeaponStateType() == EWeaponStateType::WeaponStateType_Unequipped) {
		return;
	}

	int32 NextIdx = CurrWeaponIdx;

	for (int32 i = 1; i < WeaponInventory.Num(); i++)
	{
		NextIdx -= 1;
		if (NextIdx < 0)
		{
			NextIdx = WeaponInventory.Num() + NextIdx;
		}

		if (!WeaponInventory[NextIdx].IsEmpty())
		{
			CurrWeaponIdx = NextIdx;
			ChangeWeapon(CurrWeaponIdx);
			break;
		}
	}
}

void UInventorySystemComponent::SwitchToNextWeapon()
{
	//if (!CurrWeapon) { return; }
	//if (CurrWeapon->GetCurrentState()->GetWeaponStateType() == EWeaponStateType::WeaponStateType_Switching
	//	|| CurrWeapon->GetCurrentState()->GetWeaponStateType() == EWeaponStateType::WeaponStateType_Unequipped) {
	//	return;
	//}
	//if (GetOccupiedSlotCount(WeaponInventory) > 1)
	//{
	//	const int32 PrevIndex = CurrWeaponIdx;

	//	CurrWeaponIdx = (CurrWeaponIdx + 1) % WeaponInventory.Num();
	//	ChangeWeapon(CurrWeaponIdx);

	//	//UE_LOG(LogTemp, Warning, TEXT("Broadcasting weapon switch: %d -> %d"), PrevIndex, CurrWeaponIdx);
	//	OnWeaponSwitched.Broadcast(PrevIndex, CurrWeaponIdx);
	//}

	//-----------------------------------

	if (!CurrWeapon) { return; }
	if (CurrWeapon->GetCurrentState()->GetWeaponStateType() == EWeaponStateType::WeaponStateType_Switching
		|| CurrWeapon->GetCurrentState()->GetWeaponStateType() == EWeaponStateType::WeaponStateType_Unequipped) {
		return;
	}

	int32 NextIdx = CurrWeaponIdx;

	for (int32 i = 1; i < WeaponInventory.Num(); i++)
	{
		NextIdx += 1;
		NextIdx = NextIdx % WeaponInventory.Num();

		if (!WeaponInventory[NextIdx].IsEmpty())
		{
			CurrWeaponIdx = NextIdx;
			ChangeWeapon(CurrWeaponIdx);
			break;
		}
	}
}

void UInventorySystemComponent::SwitchToIndex(int32 idx)
{
	if (CurrWeapon)
	{
		if (CurrWeapon->GetCurrentState()->GetWeaponStateType() == EWeaponStateType::WeaponStateType_Switching
			|| CurrWeapon->GetCurrentState()->GetWeaponStateType() == EWeaponStateType::WeaponStateType_Unequipped) {
			return;
		}
	}
	if (WeaponInventory.IsValidIndex(idx) && CurrWeaponIdx != idx)
	{
		const int32 PrevIndex = CurrWeaponIdx;
		CurrWeaponIdx = idx;
		ChangeWeapon(CurrWeaponIdx);

		//UE_LOG(LogTemp, Warning, TEXT("Broadcasting weapon switch: %d -> %d"), PrevIndex, CurrWeaponIdx);
		OnWeaponSwitched.Broadcast(PrevIndex, CurrWeaponIdx);
	}
}


void UInventorySystemComponent::SwitchToNextItem()
{
	if (!NextItem) { return; }
	if (NextItem->IsWeapon())
	{
		CurrWeapon = Cast<AWeapon>(NextItem);
	}
	else
	{
		bIsUsingFlashLightItem = true; //TODO: 이 변수에 대한 처리가 필요함
	}

	NextItem->Equip(PlayerOwner);
}

void UInventorySystemComponent::ChangeWeapon(int32 WeaponIndex)
{
	if (WeaponInventory.IsValidIndex(WeaponIndex) && !WeaponInventory[WeaponIndex].IsEmpty())
	{
		NextItem = WeaponInventory[WeaponIndex].GetItem(0);

		if (IsValid(CurrWeapon))
		{
			//CurrWeapon->SwitchWeapon(PlayerOwner, false);
			CurrWeapon->Unequip(PlayerOwner);
		}
		else
		{
			//SwitchToOtherWeapon();
			SwitchToNextItem();
		}
	}
}
EWeaponStateType UInventorySystemComponent::GetCurrThrowableWeaponStateType() const
{
	if (CurrThrowableWeapon) return CurrThrowableWeapon->GetCurrentState()->GetWeaponStateType();
	else return EWeaponStateType::WeaponStateType_None;
}
void UInventorySystemComponent::SwitchToPreviousThrowableWeapon()
{
	if (CurrThrowableWeapon->GetCurrentState()->GetWeaponStateType() == EWeaponStateType::WeaponStateType_Switching
		|| CurrThrowableWeapon->GetCurrentState()->GetWeaponStateType() == EWeaponStateType::WeaponStateType_Unequipped) {
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
	if (CurrThrowableWeapon->GetCurrentState()->GetWeaponStateType() == EWeaponStateType::WeaponStateType_Switching
		|| CurrThrowableWeapon->GetCurrentState()->GetWeaponStateType() == EWeaponStateType::WeaponStateType_Unequipped) {
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
void UInventorySystemComponent::SwitchToOtherThrowableWeapon()
{
	//ThrowableWeaponInventory[CurrThrowableWeaponIdx]->SwitchWeapon(PlayerOwner, true);
	//CurrThrowableWeapon = ThrowableWeaponInventory[CurrThrowableWeaponIdx];
}
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
		EWeaponStateType CurrWeaponStateType = CurrWeapon->GetCurrentState()->GetWeaponStateType();
		if (CurrWeaponStateType == EWeaponStateType::WeaponStateType_Idle) //TODO: 다른 state일 때 스킬 어떻게 작동해야 하는지
		{
			CurrWeapon->ChangeState(CurrWeapon->WaitingState);
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
		EWeaponStateType CurrWeaponStateType = CurrWeapon->GetCurrentState()->GetWeaponStateType();
		if (CurrWeaponStateType == EWeaponStateType::WeaponStateType_Waiting)
		{
			CurrWeapon->ChangeState(CurrWeapon->IdleState);
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

	if (CurrFlashLightItem->IsFlashlightOn())
	{
		if (CurrWeapon)
		{
			NextItem = CurrWeapon;
			CurrFlashLightItem->Unequip(PlayerOwner);
			bIsUsingFlashLightItem = false;
		}
		else
		{
			CurrFlashLightItem->Unequip(PlayerOwner);
			bIsUsingFlashLightItem = false;
		}
	}
	else
	{
		if (CurrWeapon)
		{
			if (CurrWeapon->GetCurrentState()->GetWeaponStateType() == EWeaponStateType::WeaponStateType_Switching
				|| CurrWeapon->GetCurrentState()->GetWeaponStateType() == EWeaponStateType::WeaponStateType_Unequipped) {
				return;
			}

			NextItem = CurrFlashLightItem;

			if (CurrWeapon->IsTwoHandedWeapon())
			{
				CurrWeapon->Unequip(PlayerOwner);
			}
			else
			{

			}
		}
		else
		{
			CurrFlashLightItem->Equip(PlayerOwner);
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
			return true;
		}
	}
	return false;
}
#pragma endregion

