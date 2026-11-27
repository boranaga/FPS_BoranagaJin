#include "ObjectPoolSubsystem.h"
#include "PoolableActorInterface.h"
#include "Interface/SaveableActorInterface.h"
#include "Engine/World.h"

void UObjectPoolSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	bIsDeinitializing = false;
}

void UObjectPoolSubsystem::Deinitialize()
{
	bIsDeinitializing = true;
	// Destroying an actor can run callbacks; do not iterate live pool containers.
	TArray<AActor*> Actors;
	Actors.Reserve(ActorToPoolClassMap.Num());
	for (const auto& Entry : ActorToPoolClassMap)
	{
		if (IsValid(Entry.Key.Get())) Actors.Add(Entry.Key.Get());
	}
	ActorPools.Empty();
	ActorToPoolClassMap.Empty();
	ReturningActors.Empty();
	for (AActor* Actor : Actors)
	{
		if (IsValid(Actor))
		{
			Actor->OnDestroyed.RemoveDynamic(this, &UObjectPoolSubsystem::HandlePooledActorDestroyed);
			Actor->Destroy();
		}
	}
	Super::Deinitialize();
}

bool UObjectPoolSubsystem::RegisterActor(AActor* Actor, bool bStartActive)
{
	if (bIsDeinitializing || !IsValid(Actor) || ReturningActors.Contains(Actor))
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

	ActorToPoolClassMap.Add(Actor, ActorClass);
	Actor->OnDestroyed.AddUniqueDynamic(this, &UObjectPoolSubsystem::HandlePooledActorDestroyed);

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
		ReturningActors.Add(Actor);
		DeactivateActor(Actor);
		ReturningActors.Remove(Actor);
		if (IsValid(Actor) && !bIsDeinitializing) AddToAvailablePool(ActorPools.FindOrAdd(ActorClass), Actor);
	}


	//UE_LOG(LogTemp, Error, TEXT("bool UObjectPoolSubsystem::RegisterActor(AActor* Actor, bool bStartActive)"));
	return true;
}

void UObjectPoolSubsystem::PrewarmPool(TSubclassOf<AActor> ActorClass, int32 Count)
{
	if (bIsDeinitializing || !ActorClass || Count <= 0) return;
	for (int32 i = 0; i < Count && !bIsDeinitializing; ++i)
	{
		AActor* NewActor = CreateNewActor(ActorClass);
		if (!IsValid(NewActor)) continue;
		// AItem and AItemPickUp may self-register from BeginPlay during SpawnActor.
		RemoveFromActivePool(ActorPools.FindOrAdd(ActorClass), NewActor);
		ReturningActors.Add(NewActor);
		DeactivateActor(NewActor);
		ReturningActors.Remove(NewActor);
		if (IsValid(NewActor) && !bIsDeinitializing) AddToAvailablePool(ActorPools.FindOrAdd(ActorClass), NewActor);
	}
}

void UObjectPoolSubsystem::EnsurePoolCapacity(TSubclassOf<AActor> ActorClass, int32 MinTotalCount)
{
	if (bIsDeinitializing || !ActorClass || MinTotalCount <= 0) return;
	int32 TotalCount = 0;
	if (const FActorPool* Pool = ActorPools.Find(ActorClass))
	{
		for (AActor* Actor : Pool->AvailableActors) if (IsValid(Actor)) ++TotalCount;
		for (AActor* Actor : Pool->ActiveActors) if (IsValid(Actor)) ++TotalCount;
	}
	PrewarmPool(ActorClass, FMath::Max(0, MinTotalCount - TotalCount));
}

