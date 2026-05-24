


#include "Items/InventorySystemComponent.h"
#include "Items/Item.h"
#include "Items/ItemPickUp.h"
#include "Items/ItemName.h"

#include "Characters/Player/CharacterPlayer.h"


#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

UInventorySystemComponent::UInventorySystemComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UInventorySystemComponent::BeginPlay()
{
	Super::BeginPlay();
	InitPlayerReference();
}

void UInventorySystemComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	SearchItem(); //TODO: 플레이어가 Idle 상태일 때만 Searching 하도록 해야함.
}

void UInventorySystemComponent::InitPlayerReference()
{
	ACharacterPlayer* NewPlayerOwner = Cast<ACharacterPlayer>(GetOwner());
	if (IsValid(NewPlayerOwner))
	{
		PlayerOwner = NewPlayerOwner;
		PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		//PlayerController = Cast<APlayerController>(PlayerOwner->GetController())

		//// Set up action bindings
		//if (PlayerController)
		//{
		//	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		//	{
		//		// Set the priority of the mapping to 1, so that it overrides the Jump action with the Fire action when using touch input
		//		Subsystem->AddMappingContext(WeaponSystemMappingContext, 1);
		//	}

		//	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent))
		//	{
		//		// Inereact With WeaponPickUp
		//		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &UWeaponSystemComponent::PickUpWeapon);

		//		// Switch Weapon
		//		EnhancedInputComponent->BindAction(SwitchWeaponUpAction, ETriggerEvent::Started, this, &UWeaponSystemComponent::SwitchToNextWeapon);
		//		EnhancedInputComponent->BindAction(SwitchWeaponDownAction, ETriggerEvent::Started, this, &UWeaponSystemComponent::SwitchToPreviousWeapon);

		//		EnhancedInputComponent->BindActionValueLambda(SwitchWeapon1Action, ETriggerEvent::Started, [this](const FInputActionValue& InputActionValue, int32 idx) {SwitchToIndex(idx); }, 0);
		//		EnhancedInputComponent->BindActionValueLambda(SwitchWeapon2Action, ETriggerEvent::Started, [this](const FInputActionValue& InputActionValue, int32 idx) {SwitchToIndex(idx); }, 1);
		//		EnhancedInputComponent->BindActionValueLambda(SwitchWeapon3Action, ETriggerEvent::Started, [this](const FInputActionValue& InputActionValue, int32 idx) {SwitchToIndex(idx); }, 2);
		//	}
		//}
	}
}

bool UInventorySystemComponent::SearchItem()
{
	return false;
}
