#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/SaveableActorInterface.h"
#include "GasRoomVolume.generated.h"

class UBoxComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class UPostProcessComponent;
class APawn;

/** A well-mixed, independently saved gas compartment. Place one per room. */
UCLASS()
class FPS_BORANAGAJIN_API AGasRoomVolume : public AActor, public ISaveableActorInterface
{
    GENERATED_BODY()
public:
    AGasRoomVolume();
    virtual FGuid GetInstanceID() const override { return InstanceID; }
    virtual bool ShouldSaveTransform() const override { return true; }
    virtual bool IsRuntimeSpawned() const override { return bRuntimeSpawned; }
    virtual void SetRuntimeSpawned(bool bValue) override;
    virtual void OnBeforeSave() override;
    virtual void OnAfterLoad() override;
    virtual void PostActorCreated() override;
#if WITH_EDITOR
    virtual void PostEditImport() override;
    virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode) override;
#endif
    UFUNCTION(BlueprintPure, Category="Gas|Room")
    float GetGasConcentration() const { return Concentration; }
    UFUNCTION(BlueprintCallable, Category="Gas|Room")
    void SetVentilationRate(float NewRate);
    UFUNCTION(BlueprintCallable, Category="Gas|Room")
    void PurgeGas();
    // Amount is in abstract gameplay volume units (m^3 equivalent).
    void AddGas(float Amount);
    bool ContainsPoint(const FVector& Point) const;
    float GetRoomVolume() const;
    float GetOpticalDepth(const FVector& Start, const FVector& End) const;
    void Simulate(float DeltaSeconds);
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Gas|Components")
    TObjectPtr<UBoxComponent> RoomBounds;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Gas|Components")
    TObjectPtr<UNiagaraComponent> RoomFog;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Gas|Components")
    TObjectPtr<UPostProcessComponent> GasPostProcess;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gas|Visual")
    TObjectPtr<UNiagaraSystem> RoomFogSystem;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gas|Visual", meta=(ClampMin="0.0", ClampMax="1.0"))
    float MaxPostProcessWeight = 0.65f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gas|Vision", meta=(ClampMin="0.0"))
    float ExtinctionPerMeter = 2.5f;
    UPROPERTY(EditAnywhere, SaveGame, BlueprintReadOnly, Category="Gas|Room", meta=(ClampMin="0.0"))
    float VentilationRate = 0.015f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gas|Damage", meta=(ClampMin="0.0", ClampMax="1.0"))
    float ToxicThreshold = 0.15f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gas|Damage", meta=(ClampMin="0.0"))
    float ExposureGraceSeconds = 2.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gas|Damage", meta=(ClampMin="0.0"))
    float MaxDamagePerSecond = 12.f;
    UPROPERTY(VisibleInstanceOnly, SaveGame, BlueprintReadOnly, Category="Gas|Room")
    float Concentration = 0.f;
private:
    UPROPERTY(EditInstanceOnly, Category="Save")
    FGuid InstanceID;
    bool bRuntimeSpawned = false;
    // Save only stable victim keys; transient references never enter the archive.
    UPROPERTY(SaveGame)
    TMap<FString, float> ExposureSeconds;
    float PendingGas = 0.f;
    void UpdateVisuals();
    void UpdateExposure(float DeltaSeconds);
    FString GetVictimKey(const APawn* Pawn) const;
};
