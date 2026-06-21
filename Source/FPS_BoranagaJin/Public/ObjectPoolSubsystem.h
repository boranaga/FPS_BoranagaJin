#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ObjectPoolSubsystem.generated.h"

USTRUCT()
struct FActorPool
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<AActor>> AvailableActors;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> ActiveActors;
};

UCLASS()
class FPS_BORANAGAJIN_API UObjectPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	AActor* SpawnFromPool(
		TSubclassOf<AActor> ActorClass,
		const FVector& Location,
		const FRotator& Rotation
	);

	template<typename T>
	T* SpawnFromPool(
		TSubclassOf<T> ActorClass,
		const FVector& Location,
		const FRotator& Rotation
	)
	{
		return Cast<T>(SpawnFromPool(
			TSubclassOf<AActor>(ActorClass),
			Location,
			Rotation
		));
	}

	void ReturnToPool(AActor* Actor);

	void PrewarmPool(
		TSubclassOf<AActor> ActorClass,
		int32 Count
	);

private:
	UPROPERTY()
	TMap<TSubclassOf<AActor>, FActorPool> ActorPools;

	AActor* CreateNewActor(TSubclassOf<AActor> ActorClass);

	void ActivateActor(
		AActor* Actor,
		const FVector& Location,
		const FRotator& Rotation
	);

	void DeactivateActor(AActor* Actor);
};
