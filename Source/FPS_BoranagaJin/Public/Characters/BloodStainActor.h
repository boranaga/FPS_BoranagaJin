#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PoolableActorInterface.h"
#include "BloodStainActor.generated.h"

class UDecalComponent;
class UObjectPoolSubsystem;

UCLASS()
class FPS_BORANAGAJIN_API ABloodStainActor : public AActor, public IPoolableActorInterface
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
	UPROPERTY(VisibleAnywhere, Category = "Blood")
	int32 SequenceIndex = INDEX_NONE;
	float BloodSpawnTime = 0.f;

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
	void ConfigureDecalFade();
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

	AActor* GetTrailOwner() const;
	int32 GetSequenceIndex() const;
	float GetBloodSpawnTime() const;
};
