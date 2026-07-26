// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Items/ItemName.h"
#include "Items/InventorySlot.h"
#include "Items/Weapons/WeaponName.h"
#include "Items/WeaponState/WeaponStateType.h"
#include "Items/Weapons/WeaponInterface.h"
#include "InventorySystemComponent.generated.h"

//MEMO: 다른 방식?
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponPickedUp, EWeaponName, WeaponName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponSwitched, int32, PrevIndex, int32, NewIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkillWeaponEquipped, class AWeapon*, NewSkillWeapon);

class ACharacterPlayer;
class AItem;
class AWeapon;
class AThrowableWeapon;
class AItemPickUp;
class AWeaponPickUp;
class UInteractionWidget;

struct FInventorySlot;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class FPS_BORANAGAJIN_API UInventorySystemComponent : public UActorComponent, public IWeaponInterface
{
	GENERATED_BODY()
public:	
	UInventorySystemComponent();

	UPROPERTY(BlueprintAssignable, Category = "Weapon")
	FOnWeaponPickedUp OnWeaponPickedUp;
	UPROPERTY(BlueprintAssignable, Category = "Weapon")
	FOnWeaponSwitched OnWeaponSwitched;
	UPROPERTY(BlueprintAssignable, Category = "Weapon")
	FOnSkillWeaponEquipped OnSkillWeaponEquipped;

protected:
	virtual void BeginPlay() override;
public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

#pragma region PlayerReference
private:
	void InitializePlayerReference();
#pragma endregion
#pragma region InputAction
private:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "WeaponSystem|PlayerReference", Meta = (AllowPrivateAccess = "true"))
	ACharacterPlayer* PlayerOwner;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "WeaponSystem|PlayerReference", Meta = (AllowPrivateAccess = "true"))
	class APlayerController* PlayerController;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputMappingContext* WeaponSystemMappingContext;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* InteractAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* SwitchWeaponUpAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* SwitchWeaponDownAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* SwitchWeapon1Action;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* SwitchWeapon2Action;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* SwitchWeapon3Action;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* LeftMouseButtonAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* RightMouseButtonAction;
#pragma endregion
#pragma region ItemInventory
protected:
	UPROPERTY(EditAnywhere)
	int32 MaxItemSlotsCount = 10;
	UPROPERTY(EditAnywhere)
	int32 MaxWeaponSlotsCount = 3;
	UPROPERTY(EditAnywhere)
	int32 MaxThrowableWeaponSlotsCount = 3;
	
	UPROPERTY()
	TArray<FInventorySlot> ItemInventory;
	UPROPERTY()
	TArray<FInventorySlot> WeaponInventory;
	UPROPERTY()
	TArray<FInventorySlot> ThrowableWeaponInventory;

	FTimerHandle InitInventoryUITimerHandle;

protected:
	void InitInventory();
	bool AddItem(TArray<FInventorySlot>& TargetInventory, AItem* NewItem, bool bIsStackable = true, int32 AddCount = 1);
	bool AddItemFromPickUp(AItemPickUp* NewItemPickUp, int32 AddCount = 1);
	bool AddWeaponFromPickUp(AItemPickUp* NewItemPickUp, int32 AddCount = 1);
	bool AddThrowableWeaponFromPickUp(AItemPickUp* NewItemPickUp, int32 AddCount = 1);
	bool RemoveItem(TArray<FInventorySlot>& TargetInventory, EItemName ItemName, int32 RemoveCount = 1);
	bool RemoveItemAtSlot(TArray<FInventorySlot>& TargetInventory, int32 SlotIndex, int32 RemoveCount = 1);
	bool SwapSlots(TArray<FInventorySlot>& TargetInventory, int32 FromIndex, int32 ToIndex);
	bool IsValidSlotIndex(TArray<FInventorySlot>& TargetInventory, int32 SlotIndex) const;
	int32 FindItemSlot(const TArray<FInventorySlot>& TargetInventory, EItemName ItemName) const;
	int32 FindEmptySlot(const TArray<FInventorySlot>& TargetInventory) const;
	int32 GetOccupiedSlotCount(const TArray<FInventorySlot>& TargetInventory) const;
	const TArray<FInventorySlot>& GetInventorySlots() const;

	void PrintInventory() const;
protected:
	void InitInventoryUI();

