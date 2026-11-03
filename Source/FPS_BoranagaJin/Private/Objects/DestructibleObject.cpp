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
#include "Field/FieldSystemObjects.h"


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
	StopZeroGravityFracture();
	GetWorldTimerManager().ClearTimer(HideBrokenMeshTimerHandle);

	if (!bDestroyed)
	{
		SetActorHiddenInGame(false);
		SetActorEnableCollision(true);
		if (IntactMesh)
		{
			IntactMesh->SetVisibility(true);
			IntactMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		}
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
		return;
	}

	if (IntactMesh)
	{
		IntactMesh->SetVisibility(false);
		IntactMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		IntactMesh->SetSimulatePhysics(false);
	}

	if (!bBrokenMeshHidden && bIsDestructible)
	{
		if (DestructionMeshType == EDestructionMeshType::StaticMesh)
		{
			BreakStaticMesh(GetActorLocation(), FVector::ZeroVector);
		}
		else if (DestructionMeshType == EDestructionMeshType::GeometryCollection)
		{
			BreakGeometryCollection(GetActorLocation(), FVector::UpVector, false);
		}
	}

	if (bDestroyActorAfterBreak)
	{
		SetLifeSpan(BrokenMeshLifeTime);
	}
	else if (!bBrokenMeshHidden && bHideBrokenMeshAfterDelay && BrokenMeshLifeTime > 0.f)
	{
		GetWorldTimerManager().SetTimer(HideBrokenMeshTimerHandle, this,
			&ADestructibleObject::HideBrokenMesh, BrokenMeshLifeTime, false);
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
	if (bDestroyed || !bIsDestructible) return 0.f;
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
	bBrokenMeshHidden = false;
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
	GetWorldTimerManager().ClearTimer(FragmentStartTimerHandle);
	GetWorldTimerManager().ClearTimer(FragmentUpdateTimerHandle);
	bBrokenMeshHidden = true;
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
	const FVector& HitDirection,
	bool bPlayFractureAnimation)
{
	if (!IsValid(FracturedMesh) || !FracturedMesh->GetRestCollection())
	{
		UE_LOG(LogTemp, Warning, TEXT("Geometry Collection is not configured: %s"), *GetName());
		return;
	}

	if (IntactMesh)
	{
		FracturedMesh->SetWorldTransform(IntactMesh->GetComponentTransform());
	}

	FracturedMesh->SetVisibility(true);
	FracturedMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	FracturedMesh->SetEnableGravity(!bUseZeroGravityFracture);
	FracturedMesh->SetSimulatePhysics(true);

	FracturedMesh->ApplyExternalStrain(
		INDEX_NONE, HitLocation, FractureDamageRadius, 0, 1.f, FractureStrain);

	if (!bPlayFractureAnimation)
	{
		// Load restores the broken state, not the old particle transforms.
		// Gravity remains disabled in zero-gravity mode.
		return;
	}

	if (bUseZeroGravityFracture)
	{
		StartZeroGravityFracture(HitLocation);
	}
	else
	{
		const FVector Direction = HitDirection.GetSafeNormal();
		if (!Direction.IsNearlyZero())
		{
			FracturedMesh->AddImpulseAtLocation(
				Direction * FractureImpulseStrength, HitLocation);
		}
	}
}

void ADestructibleObject::StartZeroGravityFracture(const FVector& Origin)
{
	if (!IsValid(FracturedMesh)) return;

	GetWorldTimerManager().ClearTimer(FragmentStartTimerHandle);
	GetWorldTimerManager().ClearTimer(FragmentUpdateTimerHandle);

	FragmentSpreadOrigin = Origin;
	FragmentSpreadElapsed = 0.f;
	FragmentAngularAxis = FMath::VRand();
	FracturedMesh->SetEnableGravity(false);

	GetWorldTimerManager().SetTimer(
		FragmentStartTimerHandle, this,
		&ADestructibleObject::UpdateZeroGravityFracture,
		FMath::Max(0.01f, FragmentStartDelay), false);
}

