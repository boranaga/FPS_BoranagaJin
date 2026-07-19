
#include "Characters/HealthComponent.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;
	bIsDead = false;

	OnHealthChanged.Broadcast(MaxHealth, CurrentHealth);
}

float UHealthComponent::ApplyDamage(float DamageAmount)
{
	if (bIsDead || DamageAmount <= 0.f)
	{
		return 0.f;
	}

	const float OldHealth = CurrentHealth;

	CurrentHealth = FMath::Clamp(CurrentHealth - DamageAmount, 0.f, MaxHealth);

	const float ActualDamage = OldHealth - CurrentHealth;

	OnHealthChanged.Broadcast(MaxHealth, CurrentHealth);

	if (CurrentHealth <= 0.f)
	{
		bIsDead = true;
		OnDeath.Broadcast();
	}

	return ActualDamage;
}

void UHealthComponent::Heal(float HealAmount)
{
	if (bIsDead || HealAmount <= 0.f)
	{
		return;
	}

	CurrentHealth = FMath::Clamp(CurrentHealth + HealAmount, 0.f, MaxHealth);

	OnHealthChanged.Broadcast(MaxHealth, CurrentHealth);
}

bool UHealthComponent::IsDead() const
{
	return bIsDead;
}

float UHealthComponent::GetCurrentHealth() const
{
	return CurrentHealth;
}

float UHealthComponent::GetMaxHealth() const
{
	return MaxHealth;
}

float UHealthComponent::GetHealthPercent() const
{
	return MaxHealth > 0.f ? CurrentHealth / MaxHealth : 0.f;
}

void UHealthComponent::ResetHealth()
{
	CurrentHealth = MaxHealth;
	bIsDead = false;

	OnHealthChanged.Broadcast(MaxHealth, CurrentHealth);
}