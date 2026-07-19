#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StaminaComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnStaminaChanged, float currentStamina, float maxStamina);
DECLARE_MULTICAST_DELEGATE(FOnStaminaEmpty);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FPS_BORANAGAJIN_API UStaminaComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UStaminaComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction
	) override;

private:
	UPROPERTY(EditAnywhere, Category = "Stamina")
	float MaxStamina = 1000.f;
	
	UPROPERTY(VisibleAnywhere, Category = "Stamina")
	float CurrentStamina = 1000.f;

	UPROPERTY(EditAnywhere, Category = "Stamina")
	float RecoveryRate = 50.f;

	UPROPERTY(EditAnywhere, Category = "Stamina")
	float RecoveryDelay = 1.f;

	float LastConsumeTime = -999.f;

public:
	FOnStaminaChanged OnStaminaChanged;
	FOnStaminaEmpty OnStaminaEmpty;

	bool CanConsume(float Amount) const;
	bool ConsumeStamina(float Amount);

	void RecoverStamina(float DeltaTime);
	void ResetStamina();

	bool IsExhausted() const;

	float GetCurrentStamina() const;
	float GetMaxStamina() const;
	float GetStaminaPercent() const;
};