void ADestructibleObject::ApplyFragmentLinearVelocity(const FVector& Origin, float Speed)
{
	if (!IsValid(FracturedMesh)) return;

	URadialVector* RadialField = NewObject<URadialVector>(this);
	RadialField->SetRadialVector(Speed, Origin);
	FracturedMesh->ApplyPhysicsField(
		true, EGeometryCollectionPhysicsTypeEnum::Chaos_LinearVelocity,
		nullptr, RadialField);
}

void ADestructibleObject::ApplyFragmentAngularVelocity(float Speed)
{
	if (!IsValid(FracturedMesh)) return;

	UUniformVector* AngularField = NewObject<UUniformVector>(this);
	AngularField->SetUniformVector(Speed, FragmentAngularAxis);
	FracturedMesh->ApplyPhysicsField(
		true, EGeometryCollectionPhysicsTypeEnum::Chaos_AngularVelocity,
		nullptr, AngularField);
}

void ADestructibleObject::UpdateZeroGravityFracture()
{
	if (!IsValid(FracturedMesh) || !bDestroyed || bBrokenMeshHidden)
	{
		GetWorldTimerManager().ClearTimer(FragmentUpdateTimerHandle);
		return;
	}

	const float Duration = FMath::Max(0.01f, FragmentSpreadDuration);
	const float Alpha = FMath::Clamp(FragmentSpreadElapsed / Duration, 0.f, 1.f);
	const float Ratio = FMath::Square(1.f - Alpha);

	ApplyFragmentLinearVelocity(FragmentSpreadOrigin, FragmentSpreadSpeed * Ratio);
	ApplyFragmentAngularVelocity(FragmentAngularSpeed * Ratio);

	if (Alpha >= 1.f)
	{
		StopZeroGravityFracture();
		return;
	}

	const float Interval = FMath::Max(0.01f, FragmentUpdateInterval);
	FragmentSpreadElapsed = FMath::Min(Duration, FragmentSpreadElapsed + Interval);
	GetWorldTimerManager().SetTimer(
		FragmentUpdateTimerHandle, this,
		&ADestructibleObject::UpdateZeroGravityFracture, Interval, false);
}

void ADestructibleObject::StopZeroGravityFracture()
{
	GetWorldTimerManager().ClearTimer(FragmentStartTimerHandle);
	GetWorldTimerManager().ClearTimer(FragmentUpdateTimerHandle);

	if (bFreezeFragmentsAfterSpread && bDestroyed && !bBrokenMeshHidden && IsValid(FracturedMesh))
	{
		ApplyFragmentLinearVelocity(FragmentSpreadOrigin, 0.f);
		ApplyFragmentAngularVelocity(0.f);
	}
}

void ADestructibleObject::RestoreDestructibleObject()
{
	GetWorldTimerManager().ClearTimer(FragmentStartTimerHandle);
	GetWorldTimerManager().ClearTimer(FragmentUpdateTimerHandle);
	GetWorldTimerManager().ClearTimer(HideBrokenMeshTimerHandle);
	StopBurning();
	SetLifeSpan(0.f);

	// The Geometry Collection's broken particle transforms are NOT reset by
	// SetSimulatePhysics(false). Full reuse requires recreating the GC component
	// (or a version-specific supported dynamic-collection reset path).
	if (FracturedMesh)
	{
		FracturedMesh->SetVisibility(false);
		FracturedMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		FracturedMesh->SetSimulatePhysics(false);
	}
	if (BrokenMesh)
	{
		BrokenMesh->SetVisibility(false);
		BrokenMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		BrokenMesh->SetSimulatePhysics(false);
	}

	CurrentHealth = MaxHealth;
	bDestroyed = false;
	bBrokenMeshHidden = false;
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	if (IntactMesh)
	{
		IntactMesh->SetVisibility(true);
		IntactMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		IntactMesh->SetSimulatePhysics(false);
	}
}
