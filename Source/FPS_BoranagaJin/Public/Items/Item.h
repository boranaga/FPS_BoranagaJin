

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemName.h"
#include "PoolableActorInterface.h"
#include "Interface/SaveableActorInterface.h"
#include "Item.generated.h"

class ACharacterPlayer;
class AItemPickUp;

struct FItemInstanceSaveData;

UCLASS()
class FPS_BORANAGAJIN_API AItem : public AActor, public IPoolableActorInterface, public ISaveableActorInterface
{
	GENERATED_BODY()
	
public:	
	AItem();
protected:
	virtual void BeginPlay() override;
public:	
	virtual void Tick(float DeltaTime) override;
	virtual void InitItem(ACharacterPlayer* NewCharacter, AItemPickUp* PickUpActor = nullptr);
	virtual void InitItemPost(ACharacterPlayer* NewCharacter, AItemPickUp* PickUpActor = nullptr);
	virtual bool UseItem(ACharacterPlayer* UsingCharacter);

	virtual void Equip(ACharacterPlayer* TargetCharacter);
	virtual void Unequip(ACharacterPlayer* TargetCharacter);
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsWeapon = false;
	UPROPERTY(VisibleAnywhere)
	bool bWasInitialized = false;

	UPROPERTY(EditAnywhere, Category = "ItemPickUp")
	TSubclassOf<AItemPickUp> ItemPickUpClass;
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
	bool IsWeapon() const { return bIsWeapon; }
protected:
	virtual void LoadItemData();
	virtual bool AttachItemToPlayer(ACharacterPlayer* TargetCharacter);
#pragma region SaveableActorInterface
#if WITH_EDITOR
	virtual void PostEditImport() override;
	virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode) override;
#endif
public:
	virtual FGuid GetInstanceID() const override
	{
		return InstanceID;
	}
	virtual bool IsRuntimeSpawned() const override
	{
		return bRuntimeSpawned;
	}
	virtual bool ShouldSaveTransform() const override
	{
		return false;
	}
	virtual void OnAfterLoad() override;
	virtual void SetRuntimeSpawned(bool bIsRuntimeSpawned) override;
protected:
	UPROPERTY(VisibleInstanceOnly, SaveGame, Category = "Item|Save")
	FGuid InstanceID;
	UPROPERTY(Transient)
	bool bRuntimeSpawned = false;
#pragma endregion
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
#pragma region SaveAndLoad
public:
	// WriteItemSaveData()를 const 함수로 만들지 않은 이유는 Unreal의 Serialize()가 일반적으로 비-const 함수이기 때문임
	// 억지로 const_cast를 사용하는 것보다 저장 함수 자체를 비-const로 두는 것이 나음
	virtual bool WriteItemSaveData(FItemInstanceSaveData& OutSaveData);
	virtual bool LoadItemSaveData(const FItemInstanceSaveData& SaveData);
	//직렬화된 프로퍼티 복원 이후 실행되는 후처리 함수
	virtual void OnItemStateRestored();
#pragma endregion
};
