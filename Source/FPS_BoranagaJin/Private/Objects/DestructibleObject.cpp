#include "Objects/DestructibleObject.h"
#include "Objects/FireAreaComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/PrimitiveComponent.h"
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


void ADestructibleObject::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();

	// This delegate also detects direct edits to the IntactMesh component
	// in a Blueprint viewport and runtime changes made by gameplay code.
	if (IsValid(IntactMesh))
	{
		IntactMesh->TransformUpdated.RemoveAll(this);
		IntactMesh->TransformUpdated.AddUObject(
			this, &ADestructibleObject::HandleIntactMeshTransformUpdated);
	}
}

void ADestructibleObject::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	SynchronizeDestructionMeshTransforms();
}

void ADestructibleObject::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(IntactMesh))
	{
		IntactMesh->TransformUpdated.RemoveAll(this);
	}

	Super::EndPlay(EndPlayReason);
}

#if WITH_EDITOR
void ADestructibleObject::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Called when bSynchronizeMeshTransforms changes in actor Details.
	// Component transform edits also reach the TransformUpdated delegate.
	SynchronizeDestructionMeshTransforms();
}
#endif

void ADestructibleObject::HandleIntactMeshTransformUpdated(
	USceneComponent* UpdatedComponent,
	EUpdateTransformFlags UpdateTransformFlags,
	ETeleportType Teleport)
{
	if (UpdatedComponent == IntactMesh)
	{

		UE_LOG(LogTemp, Error, TEXT("void ADestructibleObject::HandleIntactMeshTransformUpdated !!!"));
		SynchronizeDestructionMeshTransforms();
	}
}

void ADestructibleObject::SetMeshTransformSynchronizationEnabled(bool bEnabled)
{
	bSynchronizeMeshTransforms = bEnabled;
	SynchronizeDestructionMeshTransforms();
}

void ADestructibleObject::SynchronizeOneDestructionMesh(USceneComponent* TargetMesh)
{
	if (!bSynchronizeMeshTransforms || !IsValid(IntactMesh) || !IsValid(TargetMesh) ||
		TargetMesh == IntactMesh || bSynchronizingMeshTransforms)
	{
		return;
	}

	// Never teleport a running Chaos collection or a simulated broken mesh.
	if (const UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(TargetMesh))
	{
		if (Primitive->IsSimulatingPhysics()) { return; }
	}

	TGuardValue<bool> SyncGuard(bSynchronizingMeshTransforms, true);

	// Matching parents: make the editable relative transforms identical.
	// Different parents: preserve an identical world transform instead.
	if (TargetMesh->GetAttachParent() == IntactMesh->GetAttachParent() &&
		TargetMesh->IsUsingAbsoluteLocation() == IntactMesh->IsUsingAbsoluteLocation() &&
		TargetMesh->IsUsingAbsoluteRotation() == IntactMesh->IsUsingAbsoluteRotation() &&
		TargetMesh->IsUsingAbsoluteScale() == IntactMesh->IsUsingAbsoluteScale())
	{
		const FTransform SourceRelative = IntactMesh->GetRelativeTransform();
		if (!TargetMesh->GetRelativeTransform().Equals(SourceRelative))
		{
			TargetMesh->SetRelativeTransform(SourceRelative, false, nullptr,
				ETeleportType::TeleportPhysics);
		}
	}
	else
	{
		const FTransform SourceWorld = IntactMesh->GetComponentTransform();
		if (!TargetMesh->GetComponentTransform().Equals(SourceWorld))
		{
			TargetMesh->SetWorldTransform(SourceWorld, false, nullptr,
				ETeleportType::TeleportPhysics);
		}
	}
}

