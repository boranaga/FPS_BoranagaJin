#pragma once

#include "CoreMinimal.h"
#include "Items/Item.h"
#include "BatteryItem.generated.h"

class AFlashlightItem;

UCLASS()
class FPS_BORANAGAJIN_API ABatteryItem : public AItem
{
	GENERATED_BODY()

public:
	ABatteryItem();

public:
	virtual bool UseItem(ACharacterPlayer* UsingCharacter) override;

	/**
	 * 지정한 손전등에 이 배터리를 장착한다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Battery")
	bool UseBatteryOnFlashlight(AFlashlightItem* Flashlight);

	/**
	 * 배터리를 소모하고 충전량을 반환한다.
	 * 이미 소모되었으면 0을 반환한다.
	 */
	float ConsumeBattery();

	UFUNCTION(BlueprintPure, Category = "Battery")
	bool HasCharge() const
	{
		return !bWasConsumed && BatteryCharge > 0.0f;
	}

	UFUNCTION(BlueprintPure, Category = "Battery")
	float GetBatteryCharge() const
	{
		return BatteryCharge;
	}

protected:
	/** 이 배터리를 장착했을 때 제공되는 충전량 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Battery",
		meta = (ClampMin = "0.0"))
	float BatteryCharge = 100.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Battery")
	bool bWasConsumed = false;
};