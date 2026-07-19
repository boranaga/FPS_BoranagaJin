

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Characters/DamageParams.h"
#include "DamageInterface.generated.h"

UINTERFACE(MinimalAPI)
class UDamageableInterface : public UInterface
{
	GENERATED_BODY()
};

class FPS_BORANAGAJIN_API IDamageableInterface
{
	GENERATED_BODY()

public:
	//virtual bool TakeDamage(const FDamageParams& DamageData, AActor* DamageCauser) = 0;

	virtual float ReceiveDamage(const FDamageParams& DamageData) = 0;

	virtual bool IsDead() const = 0;
};
