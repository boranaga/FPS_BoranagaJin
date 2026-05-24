

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemName.h"
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
	//UFUNCTION()
	//void AttachToCharacter(ACharacterPlayer* Character);
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
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Item, meta = (AllowPrivateAccess = "true"))
	UPickUpComponent* PickUpComponent = nullptr;
public:
	EItemName GetItemName() const { return ItemName; }
};