AActor* UObjectPoolSubsystem::SpawnFromPool(TSubclassOf<AActor> ActorClass, const FVector& Location, const FRotator& Rotation, bool bShouldCreateNewActor)
{
	if (bIsDeinitializing || !ActorClass) return nullptr;
	AActor* Actor = nullptr;
	if (!bShouldCreateNewActor)
	{
		FActorPool& Pool = ActorPools.FindOrAdd(ActorClass);
		while (!Pool.AvailableActors.IsEmpty() && !Actor)
		{
			AActor* Candidate = Pool.AvailableActors.Pop(EAllowShrinking::No);
			Pool.AvailableActorSet.Remove(Candidate);
			if (IsValid(Candidate)) Actor = Candidate;
			else ActorToPoolClassMap.Remove(Candidate);
		}
	}
	if (!Actor) Actor = CreateNewActor(ActorClass);
	if (!IsValid(Actor) || bIsDeinitializing) return nullptr;

	// Register as active BEFORE invoking actor callbacks, and do not retain a TMap value reference across callbacks.
	AddToActivePool(ActorPools.FindOrAdd(ActorClass), Actor);
	ActivateActor(Actor, Location, Rotation);
	const FActorPool* Pool = ActorPools.Find(ActorClass);
	return IsValid(Actor) && Pool && Pool->ActiveActors.Contains(Actor) ? Actor : nullptr;
}

void UObjectPoolSubsystem::ReturnToPool(AActor* Actor)
{
	if (bIsDeinitializing || !IsValid(Actor) || Actor->GetWorld() != GetWorld() || ReturningActors.Contains(Actor)) return;
	const TSubclassOf<AActor> PoolClass = Actor->GetClass();
	FActorPool* Pool = ActorPools.Find(PoolClass);
	if (!Pool)
	{
		Actor->Destroy();
		ActorToPoolClassMap.Remove(Actor);
		return;
	}
	if (Pool->AvailableActorSet.Contains(Actor)) return;
	// Only a checked-out actor may be returned. Duplicate calls cannot insert duplicates.
	if (!RemoveFromActivePool(*Pool, Actor)) return;
	ReturningActors.Add(Actor);
	DeactivateActor(Actor);
	ReturningActors.Remove(Actor);
	if (IsValid(Actor) && !bIsDeinitializing) AddToAvailablePool(ActorPools.FindOrAdd(PoolClass), Actor);
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
	if (bIsDeinitializing || !IsValid(Actor) || Actor->GetWorld() != GetWorld()) return nullptr;
	const TSubclassOf<AActor> PoolClass = Actor->GetClass();
	FActorPool* Pool = ActorPools.Find(PoolClass);
	if (!Pool || !RemoveFromAvailablePool(*Pool, Actor)) return nullptr;
	AddToActivePool(*Pool, Actor);
	ActivateActor(Actor, Location, Rotation);
	Pool = ActorPools.Find(PoolClass);
	return IsValid(Actor) && Pool && Pool->ActiveActors.Contains(Actor) ? Actor : nullptr;
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
	if (bIsDeinitializing || !GetWorld() || !ActorClass)
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

	if (!IsValid(NewActor))
	{
		return nullptr;
	}

	ActorToPoolClassMap.Add(NewActor, ActorClass);
	NewActor->OnDestroyed.AddUniqueDynamic(this, &UObjectPoolSubsystem::HandlePooledActorDestroyed);

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

	Actor->SetActorLocationAndRotation(Location, Rotation, false, nullptr, ETeleportType::TeleportPhysics);
	if (!IsValid(Actor)) return;

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
	if (!IsValid(Actor)) return;
	if (IPoolableActorInterface* Poolable = Cast<IPoolableActorInterface>(Actor)) Poolable->OnDeactivateToPool();
	if (!IsValid(Actor)) return;
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

	const int32 RemovedCount = Pool.AvailableActors.RemoveSwap(Actor, EAllowShrinking::No);

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

	const int32 RemovedCount = Pool.ActiveActors.Remove(Actor);

	return RemovedCount > 0;
}
void UObjectPoolSubsystem::HandlePooledActorDestroyed(AActor* Actor)
{
	// OnDestroyed may be called with an actor already marked for destruction.
	if (!Actor) return;
	if (const TSubclassOf<AActor>* PoolClass = ActorToPoolClassMap.Find(Actor))
	{
		if (FActorPool* Pool = ActorPools.Find(*PoolClass))
		{
			Pool->AvailableActorSet.Remove(Actor);
			Pool->AvailableActors.RemoveSwap(Actor, EAllowShrinking::No);
			Pool->ActiveActors.Remove(Actor);
		}
	}
	ActorToPoolClassMap.Remove(Actor);
	ReturningActors.Remove(Actor);
}
