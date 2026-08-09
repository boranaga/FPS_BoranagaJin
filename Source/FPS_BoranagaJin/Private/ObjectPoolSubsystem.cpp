#include "ObjectPoolSubsystem.h"
#include "PoolableActorInterface.h"
#include "Interface/SaveableActorInterface.h"
#include "Engine/World.h"

void UObjectPoolSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
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

	Super::Deinitialize();
}

bool UObjectPoolSubsystem::RegisterActor(AActor* Actor, bool bStartActive)
{
	if (!IsValid(Actor))
	{
		return false;
	}

	if (Actor->GetWorld() != GetWorld())
	{
		UE_LOG(LogTemp, Warning, TEXT("ObjectPoolSubsystem: 다른 World의 Actor를 등록할 수 없습니다. Actor=%s"), *GetNameSafe(Actor));
		return false;
	}

	if (!Actor->GetClass()->ImplementsInterface(UPoolableActorInterface::StaticClass()))
	{
		UE_LOG(LogTemp, Warning, TEXT("ObjectPoolSubsystem: Actor가 PoolableActorInterface를 구현하지 않았습니다. Actor=%s"), *GetNameSafe(Actor));
		return false;
	}

	const TSubclassOf<AActor> ActorClass = Actor->GetClass();

	/*
	 * 해당 클래스의 Pool이 없다면 새 FActorPool을 생성하여
	 * ActorPools에 등록한다.
	 */
	FActorPool& Pool = ActorPools.FindOrAdd(ActorClass);

	/*
	 * 이미 Available 또는 Active 상태로 등록되어 있다면
	 * 중복 등록하지 않는다.
	 */
	if (Pool.AvailableActorSet.Contains(Actor) || Pool.ActiveActors.Contains(Actor))
	{
		return false;
	}

	if (IPoolableActorInterface* PoolableActor = Cast<IPoolableActorInterface>(Actor))
	{
		PoolableActor->SetOwningPool(this);
	}

	if (bStartActive)
	{
		/*
		 * 레벨에 배치된 Actor는 현재 보이는 상태를 유지하면서
		 * ActiveActorSet에만 등록한다.
		 */
		Pool.ActiveActors.Add(Actor);

		if (IPoolableActorInterface* Poolable = Cast<IPoolableActorInterface>(Actor))
		{
			Poolable->OnActivateFromPool();
		}
	}
	else
	{
		/*
		 * 처음부터 사용 가능한 비활성 객체로 등록한다.
		 */
		DeactivateActor(Actor);
		AddToAvailablePool(Pool, Actor);
	}


	//UE_LOG(LogTemp, Error, TEXT("bool UObjectPoolSubsystem::RegisterActor(AActor* Actor, bool bStartActive)"));
	return true;
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

AActor* UObjectPoolSubsystem::SpawnFromPool(TSubclassOf<AActor> ActorClass, const FVector& Location, const FRotator& Rotation, bool bShouldCreateNewActor)
{
	if (!ActorClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("AActor* UObjectPoolSubsystem::SpawnFromPool 1"));
		return nullptr;
	}

	FActorPool& Pool = ActorPools.FindOrAdd(ActorClass);
	AActor* Actor = nullptr;

	if (bShouldCreateNewActor)
	{
		Actor = CreateNewActor(ActorClass);
	}
	else
	{
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
			UE_LOG(LogTemp, Warning, TEXT("AActor* UObjectPoolSubsystem::SpawnFromPool 2"));
			Actor = CreateNewActor(ActorClass);
		}
	}

	if (!Actor)
	{
		UE_LOG(LogTemp, Warning, TEXT("AActor* UObjectPoolSubsystem::SpawnFromPool 3"));
		return nullptr;
	}

	ActivateActor(Actor, Location, Rotation);
	AddToActivePool(Pool, Actor);

	UE_LOG(LogTemp, Warning, TEXT("AActor* UObjectPoolSubsystem::SpawnFromPool 4"));
	return Actor;
}

