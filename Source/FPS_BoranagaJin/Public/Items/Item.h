

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemName.h"
#include "Item.generated.h"

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

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EItemName ItemName = EItemName::ItemName_Base;
public:
	EItemName GetItemName() const { return ItemName; }
	
	
};
