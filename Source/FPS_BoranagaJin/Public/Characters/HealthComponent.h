#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Misc/Optional.h"
#include "HealthComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, float MaxHealth, float CurrentHealth);
DECLARE_MULTICAST_DELEGATE(FOnDeath);

class UBloodTrailComponent;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FPS_BORANAGAJIN_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UHealthComponent();
protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBloodTrailComponent> BloodTrailComponent;

	UPROPERTY(EditAnywhere, Category = "Health")
	float MaxHealth = 100.f;

	UPROPERTY(VisibleAnywhere, Category = "Health")
	float CurrentHealth = 100.f;

	UPROPERTY(EditAnywhere, Category = "Blood Trail", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BleedingHealthThresholdPercent = 0.6f;

	UPROPERTY(EditAnywhere, Category = "Blood Trail")
	float HeavyDamageThreshold = 25.f;

	UPROPERTY(EditAnywhere, Category = "Blood Trail")
	float BleedingDurationAfterHeavyDamage = 8.f;

	//bool bIsBleeding = false;
	bool bIsDead = false;
	bool bSettingsInitialized = false;
	// An explicit save restore before BeginPlay is applied after the DT maximum is known.
	TOptional<float> PendingInitialHealth;
public:
	// Reads only the Owner actor's assigned row (if it implements the interface).
	UFUNCTION(BlueprintCallable, Category = "Health - Configuration")
	bool ReloadSettingsFromOwnerDataTable();

	FOnHealthChanged OnHealthChanged;
	FOnDeath OnDeath;

	float ApplyDamage(float DamageAmount);
	void Heal(float HealAmount);
	void StopBleeding(float HealAmount = 0.f);
	bool IsDead() const;
	bool IsBleeding() const;
	float GetCurrentHealth() const;
	float GetMaxHealth() const;
	float GetHealthPercent() const;

	// Save the gameplay HP, not the DT configuration or derived death flag.
	float WriteHealthSaveData() const;
	void LoadHealthSaveData(float SavedCurrentHealth);

	void SetCurrHealth(float NewHealthVal);
	void ResetHealth();
private:
	void CommitHealth(float NewHealth, bool bForceNotify = false);
	void BroadcastHealthChanged();
};
