#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SaveableActorInterface.generated.h"

UINTERFACE(MinimalAPI)
class USaveableActorInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 월드 상태 저장 대상 Actor가 구현하는 인터페이스입니다.
 */
class FPS_BORANAGAJIN_API ISaveableActorInterface
{
	GENERATED_BODY()

public:
	/**
	 * 저장 데이터에서 Actor를 식별할 고유 ID를 반환합니다.
	 */
	virtual FGuid GetSaveID() const = 0;

	/**
	 * Transform도 저장하고 복원할지 결정합니다.
	 */
	virtual bool ShouldSaveTransform() const
	{
		return true;
	}

	/**
	 * 저장하기 직전에 호출됩니다.
	 */
	virtual void OnBeforeSave()
	{
	}

	/**
	 * Actor 데이터 복원이 완료된 후 호출됩니다.
	 */
	virtual void OnAfterLoad()
	{
	}
};