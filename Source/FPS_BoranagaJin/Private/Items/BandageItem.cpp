#include "Items/BandageItem.h"

#include "Characters/Player/CharacterPlayer.h"
#include "Characters/HealthComponent.h"

ABandageItem::ABandageItem()
{
	PrimaryActorTick.bCanEverTick = false;
	ItemName = EItemName::ItemName_Bandage;
	bIsStackable = true;
}

bool ABandageItem::UseItem(ACharacterPlayer* UsingCharacter)
{
	if (!IsValid(UsingCharacter))
	{
		UE_LOG(LogTemp, Warning, TEXT("ABandageItem::UseItem: UsingCharacter is invalid"));
		return false;
	}

	UHealthComponent* HealthComponent = UsingCharacter->FindComponentByClass<UHealthComponent>();

	if (!HealthComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABandageItem::UseItem: ""HealthComponent was not found on %s"), *UsingCharacter->GetName());
		return false;
	}

	/*
	 * 출혈하지 않는 상태에서는 붕대를 소비하지 않는다.
	 */
	if (!HealthComponent->IsBleeding())
	{
		UE_LOG(LogTemp, Log, TEXT("ABandageItem::UseItem: %s is not bleeding"), *UsingCharacter->GetName());
		return false;
	}

	HealthComponent->StopBleeding(RestoreHealthAmount);
	UE_LOG(LogTemp, Log, TEXT("%s used a bandage"), *UsingCharacter->GetName());

	return true;
}