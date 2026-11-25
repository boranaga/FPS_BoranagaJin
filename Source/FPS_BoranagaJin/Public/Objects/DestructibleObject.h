#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"
#include "Objects/DestructionMeshType.h"
#include "Interface/DamageInterface.h"
#include "Interface/SaveableActorInterface.h"
#include "DestructibleObject.generated.h"

class UFireAreaComponent;

class UStaticMeshComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class USoundBase;
class UGeometryCollectionComponent;

/** Only runtime state missing from the regular SaveGame UPROPERTYs.
 *  CurrentHealth/bDestroyed/bBrokenMeshHidden/bBurning are saved independently,
 *  exactly once. This struct deliberately does not duplicate them.
 */
USTRUCT(BlueprintType)
struct FPS_BORANAGAJIN_API FDestructibleObjectRuntimeSaveState
{
	GENERATED_BODY()

	/** Zero for old saves created before the hierarchical save system. */
	UPROPERTY(SaveGame)
	int32 Version = 0;

	/** Used only when ISaveableActorInterface::ShouldSaveTransform() is false. */
	UPROPERTY(SaveGame)
	bool bHasActorWorldTransform = false;

	UPROPERTY(SaveGame)
	FTransform ActorWorldTransform = FTransform::Identity;

	/** Timer snapshots use -1 when no timer is running. */
	UPROPERTY(SaveGame)
	float BrokenMeshHideTimeRemaining = -1.f;

	/** Remaining actor destruction lifespan, only for bDestroyActorAfterBreak. */
	UPROPERTY(SaveGame)
	float ActorLifeSpanRemaining = -1.f;

	UPROPERTY(SaveGame)
	float BurnDurationTimeRemaining = -1.f;

	UPROPERTY(SaveGame)
	float BurnDamageTimeRemaining = -1.f;

	/** Rigid-body state only; Chaos per-fragment state is not reconstructed. */
	UPROPERTY(SaveGame)
	bool bHasBrokenStaticMeshPhysics = false;

	UPROPERTY(SaveGame)
	FTransform BrokenStaticMeshWorldTransform = FTransform::Identity;

	UPROPERTY(SaveGame)
	FVector BrokenStaticMeshLinearVelocity = FVector::ZeroVector;

	UPROPERTY(SaveGame)
	FVector BrokenStaticMeshAngularVelocity = FVector::ZeroVector;
};

UCLASS()
class FPS_BORANAGAJIN_API ADestructibleObject : public AActor, public IDamageableInterface, public ISaveableActorInterface
{
	GENERATED_BODY()
public:
	ADestructibleObject();

	/** Synchronizes inactive destruction meshes to IntactMesh. */
	UFUNCTION(BlueprintCallable, Category = "Destruction|Mesh Transform")
	void SynchronizeDestructionMeshTransforms();

	/** Enable or disable transform synchronization at runtime. */
	UFUNCTION(BlueprintCallable, Category = "Destruction|Mesh Transform")
	void SetMeshTransformSynchronizationEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Destruction|Mesh Transform")
	bool IsMeshTransformSynchronizationEnabled() const { return bSynchronizeMeshTransforms; }

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostRegisterAllComponents() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	/** Called whenever IntactMesh's component transform changes. */
	void HandleIntactMeshTransformUpdated(USceneComponent* UpdatedComponent,
		EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport);

	/** Sync a single inactive output mesh, including immediately before activation. */
	void SynchronizeOneDestructionMesh(USceneComponent* TargetMesh);

#pragma region SaveableActorInterface
#if WITH_EDITOR
	virtual void PostEditImport() override;
	virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode) override;
#endif
public:
	virtual FGuid GetInstanceID() const override
	{
		return InstanceID;
	}

	virtual bool ShouldSaveTransform() const override
	{
		return false;
	}
	virtual bool IsRuntimeSpawned() const override
	{
		return bRuntimeSpawned;
	}
	virtual void OnBeforeSave() override;
	virtual void OnAfterLoad() override;
	virtual void SetRuntimeSpawned(bool bIsRuntimeSpawned) override;
