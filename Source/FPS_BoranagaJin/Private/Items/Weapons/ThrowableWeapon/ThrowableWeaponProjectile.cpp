#include "Items/Weapons/ThrowableWeapon/ThrowableWeaponProjectile.h"
#include "Items/Weapons/ThrowableWeapon/ThrowableWeapon.h"
#include "Characters/DamageParams.h"
#include "Interface/DamageInterface.h"
#include "ObjectPoolSubsystem.h"
#include "SoundSystem/SurfaceTypeName.h"

#include "Components/SphereComponent.h"
#include "Components/DecalComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Perception/AISense_Hearing.h"

AThrowableWeaponProjectile::AThrowableWeaponProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	CollisionComp->SetCollisionProfileName(TEXT("PlayerProjectile"));
	CollisionComp->SetCollisionObjectType(ECC_GameTraceChannel3);
	CollisionComp->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Ignore);
	CollisionComp->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Ignore);
	CollisionComp->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Ignore);
	CollisionComp->SetCollisionResponseToChannel(ECC_GameTraceChannel5, ECR_Ignore);
	CollisionComp->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	CollisionComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionComp->bReturnMaterialOnMove = true;
	CollisionComp->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Unwalkable, 0.f));
	CollisionComp->CanCharacterStepUpOn = ECB_No;
	RootComponent = CollisionComp;

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
	ProjectileMovement->UpdatedComponent = CollisionComp;
	ProjectileMovement->InitialSpeed = 1000.f;
	ProjectileMovement->MaxSpeed = 0.f;
	ProjectileMovement->bInitialVelocityInLocalSpace = false;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = true;
	ProjectileMovement->Bounciness = 0.35f;
	ProjectileMovement->Friction = 0.6f;
	ProjectileMovement->BounceVelocityStopSimulatingThreshold = 10.f;
	ProjectileMovement->ProjectileGravityScale = 1.f;
	ProjectileMovement->bForceSubStepping = true;
	ProjectileMovement->bAutoActivate = false;
	ProjectileMovement->BounceAdditionalIterations = 4;
	ProjectileMovement->bBounceAngleAffectsFriction = false;
	ProjectileMovement->OnProjectileBounce.AddDynamic(this, &AThrowableWeaponProjectile::HandleProjectileBounce);
	ProjectileMovement->OnProjectileStop.AddDynamic(this, &AThrowableWeaponProjectile::HandleProjectileStop);

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProjectileMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	ProjectileMesh->SetCollisionObjectType(ECC_GameTraceChannel3);
	ProjectileMesh->SetCastShadow(false);

	InitialLifeSpan = 0.f;
	SetActorHiddenInGame(true);
	SetActorTickEnabled(false);
}

void AThrowableWeaponProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsActiveInPool)
	{
		return;
	}

	UpdateTrailEffect();

	if (bIsHitScan && bActivatedMeshMovementForHitScan)
	{
		UpdateHitScanProjectileMovement(DeltaTime);
	}

	if (bUseCustomProjectieMovement)
	{
		UpdateProjectileMovement(DeltaTime);
	}

	// A throwable is allowed to come to rest while its fuse is still running.
	// The old stuck check would incorrectly return a resting grenade to the pool.
	if (!bUseThrowableBehavior)
	{
		CheckAndDeactivateIfStuck();
	}
}

void AThrowableWeaponProjectile::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (TrailEffectComponent)
	{
		if (bShouldUpdateTrailEffect)
		{
			TrailEffectComponent->Deactivate();
			TrailEffectComponent->DestroyComponent();
		}
	}
}

void AThrowableWeaponProjectile::BeginDestroy()
{
	Super::BeginDestroy();
}

void AThrowableWeaponProjectile::FellOutOfWorld(const UDamageType& dmgType)
{
	SetActorLocation(FVector::ZeroVector);
	SetActorRotation(FRotator::ZeroRotator);

	DeactivateProjectile();
}

void AThrowableWeaponProjectile::OutsideWorldBounds()
{
	SetActorLocation(FVector::ZeroVector);
	SetActorRotation(FRotator::ZeroRotator);

	DeactivateProjectile();
}

void AThrowableWeaponProjectile::InitProjectile(AActor* OwnerOfProjectile, AThrowableWeapon* OwnerWeapon, float additonalDamage, int32 NumPenetrable, bool HitScan)
{
	// Data must be loaded before trail spawning, movement setup, fuse setup, and bounce setup.
	LoadProjectileData();

	bIsActivated = true;
	bPayloadActivated = false;
	bUseThrowableBehavior = !HitScan;
	bUseCustomProjectieMovement = false; // Never mix custom Tick movement with built-in movement on a thrown object.

	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);

	ProjectileOwner = IsValid(OwnerOfProjectile) ? OwnerOfProjectile : nullptr;
	ThrowableWeapon = IsValid(OwnerWeapon) ? OwnerWeapon : nullptr;
	AdditionalDamage = additonalDamage;

	ConfigureFlightCollision();

	if (ProjectileMesh)
	{
		ProjectileMesh->SetVisibility(true);
	}

	if (ProjectileMovement && CollisionComp)
	{
		ProjectileMovement->Deactivate();
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->SetUpdatedComponent(CollisionComp);
		ProjectileMovement->bSimulationEnabled = true;
		ProjectileMovement->Velocity = FVector::ZeroVector;
	}

	if (TrailEffect)
	{
		SpawnTrailEffect();
	}

	if (HitScan)
	{
		bIsHitScan = true;
		NumPenetrableObjects = NumPenetrable;
		InitHitScan();
	}
	else
	{
		bIsHitScan = false;

		if (!CollisionComp->OnComponentHit.IsAlreadyBound(this, &AThrowableWeaponProjectile::OnHit))
		{
			CollisionComp->OnComponentHit.AddDynamic(this, &AThrowableWeaponProjectile::OnHit);
		}

		// Preserve the legacy penetration path when explicitly requested.
		if (NumPenetrable > 0)
		{
			NumPenetrableObjects = NumPenetrable;
			InitPhysicsProjectile();
		}
	}

	CurrentRicochetCount = 0;
	NumPenetratedObjects = 0;
	StuckCount = 0;
	DistanceMoved = 0.f;
}

void AThrowableWeaponProjectile::InitPhysicsProjectile()
{
	if (!CollisionComp->OnComponentHit.IsAlreadyBound(this, &AThrowableWeaponProjectile::OnHit))
	{
		CollisionComp->OnComponentHit.AddDynamic(this, &AThrowableWeaponProjectile::OnHit);
	}
	if (!CollisionComp->OnComponentBeginOverlap.IsAlreadyBound(this, &AThrowableWeaponProjectile::OnComponentBeginOverlap))
	{
		CollisionComp->OnComponentBeginOverlap.AddDynamic(this, &AThrowableWeaponProjectile::OnComponentBeginOverlap);
	}
	CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionComp->SetCollisionResponseToChannel(ECC_GameTraceChannel6, ECR_Overlap);
}

void AThrowableWeaponProjectile::InitHitScan()
{
	CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionComp->SetCollisionResponseToChannel(ECC_GameTraceChannel6, ECR_Overlap);
}

