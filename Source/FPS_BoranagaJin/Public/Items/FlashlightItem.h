#pragma once

#include "CoreMinimal.h"
#include "Items/Item.h"
#include "FlashlightItem.generated.h"

class USpotLightComponent;
class ABatteryItem;

UCLASS()
class FPS_BORANAGAJIN_API AFlashlightItem : public AItem
{
	GENERATED_BODY()
public:
	AFlashlightItem();
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
public:
	virtual void InitItem(ACharacterPlayer* NewCharacter, AItemPickUp* PickUpActor = nullptr) override;
	/**
	 * 손전등 사용.
	 * 현재 구현에서는 UseItem 호출 시 전원을 토글한다.
	 */
	virtual bool UseItem(ACharacterPlayer* UsingCharacter) override;

	void Equip(ACharacterPlayer* TargetCharacter);
	void Unequip();
protected:
	void OnEquipEnded(ACharacterPlayer* UsingCharacter);
	void OnUnequipEnded();
	bool AttachWeaponToPlayer(ACharacterPlayer* TargetCharacter);
	void DetachFromPlayer();
	void StartAnimation(UAnimMontage* CharacterAnimation, UAnimMontage* WeaponAnimation, float CharacterAnimPlayRate, FName StartSection = FName());

protected:
protected:
	UPROPERTY()
	UAnimInstance* CharacterAnimInstance = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|Character")
	UAnimMontage* AM_Equip_Character = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|Character")
	UAnimMontage* AM_Unequip_Character = nullptr;
	UPROPERTY(EditAnywhere)
	float SwitchingRate = 1.f;
	FTimerHandle SwitchingTimer;
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
	UStaticMeshComponent* ItemMesh;
#pragma region FlashlightPower
public:
	UFUNCTION(BlueprintCallable, Category = "Flashlight")
	void ToggleFlashlight();
	UFUNCTION(BlueprintCallable, Category = "Flashlight")
	bool TurnOnFlashlight();
	UFUNCTION(BlueprintCallable, Category = "Flashlight")
	void TurnOffFlashlight();
	UFUNCTION(BlueprintPure, Category = "Flashlight")
	bool IsFlashlightOn() const { return bIsFlashlightOn; }
private:
	void SetLightEnabled(bool bEnabled);
#pragma endregion

#pragma region Battery
public:
	/**
	 * 배터리 아이템을 사용하여 배터리를 교체한다.
	 * 성공하면 true를 반환한다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Flashlight|Battery")
	bool ReplaceBattery(ABatteryItem* BatteryItem);
	/**
	 * 인벤토리 시스템에서 배터리 액터를 직접 전달하기 어려울 때 사용한다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Flashlight|Battery")
	bool ReplaceBatteryByCharge(float NewBatteryCharge);
	UFUNCTION(BlueprintPure, Category = "Flashlight|Battery")
	float GetCurrentBatteryCharge() const { return CurrentBatteryCharge; }
	UFUNCTION(BlueprintPure, Category = "Flashlight|Battery")
	float GetMaxBatteryCharge() const { return MaxBatteryCharge; }
	UFUNCTION(BlueprintPure, Category = "Flashlight|Battery")
	float GetBatteryPercent() const;
	UFUNCTION(BlueprintPure, Category = "Flashlight|Battery")
	bool HasBatteryCharge() const { return CurrentBatteryCharge > UE_KINDA_SMALL_NUMBER; }
protected:
	void StartBatteryDrain();
	void StopBatteryDrain();
	UFUNCTION()
	void DrainBattery();
	UFUNCTION()
	void HandleBatteryDepleted();
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flashlight|Battery", meta = (ClampMin = "0.0"))
	float MaxBatteryCharge = 100.0f;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Flashlight|Battery", meta = (ClampMin = "0.0"))
	float CurrentBatteryCharge = 100.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flashlight|Battery", meta = (ClampMin = "0.0"))
	float BatteryDrainAmount = 1.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flashlight|Battery", meta = (ClampMin = "0.01"))
	float BatteryDrainInterval = 1.0f;
private:
	FTimerHandle BatteryDrainTimerHandle;
#pragma endregion
#pragma region DamageAndFlicker
public:
	/**
	 * 손전등에 피해를 적용한다.
	 * 프로젝트의 DamageInterface에서 이 함수를 호출하도록 연결할 수 있다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Flashlight|Damage")
	void ApplyFlashlightDamage(float DamageAmount);
	UFUNCTION(BlueprintCallable, Category = "Flashlight|Damage")
	void BreakFlashlight();
	UFUNCTION(BlueprintCallable, Category = "Flashlight|Damage")
	void RepairFlashlight();
	UFUNCTION(BlueprintPure, Category = "Flashlight|Damage")
	bool IsBroken() const { return bIsBroken; }
protected:
	void StartFlickering();
	void StopFlickering();
	UFUNCTION()
	void UpdateFlicker();
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flashlight|Damage", meta = (ClampMin = "0.0"))
	float MaxDurability = 100.0f;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Flashlight|Damage", meta = (ClampMin = "0.0"))
	float CurrentDurability = 100.0f;
	/**
	 * 고장 상태에서 깜빡임 상태를 다시 계산하는 최소 간격.
	 * 실제 다음 간격은 Min~Max 사이에서 무작위로 결정된다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flashlight|Damage", meta = (ClampMin = "0.01"))
	float MinFlickerInterval = 0.05f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flashlight|Damage", meta = (ClampMin = "0.01"))
	float MaxFlickerInterval = 0.25f;
	/**
	 * 깜빡임 갱신 시 불빛이 켜질 확률.
	 * 예: 0.65면 약 65% 확률로 켜진 상태가 된다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flashlight|Damage", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FlickerOnProbability = 0.65f;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Flashlight|Damage")
	bool bIsBroken = false;
private:
	void ScheduleNextFlicker();
	FTimerHandle FlickerTimerHandle;
#pragma endregion

#pragma region Components
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flashlight")
	TObjectPtr<USpotLightComponent> FlashlightLight;
#pragma endregion
private:
	/**
	 * 플레이어가 손전등 전원을 켜 놓았는지를 나타낸다.
	 * 고장으로 실제 조명이 잠시 꺼져도 이 값은 true일 수 있다.
	 */
	UPROPERTY(VisibleInstanceOnly, Category = "Flashlight")
	bool bIsFlashlightOn = false;

#pragma region Animation
protected:
	UPROPERTY(EditAnywhere, Category = "Socket")
	FTransform RightHandSocketTransform;
	UPROPERTY(EditAnywhere, Category = "Socket")
	FTransform RightHandSocketTransform_Crouch;
public:
	FTransform GetRightHandSocketTransform() const { return RightHandSocketTransform; }
	FTransform GetRightHandSocketTransform_Crouch() const { return RightHandSocketTransform_Crouch; }
#pragma endregion
};