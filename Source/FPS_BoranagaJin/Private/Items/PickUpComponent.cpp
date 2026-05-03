


#include "Items/PickUpComponent.h"


UPickUpComponent::UPickUpComponent()
{
	SphereRadius = 32.f;
	//TODO: Enable Simulate Physics (Optional)
}
// Called when the game starts
void UPickUpComponent::BeginPlay()
{
	Super::BeginPlay();

	OnComponentBeginOverlap.AddDynamic(this, &UPickUpComponent::OnSphereBeginOverlap);
}

void UPickUpComponent::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Checking if it is a First Person Character overlapping
	ACharacterPlayer* Character = Cast<ACharacterPlayer>(OtherActor);
	if (Character != nullptr)
	{
		// Notify that the actor is being picked up
		OnPickUp.Broadcast(Character);

		// Unregister from the Overlap Event so it is no longer triggered
		OnComponentBeginOverlap.RemoveAll(this);

		UE_LOG(LogTemp, Warning, TEXT("Weapon Collided"));
	}
}