void AThrowableWeaponProjectile::LoadProjectileData()
{
	ProjectileData = nullptr;

	if (bUseDataTable)
	{
		ProjectileData = ProjectileDataTableHandle.GetRow<FThrowableWeaponProjectileData>(TEXT("AThrowableWeaponProjectile::LoadProjectileData"));
	}
	else
	{
		ProjectileData = &BlueprintProjectileData;
	}

	if (!ProjectileData)
	{
		UE_LOG(LogTemp, Warning, TEXT("Throwable projectile data is invalid: %s"), *GetName());
		return;
	}

	ApplyProjectileData();
}

void AThrowableWeaponProjectile::SetWeapon(AThrowableWeapon* NewWeapon)
{
	ThrowableWeapon = NewWeapon;
}

void AThrowableWeaponProjectile::LaunchProjectile(FVector StartPos, FRotator Direction)
{
	SetActorLocationAndRotation(StartPos, Direction);
	ProjectileMovement->Activate();
}

void AThrowableWeaponProjectile::LaunchProjectile(const FVector& StartPosition, const FVector& Direction, float LaunchSpeed)
{
	if (!IsValid(ProjectileMovement) || !IsValid(CollisionComp))
	{
		return;
	}

	const FVector NormalizedDirection = Direction.GetSafeNormal();
	if (NormalizedDirection.IsNearlyZero())
	{
		return;
	}

	SetActorLocationAndRotation(StartPosition, NormalizedDirection.Rotation());

	ProjectileMovement->Deactivate();
	ProjectileMovement->StopMovementImmediately();
	ProjectileMovement->SetUpdatedComponent(CollisionComp);
	ProjectileMovement->bSimulationEnabled = true;
	bUseCustomProjectieMovement = false;
	ProjectileMovement->bShouldBounce = bUseThrowableBehavior || bCanSimpleBounce;
	ProjectileMovement->Velocity = NormalizedDirection * LaunchSpeed;
	ProjectileMovement->Activate(true);

	UE_LOG(LogTemp, Log, TEXT("[ThrowableBounce] Launch: Bounce=%d Active=%d Sim=%d Updated=%s Velocity=%s"),
		ProjectileMovement->bShouldBounce,
		ProjectileMovement->IsActive(),
		ProjectileMovement->bSimulationEnabled,
		*GetNameSafe(ProjectileMovement->UpdatedComponent),
		*ProjectileMovement->Velocity.ToString());

	StartFuse();
	StartLifeTimer(LifeSpan);

	UE_LOG(LogTemp, Error, TEXT("Throwable launched. Type=%s Speed=%.1f GravityScale=%.2f"),
		*UEnum::GetValueAsString(ProjectileType),
		LaunchSpeed,
		ProjectileMovement->ProjectileGravityScale);
}

void AThrowableWeaponProjectile::HandleProjectileBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	if (!bUseThrowableBehavior || !bIsActivated || bPayloadActivated) return;

	UE_LOG(LogTemp, Log, TEXT("[ThrowableBounce] Bounce: Surface=%s Normal=%s Incoming=%s Outgoing=%s"),
		*GetNameSafe(ImpactResult.GetActor()),
		*ImpactResult.ImpactNormal.ToString(),
		*ImpactVelocity.ToString(),
		ProjectileMovement ? *ProjectileMovement->Velocity.ToString() : TEXT("None"));
}

void AThrowableWeaponProjectile::HandleProjectileStop(const FHitResult& ImpactResult)
{
	if (!bUseThrowableBehavior || !bIsActivated || bPayloadActivated) return;

	UE_LOG(LogTemp, Warning, TEXT("[ThrowableBounce] Movement stopped: Surface=%s Bounce=%d Sim=%d Velocity=%s"),
		*GetNameSafe(ImpactResult.GetActor()),
		ProjectileMovement ? ProjectileMovement->bShouldBounce : false,
		ProjectileMovement ? ProjectileMovement->bSimulationEnabled : false,
		ProjectileMovement ? *ProjectileMovement->Velocity.ToString() : TEXT("None"));
}

void AThrowableWeaponProjectile::StartLifeTimer(float Seconds)
{
	if (Seconds > 0.f)
	{
		TWeakObjectPtr WeakThis = this;
		GetWorld()->GetTimerManager().SetTimer(LifeTimer, FTimerDelegate::CreateWeakLambda(this, [WeakThis]()
			{
				if (auto* HardThis = WeakThis.Get())
				{
					HardThis->DeactivateProjectile();
				}
			}), Seconds, false);
	}
}

void AThrowableWeaponProjectile::StopLifeTimer()
{
	GetWorldTimerManager().ClearTimer(LifeTimer);
}

void AThrowableWeaponProjectile::ApplyExplosiveDamage(bool bCanExplosiveDamage, FVector CenterLocation)
{
	if (!bCanExplosiveDamage || MaxExplosionRadius <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	TArray<AActor*> OverlappedActors;
	if (!FindActorsInRadius(CenterLocation, MaxExplosionRadius, OverlappedActors, bCanAffectOwner))
	{
		return;
	}

	for (AActor* OverlappedActor : OverlappedActors)
	{
		if (!IsValid(OverlappedActor) || OverlappedActor == this)
		{
			continue;
		}

		if (bExplosionRequiresLineOfSight && !HasEffectLineOfSight(CenterLocation, OverlappedActor))
		{
			continue;
		}

		const float DistanceToTarget = FVector::Distance(CenterLocation, OverlappedActor->GetActorLocation());
		const float DistanceAlpha = 1.f - FMath::Clamp(DistanceToTarget / MaxExplosionRadius, 0.f, 1.f);
		const float FalloffAlpha = FMath::Pow(DistanceAlpha, FMath::Max(ExplosionDamageFalloffExponent, 0.01f));
		const float DamageAmount = FMath::Lerp(MinExplosiveDamage, MaxExplosiveDamage, FalloffAlpha);

		ApplyDamage(
			OverlappedActor,
			DamageAmount,
			EGameDamageType::Explosion,
			true,
			NAME_None,
			SurfaceType1,
			(OverlappedActor->GetActorLocation() - CenterLocation).GetSafeNormal(),
			OverlappedActor->GetActorLocation());

		if (OnBodyShot.IsBound())
		{
			OnBodyShot.Execute();
		}
	}

	if (bVisualizeExplosionRadius)
	{
		DrawSphere(CenterLocation, MaxExplosionRadius);
	}
}

void AThrowableWeaponProjectile::ApplyDamage(AActor* OtherActor, float DamageAmount, EGameDamageType DamageType, bool bCanForceDamage, const FName BoneName, TEnumAsByte<EPhysicalSurface> SurfaceType, const FVector ImpulseDirection, const FVector ImpactPoint)
{
	FDamageParams Damage; //TODO: ��ź ��ġ
	Damage.DamageAmount = DamageAmount;
	Damage.DamageType = DamageType;
	Damage.bCanForceDamage = bCanForceDamage;
	Damage.HitBoneName = BoneName;
	Damage.ImpulseDirection = ImpulseDirection;
	Damage.SurfaceType = SurfaceType;
	Damage.ImpactPoint = ImpactPoint;
	Damage.DamageCauser = this->ProjectileOwner;

	if (OtherActor->GetClass()->ImplementsInterface(UDamageableInterface::StaticClass()))
	{
		Cast<IDamageableInterface>(OtherActor)->ReceiveDamage(Damage);
	}
}

bool AThrowableWeaponProjectile::SearchOverlappedActor(FVector CenterLocation, float SearchRadius, TArray<AActor*>& OverlappedActors)
{
	TArray<TEnumAsByte<EObjectTypeQuery>> traceObjectTypes;
	traceObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));
	traceObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_GameTraceChannel6));
	TArray<AActor*> ignoreActors;
	ignoreActors.Init(ProjectileOwner, 1);
	bool bIsAnyActorExist = UKismetSystemLibrary::SphereOverlapActors(GetWorld(), CenterLocation, SearchRadius, traceObjectTypes, nullptr, ignoreActors, OverlappedActors);

	return bIsAnyActorExist;
}

void AThrowableWeaponProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{

	if (bUseThrowableBehavior)
	{
		if (!bIsActivated || bPayloadActivated)
		{
			return;
		}

		if (OtherActor == ProjectileOwner)
		{
			return;
		}

		// OnHit is expected on every bounce. Do not deactivate a normal grenade here.
		if (Hit.bBlockingHit)
		{
			PlaySoundAtLocationByMaterial(UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get()), Hit.ImpactPoint);
			ReportNoiseToAI();
		}

		if (bDetonateOnImpact)
		{
			ActivatePayload();
		}

		return;
	}


	UE_LOG(LogTemp, Error, TEXT("void AThrowableWeaponProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)"));

	if (NumPenetrableObjects > 0)
	{
		SpawnImpactEffect(Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
		SpawnDecalEffect(Hit.ImpactPoint, Hit.ImpactNormal.Rotation());

		if (bShouldUpdateTrailEffect)
		{
			if (TrailEffectComponent)
			{
				TrailEffectComponent->Deactivate();
				TrailEffectComponent->DestroyComponent();
				TrailEffectComponent = nullptr;
			}
		}


	}
	else
	{
		if (OtherActor != nullptr)
		{
			if (OtherActor != ProjectileOwner)
			{
				// Only add impulse and destroy projectile if we hit a physics
				if ((OtherActor != nullptr) && (OtherActor != this) && (OtherComp != nullptr) && OtherComp->IsSimulatingPhysics())
				{
					OtherComp->AddImpulseAtLocation(GetVelocity() * 100.0f, GetActorLocation());
				}

				SpawnImpactEffect(Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
				SpawnDecalEffect(Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
				if (!bShouldUpdateTrailEffect)
				{
					if (TrailEffectComponent)
					{
						TrailEffectComponent->Deactivate();
						TrailEffectComponent->DestroyComponent();
						TrailEffectComponent = nullptr;
					}
				}

				ReportNoiseToAI();

				//TODO: SoundSystem�� ����
				PlaySoundAtLocationByMaterial(UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get()), Hit.ImpactPoint);

				if (HeadShotAdditionalDamage > 0.f && CheckHeadHit(Hit))
				{
					ApplyDamage(OtherActor, DefaultDamage + AdditionalDamage + HeadShotAdditionalDamage, EGameDamageType::Melee, false, Hit.BoneName, UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get()), Hit.ImpactNormal, Hit.ImpactPoint);

					if (OnHeadShot.IsBound())
					{
						OnHeadShot.Execute();
					}
				}
				else
				{
					ApplyDamage(OtherActor, DefaultDamage + AdditionalDamage, EGameDamageType::Melee, false, Hit.BoneName, UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get()), Hit.ImpactNormal, Hit.ImpactPoint);
					//UE_LOG(LogTemp, Error, TEXT("bone11-1: %s"), *Hit.BoneName.ToString());
					if (Cast<ACharacter>(OtherActor))
					{
						if (OnBodyShot.IsBound())
						{
							OnBodyShot.Execute();
						}
					}
				}

				ApplyExplosiveDamage(bIsExplosive, Hit.ImpactPoint);

				if (bCanApplyImpulseToEnemy)
				{
					AddImpulseToEnemy(OtherActor, GetVelocity().GetSafeNormal() * HitImpulseToEnemy);
				}

				if (Cast<ACharacter>(OtherActor))
				{
					DeactivateProjectile();
				}
				else
				{
					if (bCanSimpleBounce && CurrentRicochetCount < MaxRicochetCount && CheckRicochetAngle(Hit.ImpactNormal, ProjectileMovement->Velocity))
					{
						CurrentRicochetCount++;
					}
					else
					{
						DeactivateProjectile();
					}
				}
			}
		}
		else //TODO: �̰��� �ݵ�� �ʿ��� �б��ΰ�?
		{
			SpawnImpactEffect(Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
			SpawnDecalEffect(Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
			DeactivateProjectile();
		}
	}
}

void AThrowableWeaponProjectile::OnComponentBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (NumPenetrableObjects > 0)
	{
		if (OtherActor == nullptr) { return; }
		if (OtherActor != ProjectileOwner)
		{
			if ((OtherActor != this) && (OtherComp != nullptr) && OtherComp->IsSimulatingPhysics())
			{
				OtherComp->AddImpulseAtLocation(GetVelocity() * 100.0f, GetActorLocation());
			}

			if (HeadShotAdditionalDamage > 0.f && CheckHeadOvelap(OtherActor, SweepResult))
			{
				SpawnImpactEffect(SweepResult.ImpactPoint, SweepResult.ImpactNormal.Rotation());
				ApplyDamage(OtherActor, DefaultDamage + AdditionalDamage + HeadShotAdditionalDamage, EGameDamageType::Melee, false, SweepResult.BoneName, UPhysicalMaterial::DetermineSurfaceType(SweepResult.PhysMaterial.Get()), SweepResult.ImpactNormal, SweepResult.ImpactPoint);

				if (OnHeadShot.IsBound())
				{
					OnHeadShot.Execute();
				}
			}
			else
			{
				SpawnImpactEffect(SweepResult.ImpactPoint, SweepResult.ImpactNormal.Rotation());
				ApplyDamage(OtherActor, DefaultDamage + AdditionalDamage, EGameDamageType::Melee, false, SweepResult.BoneName, UPhysicalMaterial::DetermineSurfaceType(SweepResult.PhysMaterial.Get()), SweepResult.ImpactNormal, SweepResult.ImpactPoint);
				if (Cast<ACharacter>(OtherActor))
				{
					if (OnBodyShot.IsBound())
					{
						OnBodyShot.Execute();
					}
				}
			}

			ApplyExplosiveDamage(bIsExplosive, SweepResult.ImpactPoint);

			if (bCanApplyImpulseToEnemy)
			{
				AddImpulseToEnemy(OtherActor, GetVelocity().GetSafeNormal() * HitImpulseToEnemy);
			}

			UpdatePenetration();
			if (NumPenetratedObjects > NumPenetrableObjects)
			{
				ResetPenetration();


				if (bShouldUpdateTrailEffect)
				{
					TrailEffectComponent->Deactivate();
					TrailEffectComponent->DestroyComponent();
				}

				DeactivateProjectile();
			}
		}
	}
}

