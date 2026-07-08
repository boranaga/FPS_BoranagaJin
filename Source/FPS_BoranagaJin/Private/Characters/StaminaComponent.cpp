#include "Characters/StaminaComponent.h"

UStaminaComponent::UStaminaComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UStaminaComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentStamina = MaxStamina;
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