

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "Characters/Player/CharacterPlayer.h"
#include "PickUpComponent.generated.h"

//TODO: Delegate 말고 Interface로 구현하는게 좋을지 고민해봐야함
// Declaration of the delegate that will be called when someone picks this up
// The character picking this up is the parameter sent with the notification
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPickUpWeapon, ACharacterPlayer*, PickUpCharacter);

UCLASS(Blueprintable, BlueprintType, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FPS_BORANAGAJIN_API UPickUpComponent : public USphereComponent
{
	GENERATED_BODY()
public:
	//TODO: Interface로 구현할지 고민해봐야함
	/** Delegate to whom anyone can subscribe to receive this event */
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnPickUpWeapon OnPickUp;

	UPickUpComponent();
protected:
	virtual void BeginPlay() override;
	UFUNCTION()
	void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};

