#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ObjectPoolSubsystem.generated.h"

class IPoolableActorInterface;

USTRUCT()
struct FActorPool
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<AActor>> AvailableActors;
	UPROPERTY()
	TSet<TObjectPtr<AActor>> AvailableActorSet;
	UPROPERTY()
	TSet<TObjectPtr<AActor>> ActiveActors;
};

UCLASS()
class FPS_BORANAGAJIN_API UObjectPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	bool RegisterActor(AActor* Actor, bool bStartActive = true);

	AActor* SpawnFromPool(TSubclassOf<AActor> ActorClass, const FVector& Location, const FRotator& Rotation, bool bShouldCreateNewActor = false);

	template<typename T>
	T* SpawnFromPool(TSubclassOf<T> ActorClass, const FVector& Location, const FRotator& Rotation, bool bShouldCreateNewActor = false)
	{
		return Cast<T>(SpawnFromPool(TSubclassOf<AActor>(ActorClass), Location, Rotation, bShouldCreateNewActor));
	}

	void ReturnToPool(AActor* Actor);
	/** Add Count actors (legacy additive behavior). */
	void PrewarmPool(TSubclassOf<AActor> ActorClass, int32 Count);
	/** Ensure at least MinTotalCount valid actors, including active leases. */
	void EnsurePoolCapacity(TSubclassOf<AActor> ActorClass, int32 MinTotalCount);
	bool IsActorInAvailablePool(AActor* Actor) const;
	AActor* GetActorFromAvailablePool(AActor* Actor, const FVector& Location, const FRotator& Rotation);
	AActor* FindActorByInstanceIDFromPool(FGuid InInstanceID, TSubclassOf<AActor> ActorClass);
private:
	UPROPERTY()
	TMap<TSubclassOf<AActor>, FActorPool> ActorPools;


	UPROPERTY()
	TMap<TObjectPtr<AActor>, TSubclassOf<AActor>> ActorToPoolClassMap; //MEMO: 없어도 되지 않나?

	bool bIsDeinitializing = false;
	// Prevent a pool callback from recursively returning the same actor.
	TSet<AActor*> ReturningActors;
	UFUNCTION()
	void HandlePooledActorDestroyed(AActor* Actor);

	AActor* CreateNewActor(TSubclassOf<AActor> ActorClass);

	AActor* ExtractActorFromAvailablePool(AActor* Actor);
	void ActivateActor(AActor* Actor, const FVector& Location, const FRotator& Rotation);
	void DeactivateActor(AActor* Actor);
	void AddToAvailablePool(FActorPool& Pool, AActor* Actor);
	bool RemoveFromAvailablePool(FActorPool& Pool, AActor* Actor);
	void AddToActivePool(FActorPool& Pool, AActor* Actor);
	bool RemoveFromActivePool(FActorPool& Pool, AActor* Actor);
};