void UObjectPoolSubsystem::ReturnToPool(AActor* Actor)
{
	UE_LOG(LogTemp, Error, TEXT("void UObjectPoolSubsystem::ReturnToPool(AActor* Actor)"));

	if (!IsValid(Actor)) { return; }

	//TODO: 사용 고려해봐야함
	//TSubclassOf<AActor>* PoolClass = ActorToPoolClassMap.Find(Actor);
	//----------------------------
	const TSubclassOf<AActor> PoolClass = Actor->GetClass();

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

	UE_LOG(LogTemp, Error, TEXT("Class = %s"), *PoolClass->GetName());
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

AActor* UObjectPoolSubsystem::GetActorFromAvailablePool(AActor* Actor, const FVector& Location, const FRotator& Rotation)
{
	if (!IsValid(Actor)) { return nullptr; }

	//TSubclassOf<AActor>* PoolClass = ActorToPoolClassMap.Find(Actor);
	const TSubclassOf<AActor> PoolClass = Actor->GetClass();

	if (!PoolClass)
	{
		UE_LOG(LogTemp, Error, TEXT("GetActorFromAvailablePool 1"));
		return nullptr;
	}

	FActorPool* Pool = ActorPools.Find(*PoolClass);

	if (!Pool)
	{
		UE_LOG(LogTemp, Error, TEXT("GetActorFromAvailablePool 2"));
		return nullptr;
	}

	if (!Pool->AvailableActorSet.Contains(Actor))
	{
		UE_LOG(LogTemp, Error, TEXT("GetActorFromAvailablePool 3"));
		return nullptr;
	}

	const bool bRemoved = RemoveFromAvailablePool(*Pool, Actor);

	if (!bRemoved)
	{
		UE_LOG(LogTemp, Error, TEXT("GetActorFromAvailablePool 4"));
		return nullptr;
	}

	ActivateActor(Actor, Location, Rotation);
	AddToActivePool(*Pool, Actor);

	UE_LOG(LogTemp, Error, TEXT("AActor* UObjectPoolSubsystem::GetActorFromAvailablePool(AActor* Actor, const FVector& Location, const FRotator& Rotation)"));

	return Actor;
}

AActor* UObjectPoolSubsystem::FindActorByInstanceIDFromPool(FGuid InInstanceID, TSubclassOf<AActor> ActorClass)
{
	if (!InInstanceID.IsValid()) { return nullptr; }
	if (!ActorClass)
	{
		UE_LOG(LogTemp, Error, TEXT("FindActorByInstanceIDFromPool 1"));
		return nullptr;
	}

	FActorPool* Pool = ActorPools.Find(*ActorClass);

	if (!Pool)
	{
		UE_LOG(LogTemp, Error, TEXT("GetActorFromAvailablePool 2"));
		return nullptr;
	}

	for (AActor* Actor : Pool->AvailableActors)
	{
		if (!IsValid(Actor))
		{
			continue;
		}

		//IPoolableActorInterface* PoolableActor = Cast<IPoolableActorInterface>(Actor);
		ISaveableActorInterface* SaveableActor = Cast<ISaveableActorInterface>(Actor);

		if (!SaveableActor)
		{
			continue;
		}

		if (SaveableActor->GetInstanceID() == InInstanceID)
		{
			return Actor;
		}
	}

	for (AActor* Actor : Pool->ActiveActors)
	{
		if (!IsValid(Actor))
		{
			continue;
		}

		ISaveableActorInterface* SaveableActor = Cast<ISaveableActorInterface>(Actor);

		if (!SaveableActor)
		{
			continue;
		}

		if (SaveableActor->GetInstanceID() == InInstanceID)
		{
			return Actor;
		}
	}

	return nullptr;
}

AActor* UObjectPoolSubsystem::ExtractActorFromAvailablePool(AActor* Actor)
{
	if (!IsValid(Actor)) { return nullptr; }

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

AActor* UObjectPoolSubsystem::CreateNewActor(TSubclassOf<AActor> ActorClass)
{
	if (!GetWorld() || !ActorClass)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

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

	if (IPoolableActorInterface* Poolable = Cast<IPoolableActorInterface>(NewActor))
	{
		Poolable->SetOwningPool(this);
	}

	if (ISaveableActorInterface* Poolable = Cast<ISaveableActorInterface>(NewActor))
	{
		Poolable->SetRuntimeSpawned(true);
	}

	return NewActor;
}

void UObjectPoolSubsystem::ActivateActor(AActor* Actor, const FVector& Location, const FRotator& Rotation)
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

	if (IPoolableActorInterface* Poolable = Cast<IPoolableActorInterface>(Actor))
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

	if (IPoolableActorInterface* Poolable = Cast<IPoolableActorInterface>(Actor))
	{
		Poolable->OnDeactivateToPool();
	}

	Actor->SetActorHiddenInGame(true);
	Actor->SetActorEnableCollision(false);
	Actor->SetActorTickEnabled(false);
}

void UObjectPoolSubsystem::AddToAvailablePool(FActorPool& Pool, AActor* Actor)
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

bool UObjectPoolSubsystem::RemoveFromAvailablePool(FActorPool& Pool, AActor* Actor)
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

void UObjectPoolSubsystem::AddToActivePool(FActorPool& Pool, AActor* Actor)
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

bool UObjectPoolSubsystem::RemoveFromActivePool(FActorPool& Pool, AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return false;
	}

	const int32 RemovedCount = Pool.ActiveActors.RemoveSwap(Actor);

	return RemovedCount > 0;
}