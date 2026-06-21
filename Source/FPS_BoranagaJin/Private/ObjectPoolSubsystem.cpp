#include "ObjectPoolSubsystem.h"
#include "PoolableActorInterface.h"
#include "Engine/World.h"

void UObjectPoolSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogTemp, Warning, TEXT("ObjectPoolSubsystem Initialized"));
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
	}

	ActorPools.Empty();

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

		if (NewActor)
		{
			DeactivateActor(NewActor);
			Pool.AvailableActors.Add(NewActor);
		}
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
		}
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
	Pool.ActiveActors.Add(Actor);

	return Actor;
}

void UObjectPoolSubsystem::ReturnToPool(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return;
	}

	TSubclassOf<AActor> ActorClass = Actor->GetClass();

	FActorPool* Pool = ActorPools.Find(ActorClass);
	if (!Pool)
	{
		Actor->Destroy(); //TODO: 이렇게 마음대로 Destroy 해버려도 되는 부분인가?
		return;
	}

	Pool->ActiveActors.Remove(Actor);

	DeactivateActor(Actor);

	Pool->AvailableActors.Add(Actor);
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
	Actor->SetActorEnableCollision(false); //TODO: 이건 안해도 되지 않나?
	Actor->SetActorTickEnabled(false);
}