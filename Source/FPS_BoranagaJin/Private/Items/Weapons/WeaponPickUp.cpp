// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Weapons/WeaponPickUp.h"
#include "Items/Weapons/Weapon.h"
#include "Items/PickUpComponent.h"

// Sets default values
AWeaponPickUp::AWeaponPickUp()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	WeaponName = EWeaponName::WeaponName_Rifle;
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(RootComponent);


	PickUpComponent = CreateDefaultSubobject<UPickUpComponent>(TEXT("PickUpComponent"));
	PickUpComponent->SetupAttachment(WeaponMesh);
}
void AWeaponPickUp::BeginPlay()
{
	Super::BeginPlay();
	
}
//void AWeaponPickUp::Tick(float DeltaTime)
//{
//	Super::Tick(DeltaTime);
//}

void AWeaponPickUp::AttachToCharacter(ACharacterPlayer* Character)
{
	UE_LOG(LogTemp, Warning, TEXT("Weapon has been spwaned"));
}
AWeapon* AWeaponPickUp::SpawnWeapon(ACharacterPlayer* Character)
{
	AWeapon* NewWeapon = nullptr;
	if (WeaponClass != nullptr)
	{
		UWorld* const World = GetWorld();
		if (World != nullptr && Character != nullptr)
		{
			FActorSpawnParameters ActorSpawnParams;
			ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			NewWeapon = GetWorld()->SpawnActor<AWeapon>(WeaponClass, GetActorTransform(), ActorSpawnParams);
			NewWeapon->InitWeapon(Character);
		}
	}
	return NewWeapon;
}
void AWeaponPickUp::DestroyWeaponPickUp()
{
	Destroy();
}