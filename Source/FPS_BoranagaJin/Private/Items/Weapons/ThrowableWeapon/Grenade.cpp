#include "Items/Weapons/ThrowableWeapon/Grenade.h"

#include "Kismet/GameplayStatics.h"

AGrenade::AGrenade()
{
	ItemName = EItemName::ItemName_Grenade;

	ThrowStrength = 1600.f;
	FuseTime = 3.f;
}

void AGrenade::ActivateThrowable()
{
	Explode();
}

void AGrenade::Explode()
{
	UWorld* World = GetWorld();

	if (!IsValid(World))
	{
		return;
	}


	//TODO: Damageable Interface의 함수로 통합시키기
	//UGameplayStatics::ApplyRadialDamage(
	//	this,
	//	ExplosionDamage,
	//	GetActorLocation(),
	//	ExplosionRadius,
	//	UDamageType::StaticClass(),
	//	TArray<AActor*>(),
	//	this,
	//	Character,
	//	true
	//);

	// TODO:
	// Niagara Explosion
	// Explosion Sound
	// Camera Shake

	DeactivateItem();
}