protected:
	UPROPERTY(EditInstanceOnly, Category = "Save", meta = (AllowPrivateAccess = "true"))
	FGuid InstanceID;
	UPROPERTY(Transient)
	bool bRuntimeSpawned = false;
	// Legacy runtime mirror. Not saved: CurrentHealth is the one authoritative
	// durability value, including in older SaveGame data.
	UPROPERTY(VisibleAnywhere, Category = "Destructible|Legacy")
	float CurrentDurability = 100.f;

	/** Missing dynamic information captured immediately before Actor::Serialize. */
	UPROPERTY(VisibleInstanceOnly, SaveGame, Category = "Destruction|Save")
	FDestructibleObjectRuntimeSaveState SavedObjectRuntimeState;
#pragma endregion
protected:
	virtual void BeginPlay() override;
public:
	virtual float ReceiveDamage(const FDamageParams& DamageData) override;
	virtual bool IsDead() const;

	// The original damage pipeline reduces CurrentHealth, not CurrentDurability.
	// Gameplay AI therefore queries this authoritative value.
	UFUNCTION(BlueprintPure, Category = "Destruction|AI")
	float GetRemainingDurability() const { return FMath::Max(0.f, CurrentHealth); }

	UFUNCTION(BlueprintPure, Category = "Destruction|AI")
	float GetMaximumDurability() const { return FMath::Max(0.f, MaxHealth); }

	UFUNCTION(BlueprintPure, Category = "Destruction|AI")
	bool CanBeDamagedByAI() const
	{
		return bIsDestructible && !bDestroyed && CurrentHealth > 0.f;
	}

	UFUNCTION(BlueprintCallable, Category = "Fire")
	void Ignite(AActor* FireCauser = nullptr);

protected:
	UFUNCTION()
	virtual void BreakObject(const FVector& HitLocation, const FVector& HitDirection);

	UFUNCTION()
	virtual void HideBrokenMesh();

	UFUNCTION(BlueprintCallable, Category = "Fire")
	virtual void StartBurning(AActor* FireCauser = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Fire")
	virtual void StopBurning();

	UFUNCTION()
	virtual void ApplyBurnDamage();
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
	USceneComponent* SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
	UStaticMeshComponent* IntactMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
	UStaticMeshComponent* BrokenMesh;

	/**
	 * If enabled, IntactMesh drives the transforms of inactive BrokenMesh and
	 * FracturedMesh. Turn this off to keep individual Blueprint offsets.
	 * Defaults to off for existing DestructibleObject assets; DestructibleDoor
	 * enables it by default so its three meshes move together.
	 */
	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category = "Destruction|Mesh Transform")
	bool bSynchronizeMeshTransforms = false;

private:
	/** Prevents nested transform updates from rewriting simulated debris. */
	bool bSynchronizingMeshTransforms = false;

protected:

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category = "Destruction")
	float MaxHealth = 10.f;

	UPROPERTY(VisibleAnywhere, SaveGame, BlueprintReadOnly, Category = "Destruction")
	float CurrentHealth = 10.f;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category = "Destruction")
	bool bIsDestructible = true;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Destruction")
	//float BreakImpulse = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Destruction")
	float BreakImpulse = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Destruction")
	float BrokenMeshLifeTime = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Destruction")
	bool bDestroyActorAfterBreak = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Destruction")
	bool bHideBrokenMeshAfterDelay = true;

	UPROPERTY(VisibleAnywhere, SaveGame, BlueprintReadOnly, Category = "Destruction")
	bool bDestroyed = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	UNiagaraSystem* BreakEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	USoundBase* BreakSound;

	FTimerHandle HideBrokenMeshTimerHandle;

	// <Burning>
	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category = "Fire")
	bool bCanBurn = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire")
	bool bCanIgniteFromDamage = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire")
	float IgnitionDamageThreshold = 0.f;

	UPROPERTY(VisibleAnywhere, SaveGame, BlueprintReadOnly, Category = "Fire")
	bool bBurning = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire")
	float BurnDamage = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire")
	float BurnDamageInterval = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire")
	float BurnDuration = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire")
	UNiagaraSystem* FireEffect;

	UPROPERTY()
	UNiagaraComponent* ActiveFireEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire")
	USoundBase* FireStartSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire")
	USoundBase* FireLoopSound;

	UPROPERTY()
	UAudioComponent* ActiveFireLoopSound;

	UPROPERTY()
	AActor* BurnCauser;

	FTimerHandle BurnDamageTimerHandle;
	FTimerHandle BurnDurationTimerHandle;

	// <Fire Component>
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
	UFireAreaComponent* FireAreaComponent;

	//---------------------------------------
