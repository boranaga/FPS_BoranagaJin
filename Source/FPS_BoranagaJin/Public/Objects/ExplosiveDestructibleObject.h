#pragma once

#include "CoreMinimal.h"
#include "DestructibleObject.h"
#include "ExplosiveDestructibleObject.generated.h"

class UNiagaraSystem;
class USoundBase;

UCLASS()
class FPS_BORANAGAJIN_API AExplosiveDestructibleObject : public ADestructibleObject
{
	GENERATED_BODY()

public:
	AExplosiveDestructibleObject();

protected:
	virtual void BreakObject(const FVector& HitLocation, const FVector& HitDirection) override;

protected:
	UFUNCTION()
	virtual void Explode(const FVector& ExplosionLocation);

	float CalculateExplosionDamage(float Distance) const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion")
	float ExplosionRadius = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion")
	float ExplosionMaxDamage = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion")
	float ExplosionMinDamage = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion")
	float ExplosionImpulse = 3000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion")
	bool bBreakSelfOnExplosion = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion")
	TEnumAsByte<ECollisionChannel> ExplosionTraceChannel = ECC_WorldDynamic; //TODO: Channel ¼³Á¤

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	UNiagaraSystem* ExplosionEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	USoundBase* ExplosionSound;
};