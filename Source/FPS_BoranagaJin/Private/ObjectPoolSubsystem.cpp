#include "ObjectPoolSubsystem.h"
#include "PoolableActorInterface.h"
#include "Engine/World.h"

void UObjectPoolSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	//UE_LOG(LogTemp, Warning, TEXT("ObjectPoolSubsystem Initialized"));
}

void UObjectPoolSubsystem::Deinitialize()
{
	for (auto& Pair : ActorPools)
	{
		FActorPool& Pool = Pair.Value;

		for (AActor* Actor : Pool.AvailableActors)
		{
			if (IsValid(Actor))
			{
				Actor->Destroy();
			}
		}

		for (AActor* Actor : Pool.ActiveActors)
		{
			if (IsValid(Actor))
			{
				Actor->Destroy();
			}
		}

		Pool.AvailableActors.Empty();
		Pool.AvailableActorSet.Empty();
		Pool.ActiveActors.Empty();
	}

	ActorPools.Empty();
	ActorToPoolClassMap.Empty();

	UE_LOG(LogTemp, Warning, TEXT("ObjectPoolSubsystem Deinitialized"));

	Super::Deinitialize();
}

void UObjectPoolSubsystem::PrewarmPool(
	TSubclassOf<AActor> ActorClass,
	int32 Count
)
{
	if (!ActorClass || Count <= 0)
	{
		return;
	}

	FActorPool& Pool = ActorPools.FindOrAdd(ActorClass);

	for (int32 i = 0; i < Count; ++i)
	{
		AActor* NewActor = CreateNewActor(ActorClass);

		if (!NewActor)
		{
			continue;
		}

		DeactivateActor(NewActor);
		AddToAvailablePool(Pool, NewActor);
	}
}

AActor* UObjectPoolSubsystem::SpawnFromPool(
	TSubclassOf<AActor> ActorClass,
	const FVector& Location,
	const FRotator& Rotation
)
{
	if (!ActorClass)
	{
		return nullptr;
	}

	FActorPool& Pool = ActorPools.FindOrAdd(ActorClass);

	AActor* Actor = nullptr;

	while (Pool.AvailableActors.Num() > 0 && !Actor)
	{
		Actor = Pool.AvailableActors.Pop();

		if (!IsValid(Actor))
		{
			Actor = nullptr;
			continue;
		}

		Pool.AvailableActorSet.Remove(Actor);
	}

	if (!Actor)
	{
		Actor = CreateNewActor(ActorClass);
	}

	if (!Actor)
	{
		return nullptr;
	}

	ActivateActor(Actor, Location, Rotation);
	AddToActivePool(Pool, Actor);

	return Actor;
}

void UObjectPoolSubsystem::ReturnToPool(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return;
	}

	TSubclassOf<AActor>* PoolClass = ActorToPoolClassMap.Find(Actor);

	if (!PoolClass)
	{
		Actor->Destroy();
		return;
	}

	FActorPool* Pool = ActorPools.Find(*PoolClass);

	if (!Pool)
	{
		Actor->Destroy();
		ActorToPoolClassMap.Remove(Actor);
		return;
	}

	if (Pool->AvailableActorSet.Contains(Actor))
	{
		return;
	}

	RemoveFromActivePool(*Pool, Actor);

	DeactivateActor(Actor);

	AddToAvailablePool(*Pool, Actor);
}

bool UObjectPoolSubsystem::IsActorInAvailablePool(AActor* Actor) const
{
	if (!IsValid(Actor))
	{
		return false;
	}

	const TSubclassOf<AActor>* PoolClass = ActorToPoolClassMap.Find(Actor);

	if (!PoolClass)
	{
		return false;
	}

	const FActorPool* Pool = ActorPools.Find(*PoolClass);

	if (!Pool)
	{
		return false;
	}

	return Pool->AvailableActorSet.Contains(Actor);
}

AActor* UObjectPoolSubsystem::ExtractActorFromAvailablePool(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return nullptr;
	}

	TSubclassOf<AActor>* PoolClass = ActorToPoolClassMap.Find(Actor);

	if (!PoolClass)
	{
		return nullptr;
	}

	FActorPool* Pool = ActorPools.Find(*PoolClass);

	if (!Pool)
	{
		return nullptr;
	}

	if (!Pool->AvailableActorSet.Contains(Actor))
	{
		return nullptr;
	}

	const bool bRemoved = RemoveFromAvailablePool(*Pool, Actor);

	if (!bRemoved)
	{
		return nullptr;
	}

	return Actor;
}

AActor* UObjectPoolSubsystem::CreateNewActor(
	TSubclassOf<AActor> ActorClass
)
{
	if (!GetWorld() || !ActorClass)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* NewActor = GetWorld()->SpawnActor<AActor>(
		ActorClass,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParams
	);

	if (!NewActor)
	{
		return nullptr;
	}

	ActorToPoolClassMap.Add(NewActor, ActorClass);

	if (IPoolableActorInterface* Poolable =
		Cast<IPoolableActorInterface>(NewActor))
	{
		Poolable->SetOwningPool(this);
	}

	return NewActor;
}

void UObjectPoolSubsystem::ActivateActor(
	AActor* Actor,
	const FVector& Location,
	const FRotator& Rotation
)
{
	if (!IsValid(Actor))
	{
		return;
	}

	Actor->SetActorLocation(Location);
	Actor->SetActorRotation(Rotation);

	Actor->SetActorHiddenInGame(false);
	Actor->SetActorEnableCollision(true);
	Actor->SetActorTickEnabled(true);

	if (IPoolableActorInterface* Poolable =
		Cast<IPoolableActorInterface>(Actor))
	{
		Poolable->OnActivateFromPool();
	}
}

void UObjectPoolSubsystem::DeactivateActor(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return;
	}

	if (IPoolableActorInterface* Poolable =
		Cast<IPoolableActorInterface>(Actor))
	{
		Poolable->OnDeactivateToPool();
	}

	Actor->SetActorHiddenInGame(true);
	Actor->SetActorEnableCollision(false);
	Actor->SetActorTickEnabled(false);
}

void UObjectPoolSubsystem::AddToAvailablePool(
	FActorPool& Pool,
	AActor* Actor
)
{
	if (!IsValid(Actor))
	{
		return;
	}

	if (Pool.AvailableActorSet.Contains(Actor))
	{
		return;
	}

	Pool.AvailableActors.Add(Actor);
	Pool.AvailableActorSet.Add(Actor);
}

bool UObjectPoolSubsystem::RemoveFromAvailablePool(
	FActorPool& Pool,
	AActor* Actor
)
{
	if (!IsValid(Actor))
	{
		return false;
	}

	if (!Pool.AvailableActorSet.Contains(Actor))
	{
		return false;
	}

	Pool.AvailableActorSet.Remove(Actor);

	const int32 RemovedCount = Pool.AvailableActors.RemoveSwap(Actor);

	return RemovedCount > 0;
}

void UObjectPoolSubsystem::AddToActivePool(
	FActorPool& Pool,
	AActor* Actor
)
{
	if (!IsValid(Actor))
	{
		return;
	}

	if (!Pool.ActiveActors.Contains(Actor))
	{
		Pool.ActiveActors.Add(Actor);
	}
}

bool UObjectPoolSubsystem::RemoveFromActivePool(
	FActorPool& Pool,
	AActor* Actor
)
{
	if (!IsValid(Actor))
	{
		return false;
	}

	const int32 RemovedCount = Pool.ActiveActors.RemoveSwap(Actor);

	return RemovedCount > 0;
}