void AThrowableWeaponProjectile::SpawnImpactEffect(FVector SpawnLocation, FRotator SpawnRotation)
{
	if (ImpactEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ImpactEffect, SpawnLocation, SpawnRotation, FVector(1.0f), true, true, ENCPoolMethod::AutoRelease);
	}
}
void AThrowableWeaponProjectile::SpawnExplosionEffect(FVector SpawnLocation)
{
	if (ExplosionEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ExplosionEffect, SpawnLocation, FRotator::ZeroRotator, FVector(1.0f), true, true, ENCPoolMethod::AutoRelease);
	}
}
void AThrowableWeaponProjectile::SpawnTrailEffect(bool bShouldAttachedToWeapon)
{
	if (ProjectileMesh && TrailEffect)
	{
		FTransform TrailStartTransform = ProjectileMesh->GetSocketTransform(FName(TEXT("TrailStart")), ERelativeTransformSpace::RTS_Component);
		FTransform TrailEndTransform = ProjectileMesh->GetSocketTransform(FName(TEXT("TrailEnd")), ERelativeTransformSpace::RTS_Component);
		FVector TrailLocationOffset = (TrailEndTransform.GetLocation() - TrailStartTransform.GetLocation()).GetSafeNormal() * TrailOffsetDist;

		if (bShouldAttachedToWeapon) //TODO: ��� ����
		{
			TrailEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), TrailEffect,
				ThrowableWeapon->GetMuzzlePointTransform().GetLocation(),
				FRotator(0.f, 0.f, 0.f), FVector(1), true, true, ENCPoolMethod::AutoRelease);

			bShouldUpdateTrailEffect = true;

			TrailEffectComponent->SetVectorParameter(FName(TEXT("Beam End")), ProjectileMesh->GetSocketLocation(FName(TEXT("TrailStart"))));
		}
		else
		{
			TrailEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
				TrailEffect,
				ProjectileMesh,
				FName(TEXT("TrailStart")),
				TrailLocationOffset,
				FRotator(0, 0, 0),
				EAttachLocation::KeepRelativeOffset,
				true, true, ENCPoolMethod::AutoRelease);
		}
	}
}
void AThrowableWeaponProjectile::SpawnDecalEffect(FVector SpawnLocation, FRotator SpawnRotation)
{
	if (ImpactDecalMaterial)
	{
		FVector DecalSize = FVector(2.0f, 8.0f, 8.0f);

		UDecalComponent* ProjectileDecal =
			UGameplayStatics::SpawnDecalAtLocation(
				GetWorld(),
				ImpactDecalMaterial,
				DecalSize,
				SpawnLocation,
				SpawnRotation,
				10.0f
			);
		ProjectileDecal->SetFadeScreenSize(0.0001f);
	}
}

void AThrowableWeaponProjectile::UpdateTrailEffect() //TODO: Is this necessary?
{
	if (bShouldUpdateTrailEffect)
	{
		if (TrailEffectComponent)
		{
			TrailEffectComponent->SetVectorParameter(FName(TEXT("Beam End")), ProjectileMesh->GetSocketLocation(FName(TEXT("TrailStart"))));
		}
	}
}

void AThrowableWeaponProjectile::DrawSphere(FVector Location, float Radius)
{
	DrawDebugSphere(
		GetWorld(),
		Location,
		Radius,
		12,
		FColor::Red,
		false,
		5.0f,
		0,
		2.0f
	);
}

void AThrowableWeaponProjectile::PlaySoundAtLocationByMaterial(EPhysicalSurface SurfaceType, FVector Location) //TODO: SoundSystem�� ����
{
	USoundBase* SoundToPlay = nullptr;

	switch (SurfaceType)
	{
	case SURFACE_DEFAULT: SoundToPlay = HitSound_Default; break;
	case SURFACE_METAL:   SoundToPlay = HitSound_Metal;  break;
	case SURFACE_GLASS:   SoundToPlay = HitSound_Glass;  break;
	case SURFACE_ENEMY:
	case SURFACE_HEAD:
	case SURFACE_BODY:
	case SURFACE_LEFT_ARM:
	case SURFACE_RIGHT_ARM:
		SoundToPlay = HitSound_Enemy;
		break;
	case SURFACE_ENERGY:  SoundToPlay = HitSound_Energy; break;
	default:              break;
	}

	if (SoundToPlay)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SoundToPlay, Location);
	}
}

