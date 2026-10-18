#include "Objects/DestructibleObject.h"
#include "Objects/FireAreaComponent.h"

#include "Components/StaticMeshComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "Components/AudioComponent.h"

#include "GeometryCollection/GeometryCollectionComponent.h"


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

	FireAreaComponent = CreateDefaultSubobject<UFireAreaComponent>(TEXT("FireAreaComponent"));
	FireAreaComponent->SetupAttachment(SceneRoot);
	FireAreaComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	FracturedMesh = CreateDefaultSubobject<UGeometryCollectionComponent>(TEXT("FracturedMesh"));
	FracturedMesh->SetupAttachment(SceneRoot);
	FracturedMesh->SetVisibility(false);
	FracturedMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FracturedMesh->SetSimulatePhysics(false);

	// <Save>
	if (!InstanceID.IsValid())
	{
		InstanceID = FGuid::NewGuid();
	}
}

#if WITH_EDITOR
void ADestructibleObject::PostEditImport()
{
	Super::PostEditImport();

	// 에디터에서 복사된 Actor가 기존 Actor와 같은 ID를 가지지 않도록 합니다.
	InstanceID = FGuid::NewGuid();
}

void ADestructibleObject::PostDuplicate(EDuplicateMode::Type DuplicateMode)
{
	Super::PostDuplicate(DuplicateMode);

	if (DuplicateMode == EDuplicateMode::Normal)
	{
		InstanceID = FGuid::NewGuid();
	}
}
#endif

