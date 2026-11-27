

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PoolableActorInterface.h"
#include "Objects/DestructibleObject.h"
#include "ItemName.h"
#include "Weapons/WeaponName.h"
#include "SaveSystem/InventorySaveData.h"
#include "ItemPickUp.generated.h"

class AItem;
class ACharacterPlayer;
class UPickUpComponent;
class UInteractionPointComponent;

UCLASS()
class FPS_BORANAGAJIN_API AItemPickUp : public ADestructibleObject, public IPoolableActorInterface
{
	GENERATED_BODY()
public:
	AItemPickUp();
protected:
	virtual void BeginPlay() override;
public:
	UFUNCTION()
	AItem* SpawnItem(ACharacterPlayer* Character);
	void DeactivateItemPickUp();
	void ActivateItemPickUp(FVector location);
	void DestroyItemPickUp();
	void SetItemPtr(TObjectPtr<AItem> NewItem);
	void SetDroppedItemState(const FItemInstanceSaveData& State, int32 InAmmo = INDEX_NONE);
	TSubclassOf<AItem> GetItemClass() const;
	FName GetItemID() const;
	int32 GetItemNum() const { return ItemNum; }
	void ClearItemLink();
	bool HasDroppedItemState() const { return DroppedItemState.IsValid(); }
	FGuid GetLegacyItemInstanceID() const { return ItemInstanceID; }
#pragma region SaveableActorInterface
	virtual bool ShouldSaveTransform() const override { return true; }
	virtual void OnAfterLoad() override;
#pragma endregion
#pragma region PoolableActorInterface
public:
	virtual void SetOwningPool(UObjectPoolSubsystem* NewPool) override;
	virtual void OnActivateFromPool() override;
	virtual void OnDeactivateToPool() override;
	virtual bool IsActiveInPool() const override;
	void DeactivateItemPickUp_Pool();
private:
	UPROPERTY()
	TObjectPtr<UObjectPoolSubsystem> OwningPool;
	UPROPERTY(SaveGame)
	bool bIsActiveInPool = false;
#pragma endregion
protected:
	UPROPERTY(EditDefaultsOnly, Category = Item)
	TSubclassOf<AItem> ItemClass;
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Item, meta = (AllowPrivateAccess = "true"))
	//USkeletalMeshComponent* IntactMesh = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EItemName ItemName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EWeaponName WeaponName;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Item, meta = (AllowPrivateAccess = "true"))
	UPickUpComponent* PickUpComponent = nullptr;

	/** Default HUD anchor; extra UInteractionPointComponents can be added in BP. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item|Interaction")
	TObjectPtr<UInteractionPointComponent> DefaultPickupInteractionPoint;
	UPROPERTY()
	TObjectPtr<AItem> ItemPtr = nullptr;
	UPROPERTY(SaveGame)
	FGuid ItemInstanceID;
protected:
	UPROPERTY(EditAnywhere)
	bool bIsWeapon = false;
	UPROPERTY(EditAnywhere)
	bool bIsThrowableWeapon = false;
	UPROPERTY(EditAnywhere)
	bool bIsMagazine = false;
	UPROPERTY(EditAnywhere)
	bool bIsStackable = true;
	UPROPERTY(EditAnywhere, SaveGame)
	int32 NumAmmo = 10;
	UPROPERTY(EditAnywhere, SaveGame, Category = "Item", meta = (ClampMin = "1"))
	int32 ItemNum = 1;
	UPROPERTY(SaveGame)
	FItemInstanceSaveData DroppedItemState;
	UPROPERTY(EditAnywhere)
	FVector PoolLocation = FVector(0.f, 0.f, 0.f);
public:
	EItemName GetItemName() const { return DroppedItemState.IsValid() ? DroppedItemState.ItemName : ItemName; }
	EWeaponName GetWeaponName() const { return WeaponName; }
	bool IsWeapon() const { return bIsWeapon; }
	bool IsThrowableWeapon() const { return bIsThrowableWeapon; }
	bool IsMagazine() const { return bIsMagazine; }
	bool IsStackable() const { return bIsStackable; }
	int32 GetAmmo() const { return NumAmmo; }
};
