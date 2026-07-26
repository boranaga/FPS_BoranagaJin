

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PoolableActorInterface.h"
#include "ItemName.h"
#include "Weapons/WeaponName.h"
#include "ItemPickUp.generated.h"

class AItem;
class ACharacterPlayer;
class UPickUpComponent;

UCLASS()
class FPS_BORANAGAJIN_API AItemPickUp : public AActor, public IPoolableActorInterface
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
	bool bIsActiveInPool = false;
#pragma endregion
protected:
	UPROPERTY(EditDefaultsOnly, Category = Item)
	TSubclassOf<AItem> ItemClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Item, meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* ItemMesh = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EItemName ItemName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EWeaponName WeaponName;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Item, meta = (AllowPrivateAccess = "true"))
	UPickUpComponent* PickUpComponent = nullptr;
	UPROPERTY()
	TObjectPtr<AItem> ItemPtr = nullptr;
protected:
	UPROPERTY(EditAnywhere)
	bool bIsWeapon = false;
	UPROPERTY(EditAnywhere)
	bool bIsThrowableWeapon = false;
	UPROPERTY(EditAnywhere)
	bool bIsMagazine = false;
	UPROPERTY(EditAnywhere)
	bool bIsStackable = true;
	UPROPERTY(EditAnywhere)
	int32 NumAmmo = 10;
	UPROPERTY(EditAnywhere)
	FVector PoolLocation = FVector(0.f, 0.f, 0.f);
public:
	EItemName GetItemName() const { return ItemName; }
	EWeaponName GetWeaponName() const { return WeaponName; }
	bool IsWeapon() const { return bIsWeapon; }
	bool IsThrowableWeapon() const { return bIsThrowableWeapon; }
	bool IsMagazine() const { return bIsMagazine; }
	bool IsStackable() const { return bIsStackable; }
	int32 GetAmmo() const { return NumAmmo; }
};