public:	
	UFUNCTION()
	void SwapItemInventorySlots(FName InventoryName, int32 FromIndex, int32 ToIndex);
	UFUNCTION()
	void DropItemInventorySlot(FName InventoryName, int32 SlotIndex);
	void UseItemInventorySlot(FName InventoryName, int32 SlotIndex);
#pragma endregion
#pragma region PickUpItem
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponSystem")
	float SearchItemRadius = 150.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponSystem")
	float SearchItemViewportRatio_Width = 0.7;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponSystem")
	float SearchItemViewportRatio_Height = 0.7;
protected:
	TTuple<FVector2D, bool> GetScreenPositionOfWorldLocation(const FVector& SearchLocation) const;
	bool IsInViewport(FVector2D ActorScreenPosition, float ScreenRatio_Width = 0.0f, float ScreenRatio_Height = 0.0f) const;
protected:
	bool SearchItems();
	void PickUpItem();
	bool ObtainItem(AItemPickUp* NewItemPickUp);
public:
	bool ObtainAmmo(AItemPickUp* MagazinePickUp);
protected:
	UPROPERTY()
	AItemPickUp* OverlappedItem;
#pragma endregion
#pragma region WeaponOwnership
protected:
	UPROPERTY(VisibleAnywhere)
	TMap<EWeaponName, bool> OwnedWeapons;
public:
	TMap<EWeaponName, bool> GetOwnerShipMap() const { return OwnedWeapons; }
protected:
	void LoadWSCData();
	void SaveInventory();
#pragma endregion
#pragma region Zoom
protected:
	bool bIsZoomIn = false;
public:
	bool IsZoomIn() const { return bIsZoomIn; }
	virtual void ZoomIn(bool bZoomIn) override;
	bool IsWeaponModifyingCamFov();
#pragma endregion
#pragma region Aiming
protected:
	FVector ScreenCenterWorldLocation;

	FVector ScreenCenterWorldDirection;

	FVector RightHandToAimSocketOffset;

	FVector TargetRightHandWorldLocation;
public:
	virtual void SetRightHandToAimSocketOffset(FVector offset) override;

	FVector GetScreenCenterWorldPosition() const { return ScreenCenterWorldLocation; }

	FVector GetScreenCenterWorldDirection() const { return ScreenCenterWorldDirection; }

	FVector CalculateScreenCenterWorldPositionAndDirection(FVector& OutWorldPosition, FVector& OutWorldDirection) const;

	FVector GetTargetRightHandWorldLocation() const { return TargetRightHandWorldLocation; }

	FVector CalculateTargetRightHandPosition();

	FTransform GetWeaponAimSocketRelativeTransform();
#pragma endregion
#pragma region WeaponInventory
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	AWeapon* CurrentWeapon = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	int32 CurrWeaponIdx = 0;
public:
	AWeapon* GetCurrentWeapon() { return CurrentWeapon; }
	int32 GetWeaponNum() { return WeaponInventory.Num(); }
	bool IsCurrentSkillWeaponTargeting();
	EWeaponStateType GetCurrWeaponStateType() const;

	void SwitchToPreviousWeapon();
	void SwitchToNextWeapon();
	void SwitchToIndex(int32 idx);

	virtual void SwitchToOtherWeapon() override;
	void ChangeWeapon(int32 WeaponIndex);
#pragma endregion
#pragma region ThrowableWeaponInventory
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ThrowableWeapon")
	AThrowableWeapon* CurrThrowableWeapon = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ThrowableWeapon")
	int32 CurrThrowableWeaponIdx = 0;
public:
	AThrowableWeapon* GetCurrentThrowableWeapon() { return CurrThrowableWeapon; }
	int32 GetThrowableWeaponNum() { return ThrowableWeaponInventory.Num(); }
	EWeaponStateType GetCurrThrowableWeaponStateType() const;

	void SwitchToPreviousThrowableWeapon();
	void SwitchToNextThrowableWeapon();
	void SwitchToIndexThrowableWeapon(int32 idx);

	virtual void SwitchToOtherThrowableWeapon() override;
	void ChangeThrowableWeapon(int32 WeaponIndex);
#pragma endregion
#pragma region Control
protected:
	UPROPERTY()
	AWeapon* ControllingWeapon = nullptr;
public:
	bool TryTakeControl(AWeapon* NewWeapon);
	void ReleaseControl();
#pragma endregion
};
