#pragma once

#include "CoreMinimal.h"
#include "Items/Item.h"
#include "BandageItem.generated.h"

UCLASS()
class FPS_BORANAGAJIN_API ABandageItem : public AItem
{
	GENERATED_BODY()

public:
	ABandageItem();

	virtual bool UseItem(ACharacterPlayer* UsingCharacter) override;
protected:
	UPROPERTY(EditAnywhere, Category = "Bandage")
	bool bRestoreHealth = false;

	UPROPERTY(EditAnywhere, Category = "Bandage", meta = (EditCondition = "bRestoreHealth", ClampMin = "0.0"))
	float RestoreHealthAmount = 10.f;
};