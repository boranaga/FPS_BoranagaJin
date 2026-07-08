#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, float currenthealth, float maxhealth);
DECLARE_MULTICAST_DELEGATE(FOnDeath);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FPS_BORANAGAJIN_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHealthComponent();

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditAnywhere, Category = "Health")
	float MaxHealth = 100.f;

	UPROPERTY(VisibleAnywhere, Category = "Health")
	float CurrentHealth = 100.f;

	bool bIsDead = false;

public:
	FOnHealthChanged OnHealthChanged;
	FOnDeath OnDeath;

	float ApplyDamage(float DamageAmount);
	void Heal(float HealAmount);

	bool IsDead() const;
	float GetCurrentHealth() const;
	float GetMaxHealth() const;
	float GetHealthPercent() const;

	void ResetHealth();
};