#pragma once

#include "CoreMinimal.h"
#include "Objects/DestructibleObject.h"
#include "GasPipe.generated.h"

class AGasRoomVolume;
class UDecalComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UNiagaraComponent;
class UNiagaraSystem;
class UAudioComponent;
class USoundBase;

USTRUCT(BlueprintType)
struct FPS_BORANAGAJIN_API FGasPipeHole
{
    GENERATED_BODY()
    // Store only mesh-local state; world coordinates are reconstructed on load.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category="Gas")
    FVector LocalPoint = FVector::ZeroVector;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category="Gas")
    FVector LocalNormal = FVector::ForwardVector;
};

/** Destructible pressurized environmental pipe, independent from the room gas. */
UCLASS()
class FPS_BORANAGAJIN_API AGasPipe : public ADestructibleObject
{
    GENERATED_BODY()
public:
    AGasPipe();
    virtual float ReceiveDamage(const FDamageParams& DamageData) override;
    virtual void RestoreDestructibleObject() override;
    virtual void OnBeforeSave() override;
    virtual void OnAfterLoad() override;
    UFUNCTION(BlueprintCallable, Category="Gas|Supply")
    void SetGasSupplyEnabled(bool bEnabled);
    UFUNCTION(BlueprintPure, Category="Gas|Supply")
    bool IsGasSupplyEnabled() const { return bGasSupplyEnabled; }
    UFUNCTION(BlueprintPure, Category="Gas|Holes")
    int32 GetHoleCount() const { return Holes.Num(); }
    // Use for callers with a genuine FHitResult. Does not apply damage a second time.
    float ReceiveDamageAtHit(const FDamageParams& DamageData, const FHitResult& Hit);
    void Simulate(float DeltaSeconds);
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void BreakObject(const FVector& HitLocation, const FVector& HitDirection) override;
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Gas|Room")
    TObjectPtr<AGasRoomVolume> TargetRoom;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gas|Holes", meta=(ClampMin="1", ClampMax="32"))
    int32 MaxHoles = 8;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gas|Holes", meta=(ClampMin="0.0"))
    float HoleMergeDistance = 3.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gas|Holes", meta=(ClampMin="0.0"))
    float MinimumPunctureDamage = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gas|Holes", meta=(ClampMin="0.1"))
    float HoleRadius = 3.f;
    UPROPERTY(EditAnywhere, SaveGame, BlueprintReadOnly, Category="Gas|Supply")
    bool bGasSupplyEnabled = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gas|Supply", meta=(ClampMin="0.0"))
    float GasFlowPerHole = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gas|Jet", meta=(ClampMin="1.0"))
    float JetLength = 250.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gas|Jet", meta=(ClampMin="1.0", ClampMax="85.0"))
    float JetHalfAngle = 13.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gas|Jet", meta=(ClampMin="0.0"))
    float JetSpeed = 1800.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gas|Jet", meta=(ClampMin="0.0"))
    float JetDamagePerSecond = 25.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gas|Jet")
    TEnumAsByte<ECollisionChannel> JetOcclusionChannel = ECC_Visibility;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gas|Visual")
    TObjectPtr<UNiagaraSystem> JetEffect;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gas|Visual")
    TObjectPtr<UMaterialInterface> HoleDecalMaterial;
    // Optional masked pipe material: clips the rendered surface around each impact.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gas|Visual")
    bool bUseMaskedHoleMaterial = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gas|Audio")
    TObjectPtr<USoundBase> LeakLoopSound;
    UPROPERTY(VisibleInstanceOnly, SaveGame, BlueprintReadOnly, Category="Gas|Holes")
    TArray<FGasPipeHole> Holes;
private:
    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInstanceDynamic>> HoleMaterials;
    UPROPERTY(Transient)
    TArray<TObjectPtr<UNiagaraComponent>> Jets;
    UPROPERTY(Transient)
    TArray<TObjectPtr<UDecalComponent>> Decals;
    UPROPERTY(Transient)
    TArray<TObjectPtr<UAudioComponent>> LeakSounds;
    float ReceiveDamageInternal(const FDamageParams& Data, const FHitResult* ExactHit);
    bool ResolvePuncture(const FDamageParams& Data, FVector& Point, FVector& Normal) const;
    bool AddHole(const FVector& Point, const FVector& Normal);
    void GetWorldHole(const FGasPipeHole& Hole, FVector& Point, FVector& Normal) const;
    void UpdateHoleMaterial();
    void RebuildEffects();
    void ClearEffects();
};
