
#include "Characters/HealthComponent.h"
#include "Characters/BloodTrailComponent.h"
#include "Characters/CharacterComponentSettingsSource.h"
#include "Characters/CharacterComponentSettings.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	ReloadSettingsFromOwnerDataTable();
	CurrentHealth = MaxHealth;
	bIsDead = false;
	bSettingsInitialized = true;

	OnHealthChanged.Broadcast(MaxHealth, CurrentHealth);

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor) { return; }
	BloodTrailComponent = OwnerActor->FindComponentByClass<UBloodTrailComponent>();
}

void UHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	//UpdateBleedingState();
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


	if (GetWorld() && IsValid(BloodTrailComponent) && !BloodTrailComponent->IsBleeding())
	{
		const bool bHeavyDamage = ActualDamage >= HeavyDamageThreshold;
		const bool bLowHealth = GetHealthPercent() <= BleedingHealthThresholdPercent;

		if (!bHeavyDamage && !bLowHealth)
		{
			return ActualDamage;
		}

		if (!bLowHealth && bHeavyDamage)
		{
			BloodTrailComponent->StartBleeding(false, BleedingDurationAfterHeavyDamage);
			return ActualDamage;
		}

		BloodTrailComponent->StartBleeding();

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

void UHealthComponent::StopBleeding(float HealAmount)
{
	if (bIsDead) { return; }
	if (HealAmount > 0.f) { Heal(HealAmount); }
	if (!BloodTrailComponent) { return; }
	BloodTrailComponent->StopBleeding();
}

bool UHealthComponent::IsDead() const
{
	return bIsDead;
}

bool UHealthComponent::IsBleeding() const
{
	if (!BloodTrailComponent) { return false; }
	return BloodTrailComponent->IsBleeding();
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

void UHealthComponent::SetCurrHealth(float NewHealthVal)
{
	CurrentHealth = FMath::Clamp(NewHealthVal, 0.f, MaxHealth);
	OnHealthChanged.Broadcast(MaxHealth, CurrentHealth);
}

void UHealthComponent::ResetHealth()
{
	CurrentHealth = MaxHealth;
	bIsDead = false;

	OnHealthChanged.Broadcast(MaxHealth, CurrentHealth);
}

//void UHealthComponent::UpdateBleedingState()
//{
//	if (!BloodTrailComponent->IsBleeding() || !GetWorld()) { return; }
//	if (bIsDead)
//	{
//		BloodTrailComponent->StopBleeding();
//		return;
//	}
//
//	const bool bStillLowHealth = GetHealthPercent() <= BleedingHealthThresholdPercent;
//	const bool bHeavyDamageBleedingActive = GetWorld()->GetTimeSeconds() < BleedingEndTime;
//
//	if (!bStillLowHealth && !bHeavyDamageBleedingActive)
//	{
//		StopBleeding();
//	}
//
//}

// Optional explicit reload. Preserves health percentage after gameplay has begun.
bool UHealthComponent::ReloadSettingsFromOwnerDataTable()
{
	const FCharacterComponentSettingsRow* Row =
		CharacterComponentSettings::Resolve(GetOwner(), TEXT("HealthComponent"));
	if (!Row) return false;

	const float PreviousMax = MaxHealth;
	const float PreviousCurrent = CurrentHealth;
	const bool bWasDead = bIsDead;

	MaxHealth = FMath::Max(1.f, Row->Health.MaxHealth);
	BleedingHealthThresholdPercent = FMath::Clamp(
		Row->Health.BleedingHealthThresholdPercent, 0.f, 1.f);
	HeavyDamageThreshold = FMath::Max(0.f, Row->Health.HeavyDamageThreshold);
	BleedingDurationAfterHeavyDamage = FMath::Max(
		0.f, Row->Health.BleedingDurationAfterHeavyDamage);

	if (bSettingsInitialized)
	{
		// Do not heal a dead actor during DataTable hot reload.
		const float PreviousRatio = PreviousMax > KINDA_SMALL_NUMBER
			? PreviousCurrent / PreviousMax : 0.f;
		CurrentHealth = bWasDead ? 0.f : FMath::Clamp(PreviousRatio * MaxHealth, 0.f, MaxHealth);
		// Preserve the delegate's argument ordering used in the uploaded code.
		OnHealthChanged.Broadcast(MaxHealth, CurrentHealth);
	}
	return true;
}
