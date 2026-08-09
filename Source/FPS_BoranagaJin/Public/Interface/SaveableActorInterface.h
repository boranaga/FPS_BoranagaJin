#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SaveableActorInterface.generated.h"

UINTERFACE(MinimalAPI)
class USaveableActorInterface : public UInterface
{
	GENERATED_BODY()
};

class FPS_BORANAGAJIN_API ISaveableActorInterface
{
	GENERATED_BODY()
public:
	virtual FGuid GetInstanceID() const = 0;
	virtual bool ShouldSaveTransform() const { return true; }
	virtual bool IsRuntimeSpawned() const { return false; }
	virtual void SetRuntimeSpawned(bool bIsRuntimeSpawned) { }
	virtual void OnBeforeSave() { }
	virtual void OnAfterLoad() { }
};