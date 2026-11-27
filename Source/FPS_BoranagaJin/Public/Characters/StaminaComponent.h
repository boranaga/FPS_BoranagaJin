#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StaminaComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnStaminaChanged, float CurrentStamina, float MaxStamina);
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

    UPROPERTY(EditAnywhere, Category = "Stamina - Running", meta = (ClampMin = "0.0"))
    float RunStaminaCostPerSecond = 50.f;

    UPROPERTY(EditAnywhere, Category = "Stamina - Running", meta = (ClampMin = "0.0"))
    float MinimumRunStamina = 50.f;

    UPROPERTY(EditAnywhere, Category = "Stamina - Dash", meta = (ClampMin = "0.0"))
    float DashStaminaCost = 100.f;

    // Runtime activity is not a second stamina pool and does not belong in the DT.
    bool bContinuousRunningDrain = false;
    bool bRecoveryEnabled = true;

    float LastConsumeTime = -999.f;
    bool bSettingsInitialized = false;

public:
    UFUNCTION(BlueprintCallable, Category = "Stamina - Configuration")
    bool ReloadSettingsFromOwnerDataTable();

    FOnStaminaChanged OnStaminaChanged;
    FOnStaminaEmpty OnStaminaEmpty;

    UFUNCTION(BlueprintPure, Category = "Stamina - Running")
    bool CanRun() const;

    UFUNCTION(BlueprintPure, Category = "Stamina - Dash")
    bool CanDash() const;

    UFUNCTION(BlueprintCallable, Category = "Stamina - Dash")
    bool TryConsumeDashStamina();

    /** Called by movement after state/input processing. StaminaComponent ticks after movement. */
    void SetRunningDrainEnabled(bool bEnabled);
    void SetRecoveryEnabled(bool bEnabled);

    UFUNCTION(BlueprintPure, Category = "Stamina - Dash")
    float GetDashStaminaCost() const { return DashStaminaCost; }

    UFUNCTION(BlueprintPure, Category = "Stamina - Running")
    float GetRunStaminaCostPerSecond() const { return RunStaminaCostPerSecond; }

    bool CanConsume(float Amount) const;
    bool ConsumeStamina(float Amount);

    void RecoverStamina(float DeltaTime);
    void ResetStamina();

    bool IsExhausted() const;

    float GetCurrentStamina() const;
    float GetMaxStamina() const;
    float GetStaminaPercent() const;

private:
    bool SpendStamina(float Amount);
    void BroadcastStaminaChanged();
};