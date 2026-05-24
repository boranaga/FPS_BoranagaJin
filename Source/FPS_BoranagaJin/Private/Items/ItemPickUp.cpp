


#include "Items/ItemPickUp.h"
#include "Items/Item.h"
#include "Items/PickUpComponent.h"

AItemPickUp::AItemPickUp()
{
	PickUpComponent = CreateDefaultSubobject<UPickUpComponent>(TEXT("PickUpComponent"));
	PickUpComponent->SetupAttachment(ItemMesh);
}

void AItemPickUp::BeginPlay()
{
	Super::BeginPlay();
}

AItem* AItemPickUp::SpawnItem(ACharacterPlayer* Character)
{
	return nullptr;
}

void AItemPickUp::DestroyItemPickUp()
{
	Destroy();
}