void ADestructibleObject::SynchronizeDestructionMeshTransforms()
{
	// Once fractured, Chaos owns the fragment transforms until restoration.
	// BreakStaticMesh/BreakGeometryCollection explicitly position their mesh
	// just before enabling physics, including during save-game load.
	if (!bSynchronizeMeshTransforms || bDestroyed || bSynchronizingMeshTransforms)
	{
		return;
	}

	SynchronizeOneDestructionMesh(BrokenMesh);
	SynchronizeOneDestructionMesh(FracturedMesh);
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

void ADestructibleObject::OnBeforeSave()
{
	// Called once through the virtual SaveableActorInterface hierarchy.
	// Health, destruction, burning and hidden state already have SaveGame UPROPERTYs.
	CurrentDurability = FMath::Max(0.f, CurrentHealth);
	SavedObjectRuntimeState.Version = 1;

	// Some derived actors (e.g. ExitDoor/ItemPickUp) already use
	// FWorldActorSaveData.ActorTransform via ShouldSaveTransform() == true.
	// Do not serialize the actor transform a second time for those actors.
	SavedObjectRuntimeState.bHasActorWorldTransform = !ShouldSaveTransform();
	SavedObjectRuntimeState.ActorWorldTransform =
		SavedObjectRuntimeState.bHasActorWorldTransform
		? GetActorTransform() : FTransform::Identity;

	FTimerManager& Timers = GetWorldTimerManager();
	SavedObjectRuntimeState.BrokenMeshHideTimeRemaining =
		bDestroyed && !bBrokenMeshHidden
		? Timers.GetTimerRemaining(HideBrokenMeshTimerHandle) : -1.f;
	SavedObjectRuntimeState.ActorLifeSpanRemaining =
		bDestroyed && bDestroyActorAfterBreak && GetLifeSpan() > 0.f
		? GetLifeSpan() : -1.f;
	SavedObjectRuntimeState.BurnDurationTimeRemaining =
		bBurning && !bDestroyed
		? Timers.GetTimerRemaining(BurnDurationTimerHandle) : -1.f;
	SavedObjectRuntimeState.BurnDamageTimeRemaining =
		bBurning && !bDestroyed
		? Timers.GetTimerRemaining(BurnDamageTimerHandle) : -1.f;

	const bool bSaveStaticDebris = bDestroyed && !bBrokenMeshHidden &&
		DestructionMeshType == EDestructionMeshType::StaticMesh &&
		IsValid(BrokenMesh) && BrokenMesh->IsSimulatingPhysics();
	SavedObjectRuntimeState.bHasBrokenStaticMeshPhysics = bSaveStaticDebris;
	SavedObjectRuntimeState.BrokenStaticMeshWorldTransform =
		bSaveStaticDebris ? BrokenMesh->GetComponentTransform() : FTransform::Identity;
	SavedObjectRuntimeState.BrokenStaticMeshLinearVelocity =
		bSaveStaticDebris ? BrokenMesh->GetPhysicsLinearVelocity() : FVector::ZeroVector;
	SavedObjectRuntimeState.BrokenStaticMeshAngularVelocity =
		bSaveStaticDebris ? BrokenMesh->GetPhysicsAngularVelocityInRadians() : FVector::ZeroVector;
}

void ADestructibleObject::OnAfterLoad()
{
	const bool bHasRuntimeSnapshot = SavedObjectRuntimeState.Version >= 1;
	const bool bShouldResumeFire = bBurning && !bDestroyed && bCanBurn;

	FTimerManager& Timers = GetWorldTimerManager();
	StopBurning(); // Clear stale fire effects/timers even when the loaded flag is false.
	StopZeroGravityFracture();
	Timers.ClearTimer(HideBrokenMeshTimerHandle);
	SetLifeSpan(0.f); // Clear a previous destruction countdown if reloading in-place.

	// The save subsystem restores ActorTransform only for ShouldSaveTransform()==true.
	// Other destructible objects persist their world transform exactly once here.
	if (bHasRuntimeSnapshot && SavedObjectRuntimeState.bHasActorWorldTransform &&
		!ShouldSaveTransform())
	{
		SetActorTransform(SavedObjectRuntimeState.ActorWorldTransform,
			false, nullptr, ETeleportType::TeleportPhysics);
	}

	CurrentHealth = FMath::Clamp(CurrentHealth, 0.f, FMath::Max(0.f, MaxHealth));
	if (bDestroyed) { CurrentHealth = 0.f; }
	CurrentDurability = CurrentHealth;

	if (IsValid(BrokenMesh))
	{
		BrokenMesh->SetSimulatePhysics(false);
		BrokenMesh->SetVisibility(false);
		BrokenMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (IsValid(FracturedMesh))
	{
		FracturedMesh->SetSimulatePhysics(false);
		FracturedMesh->SetVisibility(false);
		FracturedMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);

	if (!bDestroyed)
	{
		if (IsValid(IntactMesh))
		{
			IntactMesh->SetSimulatePhysics(false);
			IntactMesh->SetVisibility(true);
			IntactMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		}
		SynchronizeDestructionMeshTransforms();

		if (bShouldResumeFire)
		{
			// The saved bBurning bool is an authoritative state flag; effects
			// and timer handles are runtime-only and must be recreated.
			StartBurning(nullptr);

			if (bBurning && bHasRuntimeSnapshot)
			{
				const float BurnTime = SavedObjectRuntimeState.BurnDurationTimeRemaining;
				if (BurnTime >= 0.f)
				{
					Timers.SetTimer(BurnDurationTimerHandle, this,
						&ADestructibleObject::StopBurning,
						FMath::Max(0.01f, BurnTime), false);
				}

				const float DamageTime = SavedObjectRuntimeState.BurnDamageTimeRemaining;
				if (DamageTime >= 0.f)
				{
					const float Rate = FMath::Max(0.01f, BurnDamageInterval);
					Timers.SetTimer(BurnDamageTimerHandle, this,
						&ADestructibleObject::ApplyBurnDamage, Rate, true,
						FMath::Max(0.01f, DamageTime));
				}
			}
		}
		return;
	}

	if (IsValid(IntactMesh))
	{
		IntactMesh->SetVisibility(false);
		IntactMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		IntactMesh->SetSimulatePhysics(false);
	}

	// Recreate the destroyed visual state. No break sound/effect or new damage.
	if (!bBrokenMeshHidden && bIsDestructible)
	{
		switch (DestructionMeshType)
		{
		case EDestructionMeshType::StaticMesh:
			BreakStaticMesh(GetActorLocation(), FVector::ZeroVector);
			if (bHasRuntimeSnapshot &&
				SavedObjectRuntimeState.bHasBrokenStaticMeshPhysics &&
				IsValid(BrokenMesh) && BrokenMesh->IsSimulatingPhysics())
			{
				BrokenMesh->SetWorldTransform(
					SavedObjectRuntimeState.BrokenStaticMeshWorldTransform,
					false, nullptr, ETeleportType::TeleportPhysics);
				BrokenMesh->SetPhysicsLinearVelocity(
					SavedObjectRuntimeState.BrokenStaticMeshLinearVelocity);
				BrokenMesh->SetPhysicsAngularVelocityInRadians(
					SavedObjectRuntimeState.BrokenStaticMeshAngularVelocity);
			}
			break;

		case EDestructionMeshType::GeometryCollection:
			// Current Chaos APIs do not restore independent shard positions here.
			BreakGeometryCollection(GetActorLocation(), FVector::UpVector, false);
			break;
		default:
			break;
		}
	}

	if (bDestroyActorAfterBreak)
	{
		// Actors already removed from the level cannot be rediscovered by the
		// current WorldActorData actor iterator; only existing actors reach here.
		const float SavedRemaining = bHasRuntimeSnapshot
			? SavedObjectRuntimeState.ActorLifeSpanRemaining : -1.f;
		const float Remaining = SavedRemaining > 0.f
			? SavedRemaining : BrokenMeshLifeTime;
		if (Remaining > 0.f) { SetLifeSpan(Remaining); }
	}
	else if (!bBrokenMeshHidden && bHideBrokenMeshAfterDelay && BrokenMeshLifeTime > 0.f)
	{
		const float SavedRemaining = bHasRuntimeSnapshot
			? SavedObjectRuntimeState.BrokenMeshHideTimeRemaining : -1.f;
		const float Remaining = SavedRemaining >= 0.f
			? FMath::Max(0.01f, SavedRemaining) : BrokenMeshLifeTime;
		Timers.SetTimer(HideBrokenMeshTimerHandle, this,
			&ADestructibleObject::HideBrokenMesh, Remaining, false);
	}
}

void ADestructibleObject::SetRuntimeSpawned(bool bIsRuntimeSpawned)
{
	bRuntimeSpawned = bIsRuntimeSpawned;
}

void ADestructibleObject::BeginPlay()
{
	Super::BeginPlay();

	SynchronizeDestructionMeshTransforms();

	CurrentHealth = MaxHealth;
	CurrentDurability = CurrentHealth;

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

	CurrentHealth = FMath::Max(0.f, CurrentHealth - DamageData.DamageAmount);
	CurrentDurability = CurrentHealth;

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

	// Capture any last runtime/editor transform before the intact state disappears.
	SynchronizeDestructionMeshTransforms();
	bDestroyed = true;
	bBrokenMeshHidden = false;
	CurrentHealth = 0.f;
	CurrentDurability = 0.f;

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
	// Also cleans up when deserialization overwrote bBurning with false.

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

	// Position exactly at the intact mesh before enabling rigid-body physics.
	// Disabled synchronization preserves the designer's independent offset.
	SynchronizeOneDestructionMesh(BrokenMesh);

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

	// Align the collection's initial transform before Chaos simulates pieces.
	SynchronizeOneDestructionMesh(FracturedMesh);

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
	CurrentDurability = CurrentHealth;
	bDestroyed = false;
	bBrokenMeshHidden = false;
	// Physics is off; restore the authored mesh alignment for the next break.
	SynchronizeDestructionMeshTransforms();
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	if (IntactMesh)
	{
		IntactMesh->SetVisibility(true);
		IntactMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		IntactMesh->SetSimulatePhysics(false);
	}
}
