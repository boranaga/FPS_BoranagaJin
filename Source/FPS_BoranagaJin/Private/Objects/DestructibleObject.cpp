#include "Objects/DestructibleObject.h"

#include "Components/StaticMeshComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

ADestructibleObject::ADestructibleObject()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	IntactMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("IntactMesh"));
	IntactMesh->SetupAttachment(SceneRoot);
	IntactMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	IntactMesh->SetSimulatePhysics(false);

	BrokenMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BrokenMesh"));
	BrokenMesh->SetupAttachment(SceneRoot);
	BrokenMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BrokenMesh->SetSimulatePhysics(false);
	BrokenMesh->SetVisibility(false);
}

void ADestructibleObject::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;

	if (BrokenMesh)
	{
		BrokenMesh->SetVisibility(false);
		BrokenMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		BrokenMesh->SetSimulatePhysics(false);
	}
}

//void ADestructibleObject::TakeObjectDamage(
//	float Damage,
//	const FVector& HitLocation,
//	const FVector& HitDirection,
//	AActor* DamageCauser
//)
//{
//	if (bDestroyed) return;
//	if (Damage <= 0.f) return;
//
//	CurrentHealth -= Damage;
//
//	if (CurrentHealth <= 0.f)
//	{
//		BreakObject(HitLocation, HitDirection);
//	}
//}

float ADestructibleObject::ReceiveDamage(const FDamageParams& DamageData)
{
	if (bDestroyed) return 0.f;
	if (DamageData.DamageAmount <= 0.f) return 0.f;

	CurrentHealth -= DamageData.DamageAmount;

	if (CurrentHealth <= 0.f)
	{
		BreakObject(DamageData.ImpactPoint, DamageData.HitDirection);
	}

	return DamageData.DamageAmount;
}

bool ADestructibleObject::IsDead() const
{
	return bDestroyed;
}

void ADestructibleObject::BreakObject(const FVector& HitLocation, const FVector& HitDirection)
{
	if (bDestroyed) return;

	bDestroyed = true;

	if (IntactMesh)
	{
		IntactMesh->SetVisibility(false);
		IntactMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		IntactMesh->SetSimulatePhysics(false);
	}

	if (BrokenMesh)
	{
		BrokenMesh->SetWorldTransform(IntactMesh->GetComponentTransform());

		BrokenMesh->SetVisibility(true);
		BrokenMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		BrokenMesh->SetSimulatePhysics(true);

		const FVector ImpulseDirection = HitDirection.GetSafeNormal();

		BrokenMesh->AddImpulseAtLocation(
			ImpulseDirection * BreakImpulse,
			HitLocation,
			NAME_None
		);
	}

	if (BreakEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			BreakEffect,
			HitLocation,
			HitDirection.Rotation()
		);
	}

	if (BreakSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			GetWorld(),
			BreakSound,
			HitLocation
		);
	}

	if (bDestroyActorAfterBreak)
	{
		SetLifeSpan(BrokenMeshLifeTime);
	}
	else if (bHideBrokenMeshAfterDelay)
	{
		GetWorldTimerManager().SetTimer(
			HideBrokenMeshTimerHandle,
			this,
			&ADestructibleObject::HideBrokenMesh,
			BrokenMeshLifeTime,
			false
		);
	}
}

void ADestructibleObject::HideBrokenMesh()
{
	if (BrokenMesh)
	{
		BrokenMesh->SetVisibility(false);
		BrokenMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		BrokenMesh->SetSimulatePhysics(false);
	}
}