
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
	MaxHealth = FMath::IsFinite(MaxHealth) ? FMath::Max(1.f, MaxHealth) : 100.f;
	CurrentHealth = PendingInitialHealth.IsSet()
		? FMath::Clamp(PendingInitialHealth.GetValue(), 0.f, MaxHealth) : MaxHealth;
	PendingInitialHealth.Reset();
	bIsDead = CurrentHealth <= 0.f;
	bSettingsInitialized = true;

	if (AActor* OwnerActor = GetOwner())
	{
		BloodTrailComponent = OwnerActor->FindComponentByClass<UBloodTrailComponent>();
	}
	BroadcastHealthChanged();
}

void UHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	//UpdateBleedingState();
}

float UHealthComponent::ApplyDamage(float DamageAmount)
{
	if (bIsDead || !FMath::IsFinite(DamageAmount) || DamageAmount <= 0.f)
	{
		return 0.f;
	}

	const float OldHealth = CurrentHealth;

	const float NewHealth = FMath::Clamp(OldHealth - DamageAmount, 0.f, MaxHealth);
	const float ActualDamage = OldHealth - NewHealth;
	if (ActualDamage <= 0.f) { return 0.f; }
	CommitHealth(NewHealth);


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
	if (bIsDead || !FMath::IsFinite(HealAmount) || HealAmount <= 0.f) { return; }
	CommitHealth(CurrentHealth + HealAmount);
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

float UHealthComponent::WriteHealthSaveData() const
{
	return PendingInitialHealth.IsSet() ? PendingInitialHealth.GetValue() : CurrentHealth;
}

void UHealthComponent::LoadHealthSaveData(float SavedCurrentHealth)
{
	if (!FMath::IsFinite(SavedCurrentHealth))
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid saved health on %s"), *GetNameSafe(GetOwner()));
		return;
	}
	if (!bSettingsInitialized)
	{
		// BeginPlay applies this only after the configured maximum is loaded.
		PendingInitialHealth = SavedCurrentHealth;
		return;
	}
	// Refresh listeners even when HP is unchanged. Death is still a transition.
	CommitHealth(SavedCurrentHealth, true);
}

void UHealthComponent::SetCurrHealth(float NewHealthVal)
{
	if (!FMath::IsFinite(NewHealthVal)) { return; }
	if (!bSettingsInitialized)
	{
		PendingInitialHealth = NewHealthVal;
		return;
	}
	CommitHealth(NewHealthVal);
}

void UHealthComponent::ResetHealth()
{
	PendingInitialHealth.Reset();
	CommitHealth(MaxHealth);
}

void UHealthComponent::CommitHealth(float NewHealth, bool bForceNotify)
{
	const float OldHealth = CurrentHealth;
	const bool bWasDead = bIsDead;
	CurrentHealth = FMath::Clamp(NewHealth, 0.f, MaxHealth);
	bIsDead = CurrentHealth <= 0.f;
	const bool bBecameDead = !bWasDead && bIsDead;

	// Listeners always observe a consistent health/death state.
	if (bForceNotify || OldHealth != CurrentHealth) { BroadcastHealthChanged(); }
	if (bBecameDead && bIsDead) { OnDeath.Broadcast(); }
}

void UHealthComponent::BroadcastHealthChanged()
{
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

	MaxHealth = FMath::IsFinite(Row->Health.MaxHealth) ? FMath::Max(1.f, Row->Health.MaxHealth) : 100.f;
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
		if (PreviousMax != MaxHealth || PreviousCurrent != CurrentHealth)
		{
			BroadcastHealthChanged();
		}
	}
	return true;
}
