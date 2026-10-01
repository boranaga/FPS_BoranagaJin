#pragma once

#include "CoreMinimal.h"
#include "Items/Weapons/ThrowableWeapon/ThrowableWeapon.h"
#include "SmokeGrenade.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;

UCLASS()
class FPS_BORANAGAJIN_API ASmokeGrenade : public AThrowableWeapon
{
	GENERATED_BODY()



	//TODO: Grenade 참고해서 StateType으로 state 관리하기




public:
	ASmokeGrenade();

protected:
	virtual void ActivateThrowable() override;

	void ActivateSmoke();
	void FinishSmoke();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SmokeGrenade")
	TObjectPtr<UNiagaraSystem> SmokeEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SmokeGrenade")
	float SmokeDuration = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SmokeGrenade")
	float SmokeRadius = 500.f;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> ActiveSmokeComponent;

	FTimerHandle SmokeTimerHandle;
};