protected:
	void BreakStaticMesh(const FVector& HitLocation, const FVector& HitDirection);
	void BreakGeometryCollection(const FVector& HitLocation, const FVector& HitDirection, bool bPlayFractureAnimation = true);

	void StopZeroGravityFracture();
	void StartZeroGravityFracture(const FVector& Origin);
	void UpdateZeroGravityFracture();
	void ApplyFragmentLinearVelocity(const FVector& Origin, float Speed);
	void ApplyFragmentAngularVelocity(float Speed);

public:
	UFUNCTION(BlueprintCallable, Category = "Destruction")
	virtual void RestoreDestructibleObject();

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
	TObjectPtr<UGeometryCollectionComponent> FracturedMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Destruction")
	EDestructionMeshType DestructionMeshType = EDestructionMeshType::StaticMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Destruction|Chaos", meta = (EditCondition = "DestructionMeshType == EDestructionMeshType::GeometryCollection", EditConditionHides, ClampMin = "0.0"))
	float FractureDamageRadius = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Destruction|Chaos", meta = (EditCondition = "DestructionMeshType == EDestructionMeshType::GeometryCollection", EditConditionHides, ClampMin = "0.0"))
	float FractureStrain = 500000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Destruction|Chaos", meta = (EditCondition = "DestructionMeshType == EDestructionMeshType::GeometryCollection", EditConditionHides, ClampMin = "0.0"))
	float FractureImpulseStrength = 1500.f;

	// Zero-gravity fracture: individual pieces receive Chaos velocity fields.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Destruction|Chaos|ZeroGravity")
	bool bUseZeroGravityFracture = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Destruction|Chaos|ZeroGravity", meta = (EditCondition = "bUseZeroGravityFracture", ClampMin = "0.0"))
	float FragmentSpreadSpeed = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Destruction|Chaos|ZeroGravity", meta = (EditCondition = "bUseZeroGravityFracture", ClampMin = "0.0"))
	float FragmentAngularSpeed = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Destruction|Chaos|ZeroGravity", meta = (EditCondition = "bUseZeroGravityFracture", ClampMin = "0.01"))
	float FragmentSpreadDuration = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Destruction|Chaos|ZeroGravity", meta = (EditCondition = "bUseZeroGravityFracture", ClampMin = "0.01"))
	float FragmentUpdateInterval = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Destruction|Chaos|ZeroGravity", meta = (EditCondition = "bUseZeroGravityFracture", ClampMin = "0.0"))
	float FragmentStartDelay = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Destruction|Chaos|ZeroGravity", meta = (EditCondition = "bUseZeroGravityFracture"))
	bool bFreezeFragmentsAfterSpread = true;

	UPROPERTY(VisibleAnywhere, SaveGame, BlueprintReadOnly, Category = "Destruction")
	bool bBrokenMeshHidden = false;

	FTimerHandle FragmentStartTimerHandle;
	FTimerHandle FragmentUpdateTimerHandle;
	FVector FragmentSpreadOrigin = FVector::ZeroVector;
	FVector FragmentAngularAxis = FVector::UpVector;
	float FragmentSpreadElapsed = 0.f;

};