void AThrowableWeaponProjectile::PerformHitScan(FVector StartLocation, FVector TraceDirection, float MaxDistance, float SphereRadius, TArray<FVector>& OutHitLocations)
{
	FVector Start = StartLocation;
	FVector Direction = TraceDirection;
	FVector End = StartLocation + TraceDirection * MaxDistance;

	TArray<FVector> HitStaticLocations;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(ProjectileOwner);
	Params.AddIgnoredComponent(ProjectileMesh);
	Params.AddIgnoredActor(this);
	Params.bReturnPhysicalMaterial = true;

	FCollisionResponseParams ResponseParams;
	ResponseParams.CollisionResponse.SetResponse(ECC_GameTraceChannel6, ECR_Overlap); //PawnEnemy

	FCollisionShape Sphere = FCollisionShape::MakeSphere(SphereRadius);

	for (int32 RicochetCount = 0; RicochetCount <= MaxRicochetCount; RicochetCount++)
	{
		TArray<FHitResult> TempHitResults;

		bool bHit = GetWorld()->SweepMultiByChannel(
			TempHitResults,
			Start,
			End,
			FQuat::Identity,
			ECC_GameTraceChannel3,
			Sphere,
			Params,
			ResponseParams
		);

		if (bDebugHitScan) { DrawDebugLine(GetWorld(), Start, End, FColor::Blue, false, 10.f); }

		bool bIsBlockedByWorldStatic = false;

		if (bHit)
		{
			TArray<AActor*> OnceDamagedEnemies;
			for (const FHitResult& HitResult : TempHitResults)
			{
				if (NumPenetratedObjects <= NumPenetrableObjects)
				{
					ACharacter* Enemy = Cast<ACharacter>(HitResult.GetActor());
					if (Enemy && !OnceDamagedEnemies.Contains(Enemy))
					{
						OnceDamagedEnemies.AddUnique(Enemy);

						if (HeadShotAdditionalDamage > 0.f && CheckHeadHit(HitResult))
						{
							SpawnImpactEffect(HitResult.ImpactPoint, (-HitResult.ImpactNormal).Rotation());
							ApplyDamage(HitResult.GetActor(), DefaultDamage + AdditionalDamage + HeadShotAdditionalDamage,
								EGameDamageType::Melee, false, HitResult.BoneName, UPhysicalMaterial::DetermineSurfaceType(HitResult.PhysMaterial.Get()), TraceDirection, HitResult.ImpactPoint);

							if (OnHeadShot.IsBound())
							{
								OnHeadShot.Execute();
							}
						}
						else
						{
							SpawnImpactEffect(HitResult.ImpactPoint, (-HitResult.ImpactNormal).Rotation());
							ApplyDamage(HitResult.GetActor(), DefaultDamage + AdditionalDamage, EGameDamageType::Melee, false, HitResult.BoneName, UPhysicalMaterial::DetermineSurfaceType(HitResult.PhysMaterial.Get()), TraceDirection, HitResult.ImpactPoint);
							//UE_LOG(LogTemp, Error, TEXT("bone11-2: %s"), *HitResult.BoneName.ToString());
							if (OnBodyShot.IsBound())
							{
								OnBodyShot.Execute();
							}
						}

						UpdatePenetration();
					}
				}

				if (HitResult.GetComponent()->GetCollisionObjectType() == ECC_WorldStatic)
				{
					HitStaticLocations.Add(HitResult.ImpactPoint);

					if (bDebugHitScan) { DrawDebugSphere(GetWorld(), HitResult.ImpactPoint, 20.f, 12, FColor::Red, false, 50.f); }

					Start = HitResult.ImpactPoint;

					if (CheckRicochetAngle(HitResult.ImpactNormal, Direction))
					{
						Direction = GetReflectionAngle(HitResult.ImpactNormal, Direction);
						Start = Start + Direction.GetSafeNormal() * (SphereRadius + 1.f);
						End = Start + Direction * MaxDistance;
						CurrentRicochetCount++;
					}
					else
					{
						RicochetCount = MaxRicochetCount + 1;
					}
					SpawnImpactEffect(HitResult.ImpactPoint, (-HitResult.ImpactNormal).Rotation());
					bIsBlockedByWorldStatic = true;
					break;
				}
			}
		}

		if (!bHit || !bIsBlockedByWorldStatic)
		{
			HitStaticLocations.Add(End);
			break;
		}
	}

	OutHitLocations = HitStaticLocations;
}
void AThrowableWeaponProjectile::InitHitScanProjectileMovement(FVector StartLocation)
{
	bActivatedMeshMovementForHitScan = true;

	if (!HitScanEndPoints.IsEmpty())
	{
		//FVector CurrLocation = GetActorLocation();
		FVector TargetLocation = HitScanEndPoints[0];
		MovementDirection = (TargetLocation - StartLocation).GetSafeNormal();
		SetActorLocationAndRotation(StartLocation, MovementDirection.Rotation());
		//SetActorRotation(MovementDirection.Rotation());
		TargetDistance = FVector::Dist(StartLocation, TargetLocation);
		CurrEndPointIdx = 0;
	}
}
void AThrowableWeaponProjectile::UpdateHitScanProjectileMovement(float DeltaTime)
{
	FVector CurrLocation = GetActorLocation();
	FVector TargetLocation = HitScanEndPoints[CurrEndPointIdx];

	FVector DeltaPosition = MovementDirection * HitScanProjectileVelocity * DeltaTime;

	CurrLocation = CurrLocation + DeltaPosition;
	SetActorLocation(CurrLocation);

	DistanceMoved += DeltaPosition.Length();

	if ((TargetLocation - CurrLocation).IsNearlyZero() || TargetDistance <= DistanceMoved)
	{
		if (CurrEndPointIdx + 1 > HitScanEndPoints.Num() - 1)
		{
			//TODO: �״�� ����? Destroy?
			if (TrailEffectComponent)
			{
				TrailEffectComponent->Deactivate();
				TrailEffectComponent->DestroyComponent();
				TrailEffectComponent = nullptr;
			}
			DeactivateProjectile();
		}
		else
		{
			CurrEndPointIdx++;
			CurrLocation = TargetLocation;
			SetActorLocation(CurrLocation);
			TargetLocation = HitScanEndPoints[CurrEndPointIdx];
			MovementDirection = (TargetLocation - CurrLocation).GetSafeNormal();
			SetActorRotation(MovementDirection.Rotation());
			TargetDistance = FVector::Dist(CurrLocation, TargetLocation);
			DistanceMoved = 0;
		}
	}
}

void AThrowableWeaponProjectile::SetHitScanActive(bool bflag)
{
	bIsHitScan = bflag;
}
void AThrowableWeaponProjectile::LaunchHitScan(FVector StartLocation, FVector TraceDirection, FVector MuzzlePos)
{
	//PerformHitScan(StartLocation, TraceDirection, 50000.f, ProjectileRadius, HitScanEndPoints); //TODO: MaxDistnace �����ؾ���
	PerformHitScan(StartLocation, TraceDirection, 50000.f, ProjectileRadius, HitScanEndPoints); //TODO: MaxDistnace �����ؾ���

	InitHitScanProjectileMovement(MuzzlePos);
}

void AThrowableWeaponProjectile::UpdatePenetration()
{
	NumPenetratedObjects++;
}
void AThrowableWeaponProjectile::ResetPenetration()
{
	NumPenetratedObjects = 0;
}

bool AThrowableWeaponProjectile::CheckHeadHit(const FHitResult& Hit)
{
	//UE_LOG(LogTemp, Error, TEXT("FName: %s"), *HitResult.BoneName.ToString());
	if (Hit.BoneName == "head")
	{
		return true;
	}
	return false;
}
bool AThrowableWeaponProjectile::CheckHeadOvelap(const AActor* OverlappedActor, const FHitResult& SweepResult)
{
	if (!OverlappedActor) return false;

	USkeletalMeshComponent* SkeletalMesh = OverlappedActor->GetComponentByClass<USkeletalMeshComponent>();

	if (SkeletalMesh && SkeletalMesh->DoesSocketExist(FName(TEXT("head"))))
	{
		if (CollisionComp->GetScaledSphereRadius() > FVector::Distance(SweepResult.ImpactPoint, SkeletalMesh->GetBoneLocation(FName(TEXT("head")))))
		{
			//UE_LOG(LogTemp, Error, TEXT("Head Shot!!!"));
			return true;
		}
	}
	return false;
}

void AThrowableWeaponProjectile::AddImpulseToEnemy(AActor* OtherActor, FVector Force)
{
	if (OtherActor != nullptr && IsValid(OtherActor))
	{
		ACharacter* Enemy = Cast<ACharacter>(OtherActor);
		if (Enemy)
		{
			Enemy->LaunchCharacter(Force, false, false);
		}
	}
}

bool AThrowableWeaponProjectile::CheckRicochetAngle(FVector normal, FVector vel)
{
	return FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(-normal.GetSafeNormal(), vel.GetSafeNormal()))) > MinIncidenceAngle;
}

FVector AThrowableWeaponProjectile::GetReflectionAngle(FVector normal, FVector input)
{
	FVector norm = normal.GetSafeNormal();
	FVector in = input.GetSafeNormal();

	return in - 2 * (FVector::DotProduct(in, norm) * norm);
}

