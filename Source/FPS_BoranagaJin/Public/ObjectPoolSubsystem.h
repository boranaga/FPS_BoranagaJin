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
	TArray<TObjectPtr<AActor>> ActiveActors; //TODO: ActiveActor 또한 Set을 구현하기
};

UCLASS()
class FPS_BORANAGAJIN_API UObjectPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	bool RegisterActor(AActor* Actor,bool bStartActive = true);

	AActor* SpawnFromPool(TSubclassOf<AActor> ActorClass, const FVector& Location, const FRotator& Rotation);

	template<typename T>
	T* SpawnFromPool(TSubclassOf<T> ActorClass, const FVector& Location, const FRotator& Rotation)
	{
		return Cast<T>(SpawnFromPool(TSubclassOf<AActor>(ActorClass), Location, Rotation));
	}

	void ReturnToPool(AActor* Actor);
	void PrewarmPool(TSubclassOf<AActor> ActorClass, int32 Count);
	bool IsActorInAvailablePool(AActor* Actor) const;
	AActor* GetActorFromAvailablePool(AActor* Actor, const FVector& Location, const FRotator& Rotation);
private:
	UPROPERTY()
	TMap<TSubclassOf<AActor>, FActorPool> ActorPools;


	UPROPERTY()
	TMap<TObjectPtr<AActor>, TSubclassOf<AActor>> ActorToPoolClassMap; //MEMO: 없어도 되지 않나?

	AActor* CreateNewActor(TSubclassOf<AActor> ActorClass);

	AActor* ExtractActorFromAvailablePool(AActor* Actor);
	void ActivateActor(AActor* Actor, const FVector& Location, const FRotator& Rotation);
	void DeactivateActor(AActor* Actor);
	void AddToAvailablePool(FActorPool& Pool, AActor* Actor);
	bool RemoveFromAvailablePool(FActorPool& Pool, AActor* Actor);
	void AddToActivePool(FActorPool& Pool, AActor* Actor);
	bool RemoveFromActivePool(FActorPool& Pool, AActor* Actor);
};