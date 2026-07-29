#include "Items/BatteryItem.h"
#include "Items/FlashlightItem.h"

ABatteryItem::ABatteryItem()
{
	PrimaryActorTick.bCanEverTick = false;

	ItemName = EItemName::ItemName_Battery;

	bIsStackable = true;
}

bool ABatteryItem::UseItem(ACharacterPlayer* UsingCharacter)
{
	/*
	 * 배터리만으로는 어느 손전등에 사용할지 알 수 없다.
	 *
	 * 인벤토리 시스템이 현재 장착 중인 손전등을 찾아서
	 * UseBatteryOnFlashlight()를 호출하도록 구성하는 편이 좋다.
	 */
	return false;
}

bool ABatteryItem::UseBatteryOnFlashlight(AFlashlightItem* Flashlight)
{
	if (!IsValid(Flashlight) || !HasCharge())
	{
		return false;
	}

	return Flashlight->ReplaceBattery(this);
}

float ABatteryItem::ConsumeBattery()
{
	if (!HasCharge())
	{
		return 0.0f;
	}

	bWasConsumed = true;

	return BatteryCharge;
}