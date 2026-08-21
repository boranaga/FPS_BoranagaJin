#pragma once

#include "CoreMinimal.h"
#include "PlayableMapInfo.generated.h"

USTRUCT(BlueprintType)
struct FPlayableMapInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UWorld> LevelAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UTexture2D> Thumbnail;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Description;

	bool IsValid() const
	{
		return !LevelAsset.IsNull();
	}
};