#include "Characters/StaminaComponent.h"
#include "Characters/CharacterComponentSettingsSource.h"
#include "Characters/CharacterComponentSettings.h"
#include "Engine/World.h"

UStaminaComponent::UStaminaComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    // UPlayerMovementComponent ticks during physics; consume the updated running flag afterward.
    PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UStaminaComponent::BeginPlay()
{
    Super::BeginPlay();
    ReloadSettingsFromOwnerDataTable();
    MaxStamina = FMath::IsFinite(MaxStamina) ? FMath::Max(1.f, MaxStamina) : 1000.f;
    MinimumRunStamina = FMath::Clamp(MinimumRunStamina, 0.f, MaxStamina);
    CurrentStamina = MaxStamina;
    bSettingsInitialized = true;
    bContinuousRunningDrain = false;
    bRecoveryEnabled = true;
    LastConsumeTime = -999.f;
    BroadcastStaminaChanged();
}

void UStaminaComponent::TickComponent(
    float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!GetWorld() || !FMath::IsFinite(DeltaTime) || DeltaTime <= 0.f) return;

    if (bContinuousRunningDrain)
    {
        // Sprint drain and recovery are mutually exclusive, including RecoveryDelay=0.
        if (RunStaminaCostPerSecond > 0.f)
        {
            LastConsumeTime = GetWorld()->GetTimeSeconds();
            if (CurrentStamina > 0.f)
            {
                SpendStamina(RunStaminaCostPerSecond * DeltaTime);
            }
        }
        return;
    }

    if (bRecoveryEnabled)
    {
        RecoverStamina(DeltaTime);
    }
}

bool UStaminaComponent::CanConsume(float Amount) const
{
    return FMath::IsFinite(Amount) && Amount > 0.f && CurrentStamina + KINDA_SMALL_NUMBER >= Amount;
}

bool UStaminaComponent::ConsumeStamina(float Amount)
{
    if (!CanConsume(Amount)) return false;
    return SpendStamina(Amount);
}

bool UStaminaComponent::SpendStamina(float Amount)
{
    if (!FMath::IsFinite(Amount) || Amount <= 0.f || !GetWorld() || CurrentStamina <= 0.f) return false;
    const float Old = CurrentStamina;
    CurrentStamina = FMath::Clamp(CurrentStamina - Amount, 0.f, MaxStamina);
    LastConsumeTime = GetWorld()->GetTimeSeconds();
    if (CurrentStamina == Old) return false;
    const bool bBecameEmpty = Old > 0.f && CurrentStamina <= 0.f;
    BroadcastStaminaChanged();
    if (bBecameEmpty && CurrentStamina <= 0.f)
    {
        OnStaminaEmpty.Broadcast();
    }
    return true;
}

void UStaminaComponent::BroadcastStaminaChanged()
{
    // Native delegate uses (Current, Max). UI adapts to its legacy (Max, Current) API.
    OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
}

bool UStaminaComponent::CanRun() const
{
    return CurrentStamina > KINDA_SMALL_NUMBER &&
        CurrentStamina + KINDA_SMALL_NUMBER >= MinimumRunStamina;
}

bool UStaminaComponent::CanDash() const
{
    return DashStaminaCost <= 0.f || CanConsume(DashStaminaCost);
}

bool UStaminaComponent::TryConsumeDashStamina()
{
    if (!CanDash()) return false;
    if (DashStaminaCost <= 0.f) return true;
    return ConsumeStamina(DashStaminaCost);
}

void UStaminaComponent::SetRunningDrainEnabled(bool bEnabled)
{
    bContinuousRunningDrain = bEnabled;
}

void UStaminaComponent::SetRecoveryEnabled(bool bEnabled)
{
    bRecoveryEnabled = bEnabled;
}

void UStaminaComponent::RecoverStamina(float DeltaTime)
{
    if (!GetWorld() || !bRecoveryEnabled || bContinuousRunningDrain || !FMath::IsFinite(DeltaTime) || DeltaTime <= 0.f)
        return;
    if (GetWorld()->GetTimeSeconds() - LastConsumeTime < RecoveryDelay)
        return;
    if (CurrentStamina >= MaxStamina || RecoveryRate <= 0.f)
        return;
    const float Old = CurrentStamina;
    CurrentStamina = FMath::Clamp(CurrentStamina + RecoveryRate * DeltaTime, 0.f, MaxStamina);
    if (CurrentStamina != Old) BroadcastStaminaChanged();
}

void UStaminaComponent::ResetStamina()
{
    const float Previous = CurrentStamina;
    CurrentStamina = MaxStamina;
    LastConsumeTime = -999.f;
    bContinuousRunningDrain = false;
    bRecoveryEnabled = true;
    if (Previous != CurrentStamina) BroadcastStaminaChanged();
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
    MaxStamina = FMath::IsFinite(Row->Stamina.MaxStamina) ? FMath::Max(1.f, Row->Stamina.MaxStamina) : 1000.f;
    RecoveryRate = FMath::Max(0.f, Row->Stamina.RecoveryRate);
    RecoveryDelay = FMath::Max(0.f, Row->Stamina.RecoveryDelay);
    RunStaminaCostPerSecond = FMath::Max(0.f, Row->Stamina.RunStaminaCostPerSecond);
    MinimumRunStamina = FMath::Clamp(Row->Stamina.MinimumRunStamina, 0.f, MaxStamina);
    DashStaminaCost = FMath::Max(0.f, Row->Stamina.DashStaminaCost);

    if (bSettingsInitialized)
    {
        const float Ratio = PreviousMax > KINDA_SMALL_NUMBER
            ? PreviousCurrent / PreviousMax : 0.f;
        CurrentStamina = FMath::Clamp(Ratio * MaxStamina, 0.f, MaxStamina);
        if (PreviousMax != MaxStamina || PreviousCurrent != CurrentStamina)
        {
            BroadcastStaminaChanged();
        }
    }
    return true;
}
