

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemName.h"
#include "PoolableActorInterface.h"
#include "Item.generated.h"

class ACharacterPlayer;
class AItemPickUp;

UCLASS()
class FPS_BORANAGAJIN_API AItem : public AActor, public IPoolableActorInterface
{
	GENERATED_BODY()
	
public:	
	AItem();
protected:
	virtual void BeginPlay() override;
public:	
	virtual void Tick(float DeltaTime) override;
	virtual void InitItem(ACharacterPlayer* NewCharacter, AItemPickUp* PickUpActor = nullptr);
	virtual bool UseItem(ACharacterPlayer* UsingCharacter);
protected:
	UPROPERTY(EditAnywhere, Category = Weapon)
	TSoftObjectPtr<UDataTable> ItemDataTable;
	UPROPERTY(EditAnywhere, Category = Weapon)
	FName ItemRowName;
	UPROPERTY() UDataTable* LoadedItemTable = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EItemName ItemName = EItemName::ItemName_Base;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ItemID;
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ItemMesh")
	//USkeletalMeshComponent* ItemMesh;
	UPROPERTY()
	ACharacterPlayer* Character = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsStackable = true;
	UPROPERTY(VisibleAnywhere)
	bool bWasInitialized = false;

	UPROPERTY()
	TObjectPtr<AItemPickUp> ItemPickUp = nullptr;
	//UPROPERTY()
	//AItemPickUp* ItemPickUp = nullptr;
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ItemImage")
	UTexture2D* ItemImage;
public:
	//USkeletalMeshComponent* GetItemMesh() { return ItemMesh; }
	EItemName GetItemName() const { return ItemName; }
	FName GetItemID() const { return ItemID; }
	bool IsStackable() const { return bIsStackable; }
protected:
	virtual void LoadItemData();

#pragma region PoolableActorInterface
public:
	virtual void SetOwningPool(UObjectPoolSubsystem* NewPool) override;
	virtual void OnActivateFromPool() override;
	virtual void OnDeactivateToPool() override;
	virtual bool IsActiveInPool() const override;
	void DeactivateItemAndGetItemPickUp();
	void DeactivateItem();
private:
	UPROPERTY()
	TObjectPtr<UObjectPoolSubsystem> OwningPool;
	bool bIsActiveInPool = false;
#pragma endregion
};
