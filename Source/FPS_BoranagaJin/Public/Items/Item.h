

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemName.h"
#include "Item.generated.h"

class ACharacterPlayer;

UCLASS()
class FPS_BORANAGAJIN_API AItem : public AActor
{
	GENERATED_BODY()
	
public:	
	AItem();
protected:
	virtual void BeginPlay() override;
public:	
	virtual void Tick(float DeltaTime) override;
	virtual void InitItem(ACharacterPlayer* NewCharacter);
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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ItemMesh")
	USkeletalMeshComponent* ItemMesh;
	UPROPERTY()
	ACharacterPlayer* Character = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsStackable = true;
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ItemImage")
	UTexture2D* ItemImage;
public:
	USkeletalMeshComponent* GetItemMesh() { return ItemMesh; }
	EItemName GetItemName() const { return ItemName; }
	FName GetItemID() const { return ItemID; }
	bool IsStackable() const { return bIsStackable; }
protected:
	virtual void LoadItemData();
};
