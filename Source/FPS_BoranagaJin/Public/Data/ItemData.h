// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/ItemName.h"
#include "ItemData.generated.h"

USTRUCT(BlueprintType)
struct FPS_BORANAGAJIN_API FItemData : public FTableRowBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Name")
	EItemName ItemName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Description")
	bool Description;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ItemImage")
	UTexture2D* ItemImage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ItemMesh")
	bool ItemMesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ItemType")
	bool ItemType;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlotQuantity")
	int32 SlotQuantity;
};


