#include "Items/Weapons/ThrowableWeapon/SmokeGrenade.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

ASmokeGrenade::ASmokeGrenade()
{
	ItemName = EItemName::ItemName_SmokeGrenade;

	ThrowStrength = 1400.f;
	FuseTime = 2.f;
}

void ASmokeGrenade::ActivateThrowable()
{
	ActivateSmoke();
}

void ASmokeGrenade::ActivateSmoke()
{
	if (SmokeEffect)
	{
		ActiveSmokeComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			SmokeEffect,
			GetActorLocation()
		);
	}

	// TODO:
	// SmokeRadius를 이용해 AI Sight 차단 시스템과 연동

	GetWorldTimerManager().SetTimer(
		SmokeTimerHandle,
		this,
		&ASmokeGrenade::FinishSmoke,
		SmokeDuration,
		false
	);
}

void ASmokeGrenade::FinishSmoke()
{
	if (ActiveSmokeComponent)
	{
		ActiveSmokeComponent->Deactivate();
		ActiveSmokeComponent = nullptr;
	}

	DeactivateItem();
}