void AThrowableWeaponProjectile::InitProjectileMovement(FVector StartPos, FVector Direction, FVector MuzzlePos)
{
	bUseCustomProjectieMovement = true;

	ProjectileMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	//ProjectileMesh->SetUsing

	//TODO: Dettach MeshComponent from Collision(Root)Component	
	PM_Cam_Pos = StartPos;
	PM_Dir = Direction.GetSafeNormal();

	PM_d_Pos = PM_Cam_Pos + PM_Dir * PM_Cam_To_d_Len;
	PM_Dir_d_To_Muzzle = (MuzzlePos - PM_d_Pos).GetSafeNormal();

	PM_Start_To_d_Len = FMath::Abs(FVector::DotProduct((-PM_Dir), PM_Dir_d_To_Muzzle)) * FVector::Distance(PM_d_Pos, MuzzlePos);
	PM_Start_Pos = PM_Cam_Pos + PM_Dir * (PM_Cam_To_d_Len - PM_Start_To_d_Len);

	PM_k_by_d = FVector::Distance(PM_d_Pos, MuzzlePos) / PM_Start_To_d_Len;

	//------------------------------
	ProjectileMesh->SetVisibility(true);
	DistanceMoved = 0.f;
	SetActorLocation(PM_Start_Pos);

	FVector MeshTargetLocation = PM_Start_Pos + (PM_d_Pos - PM_Start_Pos) + PM_Dir_d_To_Muzzle * (PM_d_Pos - PM_Start_Pos).Length() * PM_k_by_d;
	ProjectileMesh->SetWorldLocationAndRotation(MeshTargetLocation, (-PM_Dir_d_To_Muzzle).Rotation());

	////TODO: Draw Debug Sphere
	//DrawDebugLine(
	//	GetWorld(),
	//	PM_Start_Pos,
	//	PM_Start_Pos + PM_Dir * 1000.f,
	//	FColor::Red,
	//	false,
	//	50.f);

	//DrawDebugLine(
	//	GetWorld(),
	//	MuzzlePos,
	//	MuzzlePos + (-1) * PM_Dir_d_To_Muzzle * FVector::Distance(PM_d_Pos, MuzzlePos),
	//	FColor::Blue,
	//	false,
	//	50.f);
}
void AThrowableWeaponProjectile::UpdateProjectileMovement(float DeltaTime)
{
	FVector CurrLocation = GetActorLocation();

	FVector DeltaPos = PM_Dir * PM_Vel * DeltaTime;

	FVector CollisionTargetLocation = CurrLocation + DeltaPos;

	DistanceMoved += DeltaPos.Length();

	//SetActorLocation(CollisionTargetLocation);
	AddActorWorldOffset(DeltaPos, /*bSweep=*/true);

	if (DistanceMoved < PM_Start_To_d_Len)
	{
		FVector MeshTargetLocation = CollisionTargetLocation + (PM_d_Pos - CollisionTargetLocation) + PM_Dir_d_To_Muzzle * (PM_d_Pos - CollisionTargetLocation).Length() * PM_k_by_d;
		ProjectileMesh->SetWorldLocationAndRotation(MeshTargetLocation, (-PM_Dir_d_To_Muzzle).Rotation());
	}
	else
	{
		//TODO: Attach MeshComponent to Collision Component (Once)
		ProjectileMesh->SetWorldLocationAndRotation(CollisionTargetLocation, PM_Dir.Rotation());
	}
}

void AThrowableWeaponProjectile::CheckAndDeactivateIfStuck()
{
	if (bIsActivated)
	{
		if ((PrevProjectileLoc - GetActorLocation()).IsNearlyZero())
		{
			StuckCount++;
		}
		else
		{
			StuckCount = 0;
		}

		if (StuckCount > MaxStuckCount)
		{
			UE_LOG(LogTemp, Error, TEXT("Projectile is Stuck!!!"));
			DeactivateProjectile();
		}
		else
		{
			PrevProjectileLoc = GetActorLocation();
		}
	}
}

void AThrowableWeaponProjectile::ReportNoiseToAI()
{
	UAISense_Hearing::ReportNoiseEvent(
		GetWorld(),
		GetActorLocation(),
		ProjectileHitNoiseLoudness,
		this,
		ProjectileHitNoiseRange,
		TEXT("ProjectileHit")
	);
}


void AThrowableWeaponProjectile::ApplyProjectileData()
{
	if (!ProjectileData)
	{
		return;
	}

	ProjectileType = ProjectileData->ProjectileType;
	FuseTime = ProjectileData->FuseTime;
	LifeSpan = ProjectileData->InitialLifeSpan;
	bDetonateOnImpact = ProjectileData->bDetonateOnImpact;
	bCanAffectOwner = ProjectileData->bCanAffectOwner;

	TrailEffect = ProjectileData->TrailEffect;
	ImpactEffect = ProjectileData->ImpactEffect;
	ExplosionEffect = ProjectileData->ExplosionEffect;
	FlashEffect = ProjectileData->FlashEffect;
	SmokeEffect = ProjectileData->SmokeEffect;
	ImpactDecalMaterial = ProjectileData->ImpactDecal;
	TrailOffsetDist = ProjectileData->TrailOffsetDist;

	HitSound_Default = ProjectileData->HitSound_Default;
	HitSound_Metal = ProjectileData->HitSound_Metal;
	HitSound_Glass = ProjectileData->HitSound_Glass;
	HitSound_Enemy = ProjectileData->HitSound_Enemy;
	HitSound_Energy = ProjectileData->HitSound_Energy;
	DetonationSound = ProjectileData->DetonationSound;
	FlashSound = ProjectileData->FlashSound;
	SmokeStartSound = ProjectileData->SmokeStartSound;

	DefaultDamage = ProjectileData->DefaultDamage;
	HeadShotAdditionalDamage = ProjectileData->HeadShotAdditionalDamage;

	bIsExplosive = ProjectileData->bIsExplosive;
	bVisualizeExplosionRadius = ProjectileData->bVisualizeExplosionRadius;
	MaxExplosiveDamage = ProjectileData->MaxExplosiveDamage;
	MinExplosiveDamage = ProjectileData->MinExplosiveDamage;
	MaxExplosionRadius = ProjectileData->MaxExplosionRadius;
	ExplosionDamageFalloffExponent = ProjectileData->ExplosionDamageFalloffExponent;
	bExplosionRequiresLineOfSight = ProjectileData->bExplosionRequiresLineOfSight;

	FlashRadius = ProjectileData->FlashRadius;
	FlashMinDuration = ProjectileData->FlashMinDuration;
	FlashMaxDuration = ProjectileData->FlashMaxDuration;
	FlashDistanceFalloffExponent = ProjectileData->FlashDistanceFalloffExponent;
	bFlashRequiresLineOfSight = ProjectileData->bFlashRequiresLineOfSight;
	bFlashFadeAudio = ProjectileData->bFlashFadeAudio;

	SmokeDuration = ProjectileData->SmokeDuration;
	SmokeRadius = ProjectileData->SmokeRadius;
	bSmokeBlocksVisibilityTrace = ProjectileData->bSmokeBlocksVisibilityTrace;

	InitialSpeed = ProjectileData->InitialSpeed;
	InitialRadius = ProjectileData->InitialRadius;
	ProjectileRadius = ProjectileData->InitialRadius;
	bCanSimpleBounce = ProjectileData->bCanSimpleBounce;

	bCanApplyImpulseToEnemy = ProjectileData->bCanApplyImpulseToEnemy;
	HitImpulseToEnemy = ProjectileData->HitImpulseToEnemy;
	MaxRicochetCount = ProjectileData->MaxRicochetCount;
	MinIncidenceAngle = ProjectileData->MinIncidenceAngle;
	bDebugHitScan = ProjectileData->bDebugHitScan;
	PM_Cam_To_d_Len = ProjectileData->PM_Cam_To_d_Len;

	if (CollisionComp)
	{
		CollisionComp->SetSphereRadius(InitialRadius);
	}

	if (ProjectileMovement)
	{
		ProjectileMovement->InitialSpeed = ProjectileData->InitialSpeed;
		ProjectileMovement->MaxSpeed = ProjectileData->MaxSpeed;
		ProjectileMovement->ProjectileGravityScale = ProjectileData->ProjectileGravityScale;
		// Throwable projectiles must bounce on blocking hits; a legacy DT row may have bCanSimpleBounce=false.
		// Keep the DT field for non-throwable legacy use, but enforce bounce for physical throws.
		ProjectileMovement->bShouldBounce = bUseThrowableBehavior || ProjectileData->bCanSimpleBounce;
		ProjectileMovement->Bounciness = ProjectileData->Bounciness;
		ProjectileMovement->Friction = ProjectileData->Friction;
		ProjectileMovement->BounceVelocityStopSimulatingThreshold = ProjectileData->BounceVelocityStopSimulatingThreshold;
		ProjectileMovement->bForceSubStepping = ProjectileData->bForceSubStepping;
		ProjectileMovement->bRotationFollowsVelocity = true;
		ProjectileMovement->bInitialVelocityInLocalSpace = false;
	}

	PM_Vel = ProjectileData->MaxSpeed > 0.f ? ProjectileData->MaxSpeed : ProjectileData->InitialSpeed;
	HitScanProjectileVelocity = ProjectileData->InitialSpeed;
}

