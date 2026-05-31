// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/ItemName.h"
#include "ItemData.generated.h"

USTRUCT(BlueprintType)
struct FPS_BORANAGAJIN_API FItemData : public FTableRowBase
{
	GENERATED_BODY()

	EItemName ItemName;
	bool Description;
	UTexture2D* ItemImage;
	bool ItemMesh;
	bool ItemType;
	int32 SlotQuantity;
};


