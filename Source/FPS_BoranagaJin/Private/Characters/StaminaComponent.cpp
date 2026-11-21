#include "Characters/StaminaComponent.h"
#include "Characters/CharacterComponentSettingsSource.h"
#include "Characters/CharacterComponentSettings.h"

UStaminaComponent::UStaminaComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UStaminaComponent::BeginPlay()
{
	Super::BeginPlay();

	ReloadSettingsFromOwnerDataTable();
	CurrentStamina = MaxStamina;
	bSettingsInitialized = true;
}

void UStaminaComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction
)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	RecoverStamina(DeltaTime);
}

bool UStaminaComponent::CanConsume(float Amount) const
{
	return Amount > 0.f && CurrentStamina >= Amount;
}

bool UStaminaComponent::ConsumeStamina(float Amount)
{
	if (!CanConsume(Amount))
	{
		return false;
	}

	CurrentStamina = FMath::Clamp(CurrentStamina - Amount, 0.f, MaxStamina);
	LastConsumeTime = GetWorld()->GetTimeSeconds();

	OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);

	if (CurrentStamina <= 0.f)
	{
		OnStaminaEmpty.Broadcast();
	}

	return true;
}

void UStaminaComponent::RecoverStamina(float DeltaTime)
{
	if (!GetWorld()) return;

	const float CurrentTime = GetWorld()->GetTimeSeconds();

	if (CurrentTime - LastConsumeTime < RecoveryDelay)
	{
		return;
	}

	if (CurrentStamina >= MaxStamina)
	{
		return;
	}

	CurrentStamina = FMath::Clamp(
		CurrentStamina + RecoveryRate * DeltaTime,
		0.f,
		MaxStamina
	);

	OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
}

void UStaminaComponent::ResetStamina()
{
	CurrentStamina = MaxStamina;
	LastConsumeTime = -999.f;

	OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
}

bool UStaminaComponent::IsExhausted() const
{
	return CurrentStamina <= 0.f;
}

float UStaminaComponent::GetCurrentStamina() const
{
	return CurrentStamina;
}

float UStaminaComponent::GetMaxStamina() const
{
	return MaxStamina;
}

float UStaminaComponent::GetStaminaPercent() const
{
	return MaxStamina > 0.f ? CurrentStamina / MaxStamina : 0.f;
}
bool UStaminaComponent::ReloadSettingsFromOwnerDataTable()
{
	const FCharacterComponentSettingsRow* Row =
		CharacterComponentSettings::Resolve(GetOwner(), TEXT("StaminaComponent"));
	if (!Row) return false;

	const float PreviousMax = MaxStamina;
	const float PreviousCurrent = CurrentStamina;

	MaxStamina = FMath::Max(1.f, Row->Stamina.MaxStamina);
	RecoveryRate = FMath::Max(0.f, Row->Stamina.RecoveryRate);
	RecoveryDelay = FMath::Max(0.f, Row->Stamina.RecoveryDelay);

	if (bSettingsInitialized)
	{
		const float Ratio = PreviousMax > KINDA_SMALL_NUMBER
			? PreviousCurrent / PreviousMax : 0.f;
		CurrentStamina = FMath::Clamp(Ratio * MaxStamina, 0.f, MaxStamina);
		OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
	}
	return true;
}
