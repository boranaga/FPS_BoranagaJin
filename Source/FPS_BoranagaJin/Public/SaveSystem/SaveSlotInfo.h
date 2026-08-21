#pragma once

#include "CoreMinimal.h"
#include "SaveSlotInfo.generated.h"

USTRUCT(BlueprintType)
struct FSaveSlotInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString SlotName;

	UPROPERTY(BlueprintReadOnly)
	FName SavedLevelName = NAME_None;

	UPROPERTY(BlueprintReadOnly)
	FDateTime SavedAt;

	bool IsValid() const
	{
		return !SlotName.IsEmpty();
	}
};