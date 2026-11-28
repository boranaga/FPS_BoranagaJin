#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PoolableActorInterface.h"
#include "Interface/SaveableActorInterface.h"
#include "BloodStainActor.generated.h"

class UDecalComponent;
class UObjectPoolSubsystem;

UCLASS()
class FPS_BORANAGAJIN_API ABloodStainActor : public AActor, public IPoolableActorInterface, public ISaveableActorInterface
{
	GENERATED_BODY()
public:
	ABloodStainActor();
protected:
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void LifeSpanExpired() override;
private:
	UPROPERTY(VisibleAnywhere, Category = "Blood")
	TObjectPtr<UDecalComponent> DecalComponent;
	UPROPERTY()
	TWeakObjectPtr<AActor> TrailOwner;
	UPROPERTY(VisibleAnywhere, SaveGame, Category = "Blood")
	int32 SequenceIndex = INDEX_NONE;
	float BloodSpawnTime = 0.f;

	// Identity belongs to a logical stain, not to its reusable Actor.
	// The outer FWorldActorSaveData stores the ID and transform exactly once.
	FGuid InstanceID;

	UPROPERTY(SaveGame)
	float SavedBloodAge = -1.f;
	UPROPERTY(SaveGame)
	float SavedRemainingLife = -2.f; // -1: infinite, >= 0: finite, -2: unset
	UPROPERTY(SaveGame)
	bool bSavedOwnerIsPlayer = false;
	UPROPERTY(SaveGame)
	FGuid SavedTrailOwnerID;
	// Fallback only for a non-saveable actor placed in the persistent level.
	UPROPERTY(SaveGame)
	FName SavedPlacedOwnerName = NAME_None;

	UPROPERTY(Transient)
	TWeakObjectPtr<UObjectPoolSubsystem> OwningPool;
	bool bIsActiveInPool = false;
	bool bBloodStainInitialized = false;
	FTimerHandle BloodLifeTimer;

	// Preserve existing Actor/Decal Blueprint defaults across all leases.
	float ConfiguredLifeSpan = 30.f;
	float ConfiguredFadeStartDelay = 0.f;
	float ConfiguredFadeDuration = 0.f;
	float ConfiguredFadeInStartDelay = 0.f;
	float ConfiguredFadeInDuration = 0.f;

	void ResetBloodStainRuntime();
	void StopLifetimeTimers();
	void ConfigureDecalFade(float Age = 0.f);
	void ShowBloodStain(float Age, float RemainingLife);
public:
	// A lease is not a usable trail marker until initialization succeeds.
	bool InitializeBloodStain(AActor* InTrailOwner, int32 InSequenceIndex, const FVector& SurfaceNormal);

	virtual void SetOwningPool(UObjectPoolSubsystem* NewPool) override;
	virtual void OnActivateFromPool() override;
	virtual void OnDeactivateToPool() override;
	virtual bool IsActiveInPool() const override { return bIsActiveInPool; }

	UFUNCTION(BlueprintPure, Category = "Blood")
	bool IsBloodStainActive() const { return bBloodStainInitialized; }

	// Expiry or explicit removal. Pool callbacks only reset, never return recursively.
	UFUNCTION(BlueprintCallable, Category = "Blood")
	void ReleaseBloodStain();

	// ISaveableActorInterface: Blood stains are always runtime-created markers.
	virtual FGuid GetInstanceID() const override;
	virtual bool IsRuntimeSpawned() const override { return true; }
	virtual bool ShouldSaveTransform() const override { return true; }
	virtual void OnBeforeSave() override;
	virtual void OnAfterLoad() override;

	bool ShouldSaveBloodStain() const;
	float GetRemainingBloodLife() const;
	// Called only for a newly acquired lease after deserialization.
	void PrepareRestoredBloodStain(const FGuid& SavedID, AActor* ResolvedOwner);
	bool WasPlayerTrailOwner() const { return bSavedOwnerIsPlayer; }
	FGuid GetSavedTrailOwnerID() const { return SavedTrailOwnerID; }
	FName GetSavedPlacedOwnerName() const { return SavedPlacedOwnerName; }

	AActor* GetTrailOwner() const;
	int32 GetSequenceIndex() const;
	float GetBloodSpawnTime() const;
};
