

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
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EItemName ItemName = EItemName::ItemName_Base;
	UPROPERTY()
	ACharacterPlayer* Character = nullptr;
	bool bIsStackable = true;
public:
	EItemName GetItemName() const { return ItemName; }
	bool IsStackable() const { return bIsStackable; }
};