void ADestructibleObject::OnAfterLoad()
{
	//// <Old>

	//if (bDestroyed)
	//{
	//	if (IntactMesh)
	//	{
	//		IntactMesh->SetVisibility(false);
	//		IntactMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	//		IntactMesh->SetSimulatePhysics(false);
	//	}

	//	if (BrokenMesh)
	//	{
	//		BrokenMesh->SetWorldTransform(IntactMesh->GetComponentTransform());

	//		BrokenMesh->SetVisibility(true);
	//		BrokenMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	//		BrokenMesh->SetSimulatePhysics(true);
	//	}


	//	//TODO: 저장 전에 이미 HidBrokenMesh 된 경우에 대한 처리 필요함
	//	if (bDestroyActorAfterBreak)
	//	{
	//		SetLifeSpan(BrokenMeshLifeTime);
	//	}
	//	else if (bHideBrokenMeshAfterDelay)
	//	{
	//		GetWorldTimerManager().SetTimer(
	//			HideBrokenMeshTimerHandle,
	//			this,
	//			&ADestructibleObject::HideBrokenMesh,
	//			BrokenMeshLifeTime,
	//			false
	//		);
	//	}
	//}
	//else
	//{
	//	SetActorHiddenInGame(false);
	//	SetActorEnableCollision(true);
	//}

	//---------------------------
	// <New>

	GetWorldTimerManager().ClearTimer(HideBrokenMeshTimerHandle);

	if (!bDestroyed)
	{
		if (IntactMesh)
		{
			IntactMesh->SetVisibility(true);
			IntactMesh->SetCollisionEnabled(
				ECollisionEnabled::QueryAndPhysics
			);
		}

		HideBrokenMesh();
		return;
	}

	if (IntactMesh)
	{
		IntactMesh->SetVisibility(false);
		IntactMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (DestructionMeshType == EDestructionMeshType::StaticMesh)
	{
		// 기존 Static Mesh 방식으로 파괴 상태를 복원합니다.
		BreakStaticMesh(GetActorLocation(), FVector::ZeroVector);
	}
	else if (DestructionMeshType == EDestructionMeshType::GeometryCollection)
	{
		// 저장 시점의 파편 위치는 복원하지 않고
		// 파괴 상태를 새롭게 재현합니다.
		BreakGeometryCollection(
			GetActorLocation(),
			FVector::UpVector
		);
	}

	if (bHideBrokenMeshAfterDelay && BrokenMeshLifeTime > 0.f)
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

void ADestructibleObject::SetRuntimeSpawned(bool bIsRuntimeSpawned)
{
	bRuntimeSpawned = bIsRuntimeSpawned;
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

	if (FracturedMesh)
	{
		FracturedMesh->SetVisibility(false);
		FracturedMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		FracturedMesh->SetSimulatePhysics(false);
	}
}

float ADestructibleObject::ReceiveDamage(const FDamageParams& DamageData)
{
	if (bDestroyed) return 0.f;
	if (DamageData.DamageAmount <= 0.f) return 0.f;

	CurrentHealth -= DamageData.DamageAmount;

	if (CurrentHealth <= 0.f)
	{
		BreakObject(DamageData.ImpactPoint, DamageData.HitDirection);
	}
	else
	{
		if (bCanIgniteFromDamage && IgnitionDamageThreshold > CurrentHealth)
		{
			if (!bBurning)
			{
				StartBurning();
			}
		}
	}


	return DamageData.DamageAmount;
}

bool ADestructibleObject::IsDead() const
{
	return bDestroyed;
}

void ADestructibleObject::Ignite(AActor* FireCauser)
{
	StartBurning(FireCauser);
}

void ADestructibleObject::BreakObject(const FVector& HitLocation, const FVector& HitDirection)
{
	// <Old>

	//if (bDestroyed) return;

	//StopBurning();

	//bDestroyed = true;

	//if (BreakEffect)
	//{
	//	UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), BreakEffect, HitLocation, HitDirection.Rotation());
	//}

	//if (BreakSound)
	//{
	//	//TODO: Integrate with SoundSystem
	//	UGameplayStatics::PlaySoundAtLocation(GetWorld(), BreakSound, HitLocation);
	//}

	//if (!bIsDestructible) return;

	//if (IntactMesh)
	//{
	//	IntactMesh->SetVisibility(false);
	//	IntactMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	//	IntactMesh->SetSimulatePhysics(false);
	//}

	//if (BrokenMesh)
	//{
	//	BrokenMesh->SetWorldTransform(IntactMesh->GetComponentTransform());

	//	BrokenMesh->SetVisibility(true);
	//	BrokenMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	//	BrokenMesh->SetSimulatePhysics(true);

	//	const FVector ImpulseDirection = HitDirection.GetSafeNormal();

	//	BrokenMesh->AddImpulseAtLocation(ImpulseDirection * BreakImpulse, HitLocation, NAME_None);
	//}

	//if (bDestroyActorAfterBreak)
	//{
	//	SetLifeSpan(BrokenMeshLifeTime);
	//}
	//else if (bHideBrokenMeshAfterDelay)
	//{
	//	GetWorldTimerManager().SetTimer(
	//		HideBrokenMeshTimerHandle,
	//		this,
	//		&ADestructibleObject::HideBrokenMesh,
	//		BrokenMeshLifeTime,
	//		false
	//	);
	//}

	// <New>

	if (bDestroyed)
	{
		return;
	}

	StopBurning();

	bDestroyed = true;
	CurrentHealth = 0.f;

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

	if (!bIsDestructible)
	{
		return;
	}

	if (IntactMesh)
	{
		IntactMesh->SetVisibility(false);
		IntactMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		IntactMesh->SetSimulatePhysics(false);
	}

	switch (DestructionMeshType)
	{
	case EDestructionMeshType::StaticMesh:
		BreakStaticMesh(HitLocation, HitDirection);
		break;

	case EDestructionMeshType::GeometryCollection:
		BreakGeometryCollection(HitLocation, HitDirection);
		break;

	default:
		break;
	}

	if (bDestroyActorAfterBreak)
	{
		SetLifeSpan(BrokenMeshLifeTime);
	}
	else if (bHideBrokenMeshAfterDelay && BrokenMeshLifeTime > 0.f)
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

	if (FracturedMesh)
	{
		FracturedMesh->SetVisibility(false);
		FracturedMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		FracturedMesh->SetSimulatePhysics(false);
	}
}

void ADestructibleObject::StartBurning(AActor* FireCauser)
{
	if (bDestroyed) return;
	if (!bCanBurn) return;
	if (bBurning) return;

	bBurning = true;
	BurnCauser = FireCauser;

	if (FireAreaComponent)
	{
		FireAreaComponent->ActivateFireArea(FireCauser);
	}

	if (FireEffect)
	{
		ActiveFireEffect = UNiagaraFunctionLibrary::SpawnSystemAttached(
			FireEffect,
			IntactMesh,
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::KeepRelativeOffset,
			true
		);
	}

	if (FireStartSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			GetWorld(),
			FireStartSound,
			GetActorLocation()
		);
	}

	if (FireLoopSound)
	{
		ActiveFireLoopSound = UGameplayStatics::SpawnSoundAttached(
			FireLoopSound,
			GetRootComponent()
		);
	}

	GetWorldTimerManager().SetTimer(
		BurnDamageTimerHandle,
		this,
		&ADestructibleObject::ApplyBurnDamage,
		BurnDamageInterval,
		true
	);

	GetWorldTimerManager().SetTimer(
		BurnDurationTimerHandle,
		this,
		&ADestructibleObject::StopBurning,
		BurnDuration,
		false
	);
}

void ADestructibleObject::StopBurning()
{
	if (!bBurning) return;

	bBurning = false;
	BurnCauser = nullptr;

	GetWorldTimerManager().ClearTimer(BurnDamageTimerHandle);
	GetWorldTimerManager().ClearTimer(BurnDurationTimerHandle);

	if (FireAreaComponent)
	{
		FireAreaComponent->DeactivateFireArea();
	}

	if (ActiveFireEffect)
	{
		ActiveFireEffect->Deactivate();
		ActiveFireEffect = nullptr;
	}

	if (ActiveFireLoopSound)
	{
		ActiveFireLoopSound->Stop();
		ActiveFireLoopSound = nullptr;
	}
}

void ADestructibleObject::ApplyBurnDamage()
{
	if (bDestroyed)
	{
		StopBurning();
		return;
	}

	EGameDamageType::Fire;

	FDamageParams Damage;
	Damage.DamageAmount = BurnDamage;
	Damage.ImpactPoint = GetActorLocation();
	Damage.HitDirection = FVector::UpVector;
	Damage.DamageCauser = BurnCauser;

	ReceiveDamage(Damage);
}


void ADestructibleObject::BreakStaticMesh(
	const FVector& HitLocation,
	const FVector& HitDirection)
{
	if (!BrokenMesh)
	{
		return;
	}

	if (IntactMesh)
	{
		BrokenMesh->SetWorldTransform(
			IntactMesh->GetComponentTransform()
		);
	}

	BrokenMesh->SetVisibility(true);
	BrokenMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BrokenMesh->SetSimulatePhysics(true);

	const FVector ImpulseDirection = HitDirection.GetSafeNormal();

	BrokenMesh->AddImpulseAtLocation(
		ImpulseDirection * BreakImpulse,
		HitLocation
	);
}


void ADestructibleObject::BreakGeometryCollection(
	const FVector& HitLocation,
	const FVector& HitDirection)
{
	if (!FracturedMesh || !FracturedMesh->GetRestCollection())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Geometry Collection is not configured: %s"),
			*GetName()
		);
		return;
	}

	if (IntactMesh)
	{
		FracturedMesh->SetWorldTransform(
			IntactMesh->GetComponentTransform()
		);
	}

	FracturedMesh->SetVisibility(true);
	FracturedMesh->SetCollisionEnabled(
		ECollisionEnabled::QueryAndPhysics
	);

	FracturedMesh->SetSimulatePhysics(true);

	// 충돌 지점을 중심으로 클러스터 연결을 파괴합니다.
	FracturedMesh->ApplyExternalStrain(
		INDEX_NONE,
		HitLocation,
		FractureDamageRadius,
		0,
		1.f,
		FractureStrain
	);

	// 파편이 충격 방향으로 움직이도록 힘을 적용합니다.
	const FVector ImpulseDirection =
		HitDirection.GetSafeNormal();

	if (!ImpulseDirection.IsNearlyZero())
	{
		FracturedMesh->AddImpulseAtLocation(
			ImpulseDirection * FractureImpulseStrength,
			HitLocation
		);
	}
}