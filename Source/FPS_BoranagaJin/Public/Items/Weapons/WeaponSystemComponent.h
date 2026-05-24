// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WeaponName.h"
#include "Items/WeaponState/WeaponStateType.h"
#include "WeaponInterface.h"
#include "WeaponSystemComponent.generated.h"

//MEMO: 다른 방식?
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponPickedUp, EWeaponName, WeaponName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponSwitched, int32, PrevIndex, int32, NewIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkillWeaponEquipped, class AWeapon*, NewSkillWeapon);

class ACharacterPlayer;
class AWeapon;
class AWeaponPickUp;
class UInteractionWidget;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class FPS_BORANAGAJIN_API UWeaponSystemComponent : public UActorComponent, public IWeaponInterface
{
	GENERATED_BODY()
public:	
	UWeaponSystemComponent();

	UPROPERTY(BlueprintAssignable, Category = "Weapon")
	FOnWeaponPickedUp OnWeaponPickedUp;
	UPROPERTY(BlueprintAssignable, Category = "Weapon")
	FOnWeaponSwitched OnWeaponSwitched;
	UPROPERTY(BlueprintAssignable, Category = "Weapon")
	FOnSkillWeaponEquipped OnSkillWeaponEquipped;

protected:
	virtual void BeginPlay() override;
public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

#pragma region PlayerReference
private:
	void InitializePlayerReference();
#pragma endregion
#pragma region InputAction
private:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "WeaponSystem|PlayerReference", Meta = (AllowPrivateAccess = "true"))
	ACharacterPlayer* PlayerOwner;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "WeaponSystem|PlayerReference", Meta = (AllowPrivateAccess = "true"))
	class APlayerController* PlayerController;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputMappingContext* WeaponSystemMappingContext;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* InteractAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* SwitchWeaponUpAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* SwitchWeaponDownAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* SwitchWeapon1Action;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* SwitchWeapon2Action;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* SwitchWeapon3Action;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* LeftMouseButtonAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* RightMouseButtonAction;
#pragma endregion
#pragma region WeaponOwnership
protected:
	UPROPERTY(VisibleAnywhere)
	TMap<EWeaponName, bool> OwnedWeapons;
public:
	TMap<EWeaponName, bool> GetOwnerShipMap() const { return OwnedWeapons; }
public:
	//void UnlockWeapon(EWeaponName NewWeaponName);
protected:
	void LoadWSCData();
	//void InitStartingWeapons();
	//void InitStartingWeapons_Ordering();
	void InitStartingWeapons_New();
	void SaveInventory();
#pragma endregion
#pragma region SearchWeapon
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponSystem")
	float SearchWeaponRadius = 150.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponSystem")
	float SearchWeaponViewportRatio_Width = 0.7;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponSystem")
	float SearchWeaponViewportRatio_Height = 0.7;
	UPROPERTY()
	AWeaponPickUp* OverlappedWeapon;
protected:
	bool SearchWeapon();
	TTuple<FVector2D, bool> GetScreenPositionOfWorldLocation(const FVector& SearchLocation) const;
	bool IsInViewport(FVector2D ActorScreenPosition, float ScreenRatio_Width = 0.0f, float ScreenRatio_Height = 0.0f) const;
#pragma endregion
#pragma region Interaction
public:
	void PickUpWeapon();
	bool ObtainNewWeapon(AWeaponPickUp* NewWeaponPickUp);
	bool ObtainAmmo(AWeaponPickUp* MagazinePickUp);
#pragma endregion
#pragma region Zoom
protected:
	bool bIsZoomIn = false;
public:
	bool IsZoomIn() const { return bIsZoomIn; }
	virtual void ZoomIn(bool bZoomIn) override;
	bool IsWeaponModifyingCamFov();
#pragma endregion
#pragma region Aiming
protected:
	FVector ScreenCenterWorldLocation;

	FVector ScreenCenterWorldDirection;

	FVector RightHandToAimSocketOffset;

	FVector TargetRightHandWorldLocation;
public:
	virtual void SetRightHandToAimSocketOffset(FVector offset) override;

	FVector GetScreenCenterWorldPosition() const { return ScreenCenterWorldLocation; }

	FVector GetScreenCenterWorldDirection() const { return ScreenCenterWorldDirection; }

	FVector CalculateScreenCenterWorldPositionAndDirection(FVector& OutWorldPosition, FVector& OutWorldDirection) const;

	FVector GetTargetRightHandWorldLocation() const { return TargetRightHandWorldLocation; }

	FVector CalculateTargetRightHandPosition();

	FTransform GetWeaponAimSocketRelativeTransform();
#pragma endregion
#pragma region SwitchWeapon
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapons")
	TArray<AWeapon*> WeaponInventory;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	AWeapon* CurrentWeapon = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	int32 CurrentWeaponIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkillWeapons")
	TArray<AWeapon*> SkillWeaponInventory;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SkillWeapon")
	AWeapon* CurrentSkillWeapon = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SkillWeapon")
	int32 CurrentSkillWeaponIndex = 0;

public:
	AWeapon* GetCurrentWeapon() { return CurrentWeapon; }
	AWeapon* GetCurrentSkillWeapon() { return CurrentSkillWeapon; }
	int32 GetWeaponNum() { return WeaponInventory.Num(); }
	int32 GetSkillWeaponNum() { return SkillWeaponInventory.Num(); }
	bool IsCurrentSkillWeaponTargeting();
	EWeaponStateType GetCurrWeaponStateType() const;

	void SwitchToPreviousWeapon();
	void SwitchToNextWeapon();
	void SwitchToIndex(int32 idx);

	virtual void SwitchToOtherWeapon() override;
	void ChangeWeapon(int32 WeaponIndex);
	void AddNewWeaponToInventory(AWeapon* NewWeapon);
#pragma endregion

#pragma region Control
protected:
	UPROPERTY()
	AWeapon* ControllingWeapon = nullptr;
public:
	bool TryTakeControl(AWeapon* NewWeapon);
	void ReleaseControl();
#pragma endregion

#pragma region InteractionUI
protected:
	UPROPERTY(EditAnywhere, BlueprintreadWrite, Category = "InteractionWidget")
	TSubclassOf<UInteractionWidget> InteractionWidgetClass;
	UPROPERTY()
	UInteractionWidget* InteractionWidget;

	void InitInteractionUI();
	void UpdateInteractionUI(bool bflag = false, FVector location = FVector::ZeroVector);
#pragma endregion
};
