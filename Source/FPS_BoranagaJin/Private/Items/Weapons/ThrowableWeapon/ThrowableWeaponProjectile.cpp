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
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AISense_Hearing.h"

AThrowableWeaponProjectile::AThrowableWeaponProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	//CollisionComp->InitSphereRadius(5.0f);
	CollisionComp->SetCollisionProfileName("PlayerProjectile");
	CollisionComp->SetCollisionObjectType(ECC_GameTraceChannel3);
	CollisionComp->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Ignore); //Projectile
	CollisionComp->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Ignore); //ClimbWall
	CollisionComp->SetCollisionResponseToChannel(ECC_GameTraceChannel5, ECR_Ignore); //Weapon
	CollisionComp->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Ignore); //Player
	CollisionComp->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Ignore); //PlayerProjectile
	CollisionComp->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	CollisionComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Ignore);

	CollisionComp->bReturnMaterialOnMove = true;
	CollisionComp->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Unwalkable, 0.f));
	CollisionComp->CanCharacterStepUpOn = ECB_No;

	RootComponent = CollisionComp;

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
	ProjectileMovement->UpdatedComponent = CollisionComp;
	ProjectileMovement->InitialSpeed = InitialSpeed;
	ProjectileMovement->MaxSpeed = 0.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false; //TODO: DT에서 custom과 projectileMovement 둘 중에서 어느 것 사용할지 설정할 수 있도록
	ProjectileMovement->ProjectileGravityScale = 0.f;
	ProjectileMovement->bAutoActivate = false;

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProjectileMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	ProjectileMesh->SetCollisionObjectType(ECC_GameTraceChannel3);
	ProjectileMesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Ignore);

	ProjectileMesh->SetCastShadow(false);

	// <Pooling Version>
	InitialLifeSpan = 0;
	SetActorHiddenInGame(true);
	//SetActorEnableCollision(false);
	SetActorTickEnabled(false);
}

void AThrowableWeaponProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsActiveInPool) return;

	UpdateTrailEffect();

	if (bIsHitScan)
	{
		if (bActivatedMeshMovementForHitScan)
		{
			UpdateHitScanProjectileMovement(DeltaTime);
		}
	}

	if (bUseCustomProjectieMovement)
	{
		UpdateProjectileMovement(DeltaTime);
	}

	//--------------------------------
	// <Pool Version>
	CheckAndDeactivateIfStuck();
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
	//TODO: 포물선 궤적인지 직선 경로인지 정할 수있도록 해야함

	bIsActivated = true;
	SetActorHiddenInGame(false);
	//SetActorEnableCollision(true);
	CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetActorTickEnabled(true);

	if (IsValid(OwnerWeapon)) { ThrowableWeapon = OwnerWeapon; }
	if (IsValid(OwnerOfProjectile))
	{
		ProjectileOwner = OwnerOfProjectile;
		SpawnTrailEffect();
	}

	if (HitScan)
	{
		bIsHitScan = HitScan;
		NumPenetrableObjects = NumPenetrable;
		InitHitScan();
	}
	else
	{
		if (NumPenetrable > 0)
		{
			InitPhysicsProjectile();
			NumPenetrableObjects = NumPenetrable;
		}
		else
		{
			if (!CollisionComp->OnComponentHit.IsAlreadyBound(this, &AThrowableWeaponProjectile::OnHit))
			{
				CollisionComp->OnComponentHit.AddDynamic(this, &AThrowableWeaponProjectile::OnHit);
			}
		}
	}

	if (bCanSimpleBounce)
	{
		ProjectileMovement->bShouldBounce = true;
		ProjectileMovement->Bounciness = 0.6f;    //(0~1)
		ProjectileMovement->Friction = 0.2f;
		ProjectileMovement->BounceVelocityStopSimulatingThreshold = 10.0f;
		ProjectileMovement->bRotationFollowsVelocity = true;
	}

	AdditionalDamage = additonalDamage;

	StartLifeTimer(LifeSpan);
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
	ProjectileData = ProjectileDataTableHandle.GetRow<FThrowableWeaponProjectileData>("");
	if (ProjectileData)
	{
		// <Effect>
		TrailEffect = ProjectileData->TrailEffect;
		ImpactEffect = ProjectileData->ImpactEffect;
		ExplosionEffect = ProjectileData->ExplosionEffect;
		ImpactDecalMaterial = ProjectileData->ImpactDecal;
		TrailOffsetDist = ProjectileData->TrailOffsetDist;

		LifeSpan = ProjectileData->InitialLifeSpan;

		// <Sound>
		HitSound_Default = ProjectileData->HitSound_Default;
		HitSound_Metal = ProjectileData->HitSound_Metal;
		HitSound_Glass = ProjectileData->HitSound_Glass;
		HitSound_Enemy = ProjectileData->HitSound_Enemy;
		HitSound_Energy = ProjectileData->HitSound_Energy;

		// <Damage>
		DefaultDamage = ProjectileData->DefaultDamage;
		HeadShotAdditionalDamage = ProjectileData->HeadShotAdditionalDamage;

		// <Explosive>
		bIsExplosive = ProjectileData->bIsExplosive;
		bVisualizeExplosionRadius = ProjectileData->bVisualizeExplosionRadius;
		MaxExplosiveDamage = ProjectileData->MaxExplosiveDamage;
		MaxExplosionRadius = ProjectileData->MaxExplosionRadius;

		// <Velocity>
		ProjectileMovement->InitialSpeed = ProjectileData->InitialSpeed;
		ProjectileMovement->MaxSpeed = ProjectileData->MaxSpeed;
		PM_Vel = ProjectileData->MaxSpeed;
		HitScanProjectileVelocity = ProjectileData->InitialSpeed;

		InitialRadius = ProjectileData->InitialRadius;
		CollisionComp->SetSphereRadius(InitialRadius);

		// <Impulse>
		bCanApplyImpulseToEnemy = ProjectileData->bCanApplyImpulseToEnemy;
		HitImpulseToEnemy = ProjectileData->HitImpulseToEnemy;

		// <Ricochet>
		bCanSimpleBounce = ProjectileData->bCanSimpleBounce;
		MaxRicochetCount = ProjectileData->MaxRicochetCount;
		MinIncidenceAngle = ProjectileData->MinIncidenceAngle;

		// <HitScan>
		bDebugHitScan = ProjectileData->bDebugHitScan;

		// <CustomProjectileMovement>
		PM_Cam_To_d_Len = ProjectileData->PM_Cam_To_d_Len;
	}
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
	if (bCanExplosiveDamage)
	{
		TArray<AActor*> OverlappedActors;
		if (SearchOverlappedActor(CenterLocation, MaxExplosionRadius, OverlappedActors))
		{
			for (AActor* OverlappedActor : OverlappedActors)
			{
				float DistanceToTarget = FVector::Distance(CenterLocation, OverlappedActor->GetActorLocation());
				float DamageAmount;
				if (DistanceToTarget > MaxExplosionRadius)
				{
					DamageAmount = 0.f;
				}
				else
				{
					DamageAmount = ((MaxExplosionRadius - DistanceToTarget) / MaxExplosionRadius) * MaxExplosiveDamage;
				}
				ApplyDamage(OverlappedActor, DamageAmount, EGameDamageType::Explosion, true, NAME_None);
				if (OnBodyShot.IsBound())
				{
					OnBodyShot.Execute();
				}
			}
		}

		if (bVisualizeExplosionRadius)
		{
			DrawSphere(CenterLocation, MaxExplosionRadius);
		}
	}
}

void AThrowableWeaponProjectile::ApplyDamage(AActor* OtherActor, float DamageAmount, EGameDamageType DamageType, bool bCanForceDamage, const FName BoneName, TEnumAsByte<EPhysicalSurface> SurfaceType, const FVector ImpulseDirection, const FVector ImpactPoint)
{
	FDamageParams Damage; //TODO: 착탄 위치
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

				//TODO: SoundSystem에 편입
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
		else //TODO: 이것이 반드시 필요한 분기인가?
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

		if (bShouldAttachedToWeapon) //TODO: 기능 삭제
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

void AThrowableWeaponProjectile::PlaySoundAtLocationByMaterial(EPhysicalSurface SurfaceType, FVector Location) //TODO: SoundSystem에 편입
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
			//TODO: 그대로 직진? Destroy?
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
	//PerformHitScan(StartLocation, TraceDirection, 50000.f, ProjectileRadius, HitScanEndPoints); //TODO: MaxDistnace 설정해야함
	PerformHitScan(StartLocation, TraceDirection, 50000.f, ProjectileRadius, HitScanEndPoints); //TODO: MaxDistnace 설정해야함

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
			//UE_LOG(LogTemp, Error, TEXT("Projectile is Stuck!!!"));
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

void AThrowableWeaponProjectile::SetOwningPool(UObjectPoolSubsystem* NewPool)
{
	OwningPool = NewPool;
}

void AThrowableWeaponProjectile::OnActivateFromPool()
{
	bIsActiveInPool = true;
}

void AThrowableWeaponProjectile::OnDeactivateToPool()
{
	bIsActiveInPool = false;
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
