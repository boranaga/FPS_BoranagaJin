// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponName.h"
//#include "InteractInterface.h" //TODO: 안사용하는 듯?

#include "WeaponPickUp.generated.h"

class AWeapon;
class ACharacterPlayer;
class UPickUpComponent;

UCLASS()
class FPS_BORANAGAJIN_API AWeaponPickUp : public AActor
{
	GENERATED_BODY()
public:	
	AWeaponPickUp();
protected:
	virtual void BeginPlay() override;
//public:	
	//virtual void Tick(float DeltaTime) override;
public:
	UFUNCTION()
	void AttachToCharacter(ACharacterPlayer* Character);
	UFUNCTION()
	AWeapon* SpawnWeapon(ACharacterPlayer* Character);
	void DestroyWeaponPickUp();
protected:
	UPROPERTY(EditDefaultsOnly, Category = Weapon)
	TSubclassOf<AWeapon> WeaponClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Weapon, meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* WeaponMesh = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EWeaponName WeaponName;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Weapon, meta = (AllowPrivateAccess = "true")) //MEMO: 필요한가???
	UPickUpComponent* PickUpComponent = nullptr;
protected:
	UPROPERTY(EditAnywhere)
	bool bIsMagazine = false;
	UPROPERTY(EditAnywhere)
	int32 NumAmmo = 10;
public:
	EWeaponName GetWeaponName() const { return WeaponName; }
	bool IsMagazine() const { return bIsMagazine; }
	int32 GetAmmo() const { return NumAmmo; }
};