void AThrowableWeaponProjectile::ConfigureFlightCollision()
{
	if (!CollisionComp)
	{
		return;
	}

	CollisionComp->SetCollisionProfileName(TEXT("PlayerProjectile"));
	CollisionComp->SetCollisionObjectType(ECC_GameTraceChannel3);
	CollisionComp->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Ignore);
	CollisionComp->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Ignore);
	CollisionComp->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Ignore);
	CollisionComp->SetCollisionResponseToChannel(ECC_GameTraceChannel4, ECR_Ignore);
	CollisionComp->SetCollisionResponseToChannel(ECC_GameTraceChannel5, ECR_Ignore);
	CollisionComp->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	CollisionComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComp->SetGenerateOverlapEvents(true);
	CollisionComp->SetSphereRadius(InitialRadius);
}

void AThrowableWeaponProjectile::StopProjectileMovement()
{
	if (!ProjectileMovement)
	{
		return;
	}

	ProjectileMovement->StopMovementImmediately();
	ProjectileMovement->Velocity = FVector::ZeroVector;
	ProjectileMovement->Deactivate();
}

void AThrowableWeaponProjectile::StopTrailEffect()
{
	if (!TrailEffectComponent)
	{
		return;
	}

	TrailEffectComponent->Deactivate();
	TrailEffectComponent->DestroyComponent();
	TrailEffectComponent = nullptr;
	bShouldUpdateTrailEffect = false;
}

void AThrowableWeaponProjectile::ClearThrowableTimers()
{
	if (!GetWorld())
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(FuseTimerHandle);
	GetWorldTimerManager().ClearTimer(PersistentEffectTimerHandle);
}

void AThrowableWeaponProjectile::StartFuse()
{
	if (bPayloadActivated)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(FuseTimerHandle);

	if (FuseTime <= KINDA_SMALL_NUMBER)
	{
		ActivatePayload();
		return;
	}

	GetWorldTimerManager().SetTimer(
		FuseTimerHandle,
		this,
		&AThrowableWeaponProjectile::OnFuseExpired,
		FuseTime,
		false);
}

void AThrowableWeaponProjectile::OnFuseExpired()
{
	ActivatePayload();
}

void AThrowableWeaponProjectile::ActivatePayload()
{
	if (bPayloadActivated || !bIsActivated)
	{
		return;
	}

	bPayloadActivated = true;

	GetWorldTimerManager().ClearTimer(FuseTimerHandle);
	StopLifeTimer();
	StopProjectileMovement();
	StopTrailEffect();

	const FVector ActivationLocation = GetActorLocation();

	switch (ProjectileType)
	{
	case EThrowableProjectileType::Grenade:
		ActivateGrenadePayload();
		break;

	case EThrowableProjectileType::Flashbang:
		ActivateFlashbangPayload();
		break;

	case EThrowableProjectileType::SmokeGrenade:
		ActivateSmokePayload();
		break;

	default:
		break;
	}

	BP_OnPayloadActivated(ProjectileType, ActivationLocation);

	if (ProjectileType != EThrowableProjectileType::SmokeGrenade)
	{
		DeactivateProjectile();
	}
}

void AThrowableWeaponProjectile::ActivateGrenadePayload()
{
	const FVector CenterLocation = GetActorLocation();

	SpawnExplosionEffect(CenterLocation);

	if (DetonationSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, DetonationSound, CenterLocation);
	}

	ApplyExplosiveDamage(bIsExplosive, CenterLocation);
	ReportNoiseToAI();
}

void AThrowableWeaponProjectile::ActivateFlashbangPayload()
{
	const FVector CenterLocation = GetActorLocation();

	if (FlashEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			FlashEffect,
			CenterLocation,
			FRotator::ZeroRotator,
			FVector::OneVector,
			true,
			true,
			ENCPoolMethod::AutoRelease);
	}

	if (FlashSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FlashSound, CenterLocation);
	}

	ApplyFlashEffect(CenterLocation);
	ReportNoiseToAI();
}

void AThrowableWeaponProjectile::ActivateSmokePayload()
{
	const FVector CenterLocation = GetActorLocation();

	if (ProjectileMesh)
	{
		ProjectileMesh->SetVisibility(false);
	}

	if (SmokeEffect)
	{
		SmokeEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			SmokeEffect,
			CenterLocation,
			FRotator::ZeroRotator,
			FVector::OneVector,
			false,
			true,
			ENCPoolMethod::None);
	}

	if (SmokeStartSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SmokeStartSound, CenterLocation);
	}

	if (CollisionComp)
	{
		CollisionComp->SetSphereRadius(SmokeRadius);
		CollisionComp->SetCollisionResponseToAllChannels(ECR_Ignore);

		if (bSmokeBlocksVisibilityTrace)
		{
			CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			CollisionComp->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		}
		else
		{
			CollisionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}

	BP_OnSmokeStarted(CenterLocation, SmokeRadius, SmokeDuration);
	ReportNoiseToAI();

	if (SmokeDuration <= KINDA_SMALL_NUMBER)
	{
		FinishSmokeEffect();
		return;
	}

	GetWorldTimerManager().SetTimer(
		PersistentEffectTimerHandle,
		this,
		&AThrowableWeaponProjectile::FinishSmokeEffect,
		SmokeDuration,
		false);
}

