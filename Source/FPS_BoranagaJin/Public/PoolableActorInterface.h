#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PoolableActorInterface.generated.h"

class UObjectPoolSubsystem;

UINTERFACE()
class FPS_BORANAGAJIN_API UPoolableActorInterface : public UInterface
{
	GENERATED_BODY()
};

class FPS_BORANAGAJIN_API IPoolableActorInterface
{
	GENERATED_BODY()

public:
	virtual void SetOwningPool(UObjectPoolSubsystem* NewPool) {}

	virtual void OnActivateFromPool() {}

	virtual void OnDeactivateToPool() {}

	virtual bool IsActiveInPool() const { return false; }
};