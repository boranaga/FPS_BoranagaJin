#include "Objects/ExplosiveDestructibleObject.h"
#include "Characters/GameDamageType.h" //TODO: °æ·Î º¯°æ ÇÊ¿ä
#include "Interface/DamageInterface.h"

#include "Engine/OverlapResult.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "DrawDebugHelpers.h"

AExplosiveDestructibleObject::AExplosiveDestructibleObject()
{
	ExplosionRadius = 500.f;
	ExplosionMaxDamage = 100.f;
	ExplosionMinDamage = 20.f;
	ExplosionImpulse = 3000.f;
}

void AExplosiveDestructibleObject::BreakObject(
	const FVector& HitLocation,
	const FVector& HitDirection
)
{
	if (bDestroyed) return;

	const FVector ExplosionLocation = GetActorLocation();

	Explode(ExplosionLocation);

	if (bBreakSelfOnExplosion)
	{
		Super::BreakObject(HitLocation, HitDirection);
	}
}

void AExplosiveDestructibleObject::Explode(const FVector& ExplosionLocation)
{
	TArray<FOverlapResult> OverlapResults;

	FCollisionShape CollisionShape;
	CollisionShape.SetSphere(ExplosionRadius);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	const bool bHasOverlap = GetWorld()->OverlapMultiByChannel(
		OverlapResults,
		ExplosionLocation,
		FQuat::Identity,
		ExplosionTraceChannel,
		CollisionShape,
		QueryParams
	);

	if (bHasOverlap)
	{
		for (const FOverlapResult& Result : OverlapResults)
		{
			AActor* TargetActor = Result.GetActor();

			if (!TargetActor) continue;
			if (TargetActor == this) continue;
			if (!TargetActor->Implements<UDamageInterface>()) continue;

			const FVector TargetLocation = TargetActor->GetActorLocation();
			const float Distance = FVector::Distance(ExplosionLocation, TargetLocation);
			const float DamageAmount = CalculateExplosionDamage(Distance);

			const FVector ExplosionDirection =
				(TargetLocation - ExplosionLocation).GetSafeNormal();

			IDamageInterface* Damageable = Cast<IDamageInterface>(TargetActor);

			//------------
			FDamageParams Damage; //TODO: ÂøÅº À§Ä¡
			Damage.DamageAmount = DamageAmount;
			Damage.DamageType = EGameDamageType::Explosion;
			//Damage.bCanForceDamage = bCanForceDamage;
			//Damage.HitBoneName = BoneName;
			Damage.ImpulseDirection = ExplosionDirection;
			//Damage.SurfaceType = SurfaceType;
			Damage.ImpactPoint = TargetLocation;
			Damage.DamageCauser = this;
			//------------


			if (Damageable)
			{
				//Damageable->TakeObjectDamage(
				//	DamageAmount,
				//	TargetLocation,
				//	ExplosionDirection,
				//	this
				//);
				Damageable->ReceiveDamage(Damage);
			}
		}
	}

	if (ExplosionEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			ExplosionEffect,
			ExplosionLocation
		);
	}

	if (ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			GetWorld(),
			ExplosionSound,
			ExplosionLocation
		);
	}

#if WITH_EDITOR
	DrawDebugSphere(
		GetWorld(),
		ExplosionLocation,
		ExplosionRadius,
		32,
		FColor::Red,
		false,
		2.f
	);
#endif
}

float AExplosiveDestructibleObject::CalculateExplosionDamage(float Distance) const
{
	const float Alpha = FMath::Clamp(Distance / ExplosionRadius, 0.f, 1.f);

	return FMath::Lerp(
		ExplosionMaxDamage,
		ExplosionMinDamage,
		Alpha
	);
}