void AThrowableWeaponProjectile::FinishSmokeEffect()
{
	GetWorldTimerManager().ClearTimer(PersistentEffectTimerHandle);

	if (SmokeEffectComponent)
	{
		SmokeEffectComponent->Deactivate();
		SmokeEffectComponent->DestroyComponent();
		SmokeEffectComponent = nullptr;
	}

	DeactivateProjectile();
}

void AThrowableWeaponProjectile::ApplyFlashEffect(const FVector& CenterLocation)
{
	if (FlashRadius <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	TArray<AActor*> AffectedActors;
	if (!FindActorsInRadius(CenterLocation, FlashRadius, AffectedActors, bCanAffectOwner))
	{
		return;
	}

	for (AActor* AffectedActor : AffectedActors)
	{
		if (!IsValid(AffectedActor) || AffectedActor == this)
		{
			continue;
		}

		if (bFlashRequiresLineOfSight && !HasEffectLineOfSight(CenterLocation, AffectedActor))
		{
			continue;
		}

		const float Intensity = CalculateFlashIntensity(CenterLocation, AffectedActor);
		if (Intensity <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		const float Duration = FMath::Lerp(FlashMinDuration, FlashMaxDuration, Intensity);

		if (APawn* Pawn = Cast<APawn>(AffectedActor))
		{
			if (APlayerController* PlayerController = Cast<APlayerController>(Pawn->GetController()))
			{
				if (PlayerController->IsLocalController() && PlayerController->PlayerCameraManager)
				{
					PlayerController->PlayerCameraManager->StartCameraFade(
						FMath::Clamp(Intensity, 0.f, 1.f),
						0.f,
						Duration,
						FLinearColor::White,
						bFlashFadeAudio,
						false);
				}
			}
		}

		// Project-specific AI stun / perception suppression can be connected here in BP
		// without creating separate C++ projectile subclasses.
		BP_OnFlashActorAffected(AffectedActor, Intensity, Duration);
	}
}

float AThrowableWeaponProjectile::CalculateFlashIntensity(const FVector& CenterLocation, const AActor* TargetActor) const
{
	if (!IsValid(TargetActor) || FlashRadius <= KINDA_SMALL_NUMBER)
	{
		return 0.f;
	}

	const float Distance = FVector::Distance(CenterLocation, TargetActor->GetActorLocation());
	const float DistanceAlpha = 1.f - FMath::Clamp(Distance / FlashRadius, 0.f, 1.f);

	return FMath::Pow(
		DistanceAlpha,
		FMath::Max(FlashDistanceFalloffExponent, 0.01f));
}

bool AThrowableWeaponProjectile::HasEffectLineOfSight(const FVector& CenterLocation, const AActor* TargetActor) const
{
	if (!IsValid(TargetActor) || !GetWorld())
	{
		return false;
	}

	FVector TargetLocation;
	FRotator TargetRotation;
	TargetActor->GetActorEyesViewPoint(TargetLocation, TargetRotation);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ThrowableEffectLineOfSight), false);
	QueryParams.AddIgnoredActor(this);

	if (IsValid(ProjectileOwner))
	{
		QueryParams.AddIgnoredActor(ProjectileOwner);
	}

	FHitResult Hit;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit,
		CenterLocation,
		TargetLocation,
		ECC_Visibility,
		QueryParams);

	return !bHit || Hit.GetActor() == TargetActor;
}

bool AThrowableWeaponProjectile::FindActorsInRadius(
	const FVector& CenterLocation,
	float SearchRadius,
	TArray<AActor*>& OutActors,
	bool bIncludeOwner) const
{
	if (!GetWorld() || SearchRadius <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_PhysicsBody));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel6));

	TArray<AActor*> IgnoreActors;
	IgnoreActors.Add(const_cast<AThrowableWeaponProjectile*>(this));

	if (!bIncludeOwner && IsValid(ProjectileOwner))
	{
		IgnoreActors.Add(ProjectileOwner);
	}

	return UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		CenterLocation,
		SearchRadius,
		ObjectTypes,
		nullptr,
		IgnoreActors,
		OutActors);
}


void AThrowableWeaponProjectile::SetOwningPool(UObjectPoolSubsystem* NewPool)
{
	OwningPool = NewPool;
}

void AThrowableWeaponProjectile::OnActivateFromPool()
{
	bIsActiveInPool = true;
	bIsActivated = false;
	bPayloadActivated = false;
	bUseThrowableBehavior = true;
	bUseCustomProjectieMovement = false;

	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);

	ClearThrowableTimers();

	if (ProjectileMesh)
	{
		ProjectileMesh->SetVisibility(true);
	}

	if (CollisionComp)
	{
		CollisionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (ProjectileMovement && CollisionComp)
	{
		ProjectileMovement->Deactivate();
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->SetUpdatedComponent(CollisionComp);
		ProjectileMovement->bSimulationEnabled = true;
		ProjectileMovement->Velocity = FVector::ZeroVector;
	}

	CurrentRicochetCount = 0;
	NumPenetratedObjects = 0;
	StuckCount = 0;
	PrevProjectileLoc = GetActorLocation();
	DistanceMoved = 0.f;
	bIsHitScan = false;
	bActivatedMeshMovementForHitScan = false;
	HitScanEndPoints.Reset();
	CurrEndPointIdx = 0;
	MovementDirection = FVector::ZeroVector;
}

void AThrowableWeaponProjectile::OnDeactivateToPool()
{
	bIsActiveInPool = false;
	bIsActivated = false;
	bPayloadActivated = false;
	bUseCustomProjectieMovement = false;

	ClearThrowableTimers();
	StopLifeTimer();
	StopTrailEffect();

	if (SmokeEffectComponent)
	{
		SmokeEffectComponent->Deactivate();
		SmokeEffectComponent->DestroyComponent();
		SmokeEffectComponent = nullptr;
	}

	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->Velocity = FVector::ZeroVector;
		ProjectileMovement->Deactivate();
	}

	if (CollisionComp)
	{
		CollisionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CollisionComp->SetSphereRadius(InitialRadius);
	}

	if (ProjectileMesh)
	{
		ProjectileMesh->SetVisibility(true);
	}

	CurrentRicochetCount = 0;
	NumPenetratedObjects = 0;
	StuckCount = 0;
	PrevProjectileLoc = FVector::ZeroVector;
	DistanceMoved = 0.f;
	HitScanEndPoints.Reset();
	CurrEndPointIdx = 0;
	MovementDirection = FVector::ZeroVector;
	AdditionalDamage = 0.f;
	ProjectileOwner = nullptr;
	ThrowableWeapon = nullptr;
	ProjectileData = nullptr;

	SetActorHiddenInGame(true);
	SetActorTickEnabled(false);
}

bool AThrowableWeaponProjectile::IsActiveInPool() const
{
	return bIsActiveInPool;
}

void AThrowableWeaponProjectile::DeactivateProjectile()
{
	if (!OwningPool) { return; }
	OwningPool->ReturnToPool(this);
}
