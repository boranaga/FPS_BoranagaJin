

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemName.h"
#include "Weapons/WeaponName.h"
#include "ItemPickUp.generated.h"

class AItem;
class ACharacterPlayer;
class UPickUpComponent;

UCLASS()
class FPS_BORANAGAJIN_API AItemPickUp : public AActor
{
	GENERATED_BODY()
public:
	AItemPickUp();
protected:
	virtual void BeginPlay() override;
public:
	UFUNCTION()
	AItem* SpawnItem(ACharacterPlayer* Character);
	void DestroyItemPickUp();
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
protected:
	UPROPERTY(EditAnywhere)
	bool bIsWeapon = false;
	UPROPERTY(EditAnywhere)
	bool bIsMagazine = false;
	UPROPERTY(EditAnywhere)
	bool bIsStackable = true;
	UPROPERTY(EditAnywhere)
	int32 NumAmmo = 10;
public:
	EItemName GetItemName() const { return ItemName; }
	EWeaponName GetWeaponName() const { return WeaponName; }
	bool IsWeapon() const { return bIsWeapon; }
	bool IsMagazine() const { return bIsMagazine; }
	bool IsStackable() const { return bIsStackable; }
	int32 GetAmmo() const { return NumAmmo; }
};
