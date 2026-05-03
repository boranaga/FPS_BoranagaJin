


#include "Items/Projectile/Projectile.h"
#include "Items/Weapons/Weapon.h"

#include "Characters/Enemies/EnemyBase.h"
#include "Characters/DamageInterface.h"
#include "Characters/DamageParams.h"

#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/Character.h"
#include "Components/SphereComponent.h"
#include "Components/DecalComponent.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

#include "Kismet/GameplayStatics.h"

#define SURFACE_DEFAULT SurfaceType1
#define SURFACE_METAL SurfaceType2
#define SURFACE_GLASS SurfaceType3
#define SURFACE_ENEMY SurfaceType4
#define SURFACE_ENERGY SurfaceType5
#define SURFACE_HEAD SurfaceType6
#define SURFACE_BODY SurfaceType7
#define SURFACE_LEFT_ARM SurfaceType8
#define SURFACE_RIGHT_ARM SurfaceType9

// Sets default values
AProjectile::AProjectile()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Use a sphere as a simple collision representation
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	//CollisionComp->InitSphereRadius(5.0f);
	CollisionComp->SetCollisionProfileName("PlayerProjectile");
	CollisionComp->SetCollisionObjectType(ECC_GameTraceChannel7);
	CollisionComp->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Ignore); //Projectile
	CollisionComp->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Ignore); //ClimbWall
	CollisionComp->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Ignore); //Weapon
	CollisionComp->SetCollisionResponseToChannel(ECC_GameTraceChannel4, ECR_Ignore); //Player
	CollisionComp->SetCollisionResponseToChannel(ECC_GameTraceChannel7, ECR_Ignore); //PlayerProjectile
	CollisionComp->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	CollisionComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Ignore);

	CollisionComp->bReturnMaterialOnMove = true;

	// Players can't walk on it
	CollisionComp->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Unwalkable, 0.f));
	CollisionComp->CanCharacterStepUpOn = ECB_No;

	// Set as root component
	RootComponent = CollisionComp;

	//TODO: Projectile attribute load �ϱ� ���� Data Table �����ϱ�
	// Use a ProjectileMovementComponent to govern this projectile's movement
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
	ProjectileMovement->UpdatedComponent = CollisionComp;
	ProjectileMovement->InitialSpeed = InitialSpeed;
	ProjectileMovement->MaxSpeed = 0.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 0.f;
	ProjectileMovement->bAutoActivate = false;

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	//ProjectileMesh->SetCollisionObjectType(ECC_GameTraceChannel1);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProjectileMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	ProjectileMesh->SetCollisionObjectType(ECC_GameTraceChannel7);
	ProjectileMesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Ignore);

	ProjectileMesh->SetCastShadow(false);

	// <Pooling Version>
	InitialLifeSpan = 0;
	SetActorHiddenInGame(true);
	//SetActorEnableCollision(false);
	SetActorTickEnabled(false);
}

void AProjectile::StartLifeTimer(float Seconds)
{
	if (Seconds > 0.f)
	{
		//GetWorldTimerManager().SetTimer(LifeTimer, this, &AProjectile::DeactiveProjectile, Seconds, false);
		TWeakObjectPtr WeakThis = this;
		GetWorld()->GetTimerManager().SetTimer(LifeTimer, FTimerDelegate::CreateWeakLambda(this, [WeakThis]()
			{
				if (auto* HardThis = WeakThis.Get())
				{
					HardThis->DeactiveProjectile();
				}

			}), Seconds, false);

	}
}

void AProjectile::StopLifeTimer()
{
	GetWorldTimerManager().ClearTimer(LifeTimer);
}

void AProjectile::DeactiveProjectile()
{
	StopLifeTimer();

	// <StuckCount>
	StuckCount = 0;

	//----------------
	//TODO: Deactive 시킬것들 전부 하기

	NumPenetrableObjects = 0;
	CurrentRicochetCount = 0;

	// <AutoAim>
	bIsAutoAim = false;

	// <HitScan>
	bIsHitScan = false;
	bActivatedMeshMovementForHitScan = false;
	HitScanEndPoints.Empty();
	CurrEndPointIdx = 0;
	DistanceMoved = 0;
	TargetDistance = 0.f;


	// <Penetration>
	NumPenetratedObjects = 0;
	AdditionalDamage = 0;

	// <Homing>
	TargetEnemy = nullptr;
	ProjectileMovement->bIsHomingProjectile = false;

	// <Damage Decay>
	DefaultDamage = ProjectileData->DefaultDamage;
	GetWorldTimerManager().ClearTimer(DamageDecayTimer);

	// <Custom Projectile Movement>
	bUseCustomProjectieMovement = false;

	// <Trail Effect>
	if (TrailEffectComponent)
	{
		TrailEffectComponent->Deactivate();
		TrailEffectComponent->DestroyComponent();
	}

	// <Radius>
	ProjectileRadius = InitialRadius;
	CollisionComp->SetSphereRadius(InitialRadius);

	//-------------
	SetActorHiddenInGame(true);
	//SetActorEnableCollision(false);
	CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	CollisionComp->SetCollisionResponseToChannel(ECC_GameTraceChannel6, ECR_Block);

	CollisionComp->OnComponentHit.RemoveDynamic(this, &AProjectile::OnHit);
	CollisionComp->OnComponentBeginOverlap.RemoveDynamic(this, &AProjectile::OnComponentBeginOverlap);

	CollisionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovement->StopMovementImmediately();
	ProjectileMovement->Deactivate();

	SetActorTickEnabled(false);
	bIsActivated = false;

	//---------------

	if (Weapon)
	{
		Weapon->ReturnProjectile(this);
	}
}

void AProjectile::InitProjectile(AActor* OwnerOfProjectile, AWeapon* OwnerWeapon, float additonalDamage, float AdditionalRadius, int32 NumPenetrable, bool HitScan, bool AutoAim)
{
	//UE_LOG(LogTemp, Error, TEXT("InitProjectile_Pool"));

	bIsActivated = true;
	SetActorHiddenInGame(false);
	//SetActorEnableCollision(true);
	CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetActorTickEnabled(true);

	if (IsValid(OwnerWeapon))
	{
		Weapon = OwnerWeapon;
	}

	if (IsValid(OwnerOfProjectile))
	{
		ProjectileOwner = OwnerOfProjectile;
		SpawnTrailEffect();
	}

	if (AutoAim)
	{
		bIsAutoAim = true;
		bIsHitScan = false;
		NumPenetrableObjects = 0; //TODO: ???
	}
	else
	{
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
				if (!CollisionComp->OnComponentHit.IsAlreadyBound(this, &AProjectile::OnHit))
				{
					CollisionComp->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);
				}
			}
		}
	}

	if (bCanSimpleBounce)
	{
		// MEMO: Test
		ProjectileMovement->bShouldBounce = true;
		ProjectileMovement->Bounciness = 0.6f;    //(0~1)
		ProjectileMovement->Friction = 0.2f;
		ProjectileMovement->BounceVelocityStopSimulatingThreshold = 10.0f;
		ProjectileMovement->bRotationFollowsVelocity = true;
	}

	AdditionalDamage = additonalDamage;

	if (AdditionalRadius > 0.f)
	{
		ProjectileRadius = InitialRadius + AdditionalRadius;
		CollisionComp->SetSphereRadius(InitialRadius + AdditionalRadius);
	}

	//TODO: Set Damage Decay Timer
	if (DamageDecayTime > 0)
	{
		//GetWorld()->GetTimerManager().SetTimer(DamageDecayTimer, this, &AProjectile::ApplyDamageDecay, DamageDecayTime, false);
		TWeakObjectPtr WeakThis = this;
		GetWorld()->GetTimerManager().SetTimer(DamageDecayTimer, FTimerDelegate::CreateWeakLambda(this, [WeakThis]()
			{
				if (auto* HardThis = WeakThis.Get())
				{
					HardThis->ApplyDamageDecay();
				}

			}), DamageDecayTime, false);
	}

	StartLifeTimer(LifeSpan);
}

void AProjectile::InitPhysicsProjectile()
{

	if (!CollisionComp->OnComponentHit.IsAlreadyBound(this, &AProjectile::OnHit))
	{
		CollisionComp->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);
	}
	if (!CollisionComp->OnComponentBeginOverlap.IsAlreadyBound(this, &AProjectile::OnComponentBeginOverlap))
	{
		CollisionComp->OnComponentBeginOverlap.AddDynamic(this, &AProjectile::OnComponentBeginOverlap);
	}
	CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionComp->SetCollisionResponseToChannel(ECC_GameTraceChannel6, ECR_Overlap);
}

void AProjectile::InitHitScan()
{
	CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionComp->SetCollisionResponseToChannel(ECC_GameTraceChannel6, ECR_Overlap);
}

void AProjectile::LoadProjectileData()
{
	ProjectileData = ProjectileDataTableHandle.GetRow<FProjectileData>("");
	if (ProjectileData)
	{
		// <Effect>
		TrailEffect = ProjectileData->TrailEffect;
		ImpactEffect = ProjectileData->ImpactEffect;
		ExplosionEffect = ProjectileData->ExplosionEffect;
		DecalMaterial = ProjectileData->HoleDecal;
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

		// <Homing>
		HomingAccelerationMagnitude = ProjectileData->HomingAccelerationMagnitude;

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

		// <DamageDecay>
		DamageDecayTime = ProjectileData->DamageDecayTime;
		DamageDecayRate = ProjectileData->DamageDecayRate;

		// <CustomProjectileMovement>
		PM_Cam_To_d_Len = ProjectileData->PM_Cam_To_d_Len;
	}
}

void AProjectile::SetWeapon(AWeapon* NewWeapon)
{
	Weapon = NewWeapon;
}

void AProjectile::SetHomingTarget(bool bIsHoming, AActor* Target)
{
	ProjectileMovement->bIsHomingProjectile = bIsHoming;

	if (bIsHoming)
	{
		ProjectileMovement->HomingAccelerationMagnitude = HomingAccelerationMagnitude;
		ProjectileMovement->HomingTargetComponent = Target->GetRootComponent();

		TargetEnemy = Cast<AEnemyBase>(Target);
		if (TargetEnemy)
		{
			RecentTargetLocation = TargetEnemy->GetActorLocation();
		}
		//UE_LOG(LogTemp, Warning, TEXT("Homing!!!"));
	}
}

void AProjectile::LaunchProjectile(FVector MuzzlePos, FRotator Direction)
{
	SetActorLocationAndRotation(MuzzlePos, Direction);
	ProjectileMovement->Activate();
}

void AProjectile::ApplyExplosiveDamage(bool bCanExplosiveDamage, FVector CenterLocation)
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
				ApplyDamage(OverlappedActor, DamageAmount, EDamageType::Explosion, true, NAME_None);
				if (OnBodyShot.IsBound())
				{
					OnBodyShot.Execute();
				}
				//UE_LOG(LogTemp, Error, TEXT("Explosive Damage!!!"));
			}
		}

		if (bVisualizeExplosionRadius)
		{
			DrawSphere(CenterLocation, MaxExplosionRadius);
		}
	}
}
void AProjectile::ApplyDamage(AActor* OtherActor, float DamageAmount, EDamageType DamageType, bool bCanForceDamage,
	const FName BoneName, TEnumAsByte<EPhysicalSurface> SurfaceType, const FVector ImpulseDirection, const FVector ImpactPoint)
{
	FDamageParams Damage; //TODO: 착탄 위치
	Damage.DamageAmount = DamageAmount;
	Damage.DamageType = DamageType;
	Damage.bCanForceDamage = bCanForceDamage;
	Damage.BoneName = BoneName;
	Damage.ImpulseDirection = ImpulseDirection;
	Damage.SurfaceType = SurfaceType;
	Damage.ImpactPoint = ImpactPoint;

	if (OtherActor->GetClass()->ImplementsInterface(UDamageInterface::StaticClass()))
	{
		Cast<IDamageInterface>(OtherActor)->TakeDamage(Damage, this->ProjectileOwner);
	}
}
bool AProjectile::SearchOverlappedActor(FVector CenterLocation, float SearchRadius, TArray<AActor*>& OverlappedActors)
{
	TArray<TEnumAsByte<EObjectTypeQuery>> traceObjectTypes;
	traceObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));
	traceObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_GameTraceChannel6));
	TArray<AActor*> ignoreActors;
	ignoreActors.Init(ProjectileOwner, 1);
	bool bIsAnyActorExist = UKismetSystemLibrary::SphereOverlapActors(GetWorld(), CenterLocation, SearchRadius, traceObjectTypes, nullptr, ignoreActors, OverlappedActors);

	return bIsAnyActorExist;
}

void AProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	//TODO: Projectile�� �ٸ� actor���� hit ���� ��, OtherActor�� ������ ���� �ٸ� event �߻���Ű��. Interface ����ϱ�
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

		// Destroy(); //이게 문제가 됨
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

				PlaySoundAtLocationByMaterial(UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get()), Hit.ImpactPoint);

				if (HeadShotAdditionalDamage > 0.f && CheckHeadHit(Hit))
				{
					ApplyDamage(OtherActor, DefaultDamage + AdditionalDamage + HeadShotAdditionalDamage, EDamageType::Melee, false, Hit.BoneName, UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get()), Hit.ImpactNormal, Hit.ImpactPoint);

					if (OnHeadShot.IsBound())
					{
						OnHeadShot.Execute();
					}
				}
				else
				{
					ApplyDamage(OtherActor, DefaultDamage + AdditionalDamage, EDamageType::Melee, false, Hit.BoneName, UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get()), Hit.ImpactNormal, Hit.ImpactPoint);
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
					//Destroy();
					DeactiveProjectile();
				}
				else
				{
					if (bCanSimpleBounce && CurrentRicochetCount < MaxRicochetCount && CheckRicochetAngle(Hit.ImpactNormal, ProjectileMovement->Velocity))
					{
						CurrentRicochetCount++;
					}
					else
					{
						//Destroy();
						DeactiveProjectile();
					}
				}
			}
		}
		else
		{
			//// Only add impulse and destroy projectile if we hit a physics
			//if ((OtherActor != nullptr) && (OtherActor != this) && (OtherComp != nullptr) && OtherComp->IsSimulatingPhysics())
			//{
			//	OtherComp->AddImpulseAtLocation(GetVelocity() * 100.0f, GetActorLocation());
			//}

			SpawnImpactEffect(Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
			SpawnDecalEffect(Hit.ImpactPoint, Hit.ImpactNormal.Rotation());

			//Destroy();
			DeactiveProjectile();
		}
	}
}

void AProjectile::OnComponentBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	//UE_LOG(LogTemp, Error, TEXT("Projectile Overlapped!!!"));
	if (NumPenetrableObjects > 0)
	{
		if (OtherActor != nullptr)
		{
			if (OtherActor != ProjectileOwner)
			{
				if ((OtherActor != nullptr) && (OtherActor != this) && (OtherComp != nullptr) && OtherComp->IsSimulatingPhysics())
				{
					OtherComp->AddImpulseAtLocation(GetVelocity() * 100.0f, GetActorLocation());
				}

				//TODO: Overlap �� ���� Effect�� ��� ���� �����غ�����
				//SpawnImpactEffect(Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
				//SpawnDecalEffect(Hit.ImpactPoint, Hit.ImpactNormal.Rotation());

				//TODO: Damage�� Initialize���� ���� �޾ƿ;���. Additional Damage�� �޾ƿ;���
				//�ϴ��� �⺻ Damage�� ����

				if (HeadShotAdditionalDamage > 0.f && CheckHeadOvelap(OtherActor, SweepResult))
				{
					SpawnImpactEffect(SweepResult.ImpactPoint, SweepResult.ImpactNormal.Rotation());
					ApplyDamage(OtherActor, DefaultDamage + AdditionalDamage + HeadShotAdditionalDamage, EDamageType::Melee, false, SweepResult.BoneName, UPhysicalMaterial::DetermineSurfaceType(SweepResult.PhysMaterial.Get()), SweepResult.ImpactNormal, SweepResult.ImpactPoint);

					if (OnHeadShot.IsBound())
					{
						OnHeadShot.Execute();
					}
				}
				else
				{
					SpawnImpactEffect(SweepResult.ImpactPoint, SweepResult.ImpactNormal.Rotation());
					ApplyDamage(OtherActor, DefaultDamage + AdditionalDamage, EDamageType::Melee, false, SweepResult.BoneName, UPhysicalMaterial::DetermineSurfaceType(SweepResult.PhysMaterial.Get()), SweepResult.ImpactNormal, SweepResult.ImpactPoint);
					//UE_LOG(LogTemp, Error, TEXT("Projectile Overlapped!!!"));
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

					//Destroy();
					DeactiveProjectile();
				}
			}
		}
	}
}


void AProjectile::SpawnImpactEffect(FVector SpawnLocation, FRotator SpawnRotation)
{
	if (ImpactEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ImpactEffect, SpawnLocation, SpawnRotation,
			FVector(1.0f), true, true, ENCPoolMethod::AutoRelease);
	}
}
void AProjectile::SpawnExplosionEffect(FVector SpawnLocation)
{
	if (ExplosionEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ExplosionEffect, SpawnLocation,
			FRotator::ZeroRotator, FVector(1.0f), true, true, ENCPoolMethod::AutoRelease);
	}
}
void AProjectile::SpawnTrailEffect(bool bShouldAttachedToWeapon) //TODO: Rocket Trail ����� �̻���. �պ�����
{
	if (ProjectileMesh && TrailEffect)
	{
		FTransform TrailStartTransform = ProjectileMesh->GetSocketTransform(FName(TEXT("TrailStart")), ERelativeTransformSpace::RTS_Component);
		FTransform TrailEndTransform = ProjectileMesh->GetSocketTransform(FName(TEXT("TrailEnd")), ERelativeTransformSpace::RTS_Component);

		// TODO: offset�� Traile ���� �ٸ��� �����ؾ���. �ƴϸ� �����Ϳ��� Mesh���� socket�� ��ġ�� ���� �ٲ��ִ� ���� ���� ���������
		FVector TrailLocationOffset = (TrailEndTransform.GetLocation() - TrailStartTransform.GetLocation()).GetSafeNormal() * TrailOffsetDist;

		if (bShouldAttachedToWeapon)
		{
			//UE_LOG(LogTemp, Error, TEXT("Spawn Trail Effect!!!"));

			TrailEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(),
				TrailEffect,
				Weapon->GetWeaponMesh()->GetSocketLocation(FName(TEXT("Muzzle"))),
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
void AProjectile::SpawnDecalEffect(FVector SpawnLocation, FRotator SpawnRotation)
{
	if (DecalMaterial)
	{
		FVector DecalSize = FVector(2.0f, 8.0f, 8.0f);

		UDecalComponent* ProjectileDecal =
			UGameplayStatics::SpawnDecalAtLocation(
				GetWorld(),
				DecalMaterial,
				DecalSize,
				SpawnLocation,
				SpawnRotation,
				10.0f
			);
		ProjectileDecal->SetFadeScreenSize(0.0001f);
	}
}

void AProjectile::UpdateTrailEffect()
{
	if (bShouldUpdateTrailEffect)
	{
		if (TrailEffectComponent)
		{
			TrailEffectComponent->SetVectorParameter(FName(TEXT("Beam End")), ProjectileMesh->GetSocketLocation(FName(TEXT("TrailStart"))));
		}
	}
}

void AProjectile::DrawSphere(FVector Location, float Radius)
{
	DrawDebugSphere(
		GetWorld(),                 // UWorld* World
		Location,         // ��ġ (Center)
		Radius,                     // ������ (Radius)
		12,                        // ���׸�Ʈ �� (Segments)
		FColor::Red,               // ����
		false,                     // ���� �ð� ���� (true�� ���������� ǥ�õ�)
		5.0f,                      // ���� �ð� (��)
		0,                         // Depth Priority
		2.0f                       // �� �β�
	);
}

#pragma region Sound
void AProjectile::PlaySoundAtLocationByMaterial(EPhysicalSurface SurfaceType, FVector Location)
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
#pragma endregion

#pragma region HitScan
void AProjectile::SetHitScanActive(bool bflag)
{
	bIsHitScan = bflag;
}
void AProjectile::LaunchHitScan(FVector StartLocation, FVector TraceDirection, FVector MuzzlePos)
{
	//PerformHitScan(StartLocation, TraceDirection, 50000.f, ProjectileRadius, HitScanEndPoints); //TODO: MaxDistnace 설정해야함
	PerformHitScan(StartLocation, TraceDirection, 50000.f, ProjectileRadius, HitScanEndPoints); //TODO: MaxDistnace 설정해야함

	InitHitScanProjectileMovement(MuzzlePos);
}
void AProjectile::PerformHitScan(FVector StartLocation, FVector TraceDirection, float MaxDistance, float SphereRadius, TArray<FVector>& OutHitLocations)
{
	FVector Start = StartLocation;
	FVector Direction = TraceDirection;
	FVector End = StartLocation + TraceDirection * MaxDistance;

	TArray<FVector> HitStaticLocations;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(ProjectileOwner);
	Params.AddIgnoredComponent(Weapon->GetWeaponMesh());
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
			ECC_GameTraceChannel7,
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
								EDamageType::Melee, false, HitResult.BoneName, UPhysicalMaterial::DetermineSurfaceType(HitResult.PhysMaterial.Get()), TraceDirection, HitResult.ImpactPoint);

							if (OnHeadShot.IsBound())
							{
								OnHeadShot.Execute();
							}
						}
						else
						{
							SpawnImpactEffect(HitResult.ImpactPoint, (-HitResult.ImpactNormal).Rotation());
							ApplyDamage(HitResult.GetActor(), DefaultDamage + AdditionalDamage, EDamageType::Melee, false, HitResult.BoneName, UPhysicalMaterial::DetermineSurfaceType(HitResult.PhysMaterial.Get()), TraceDirection, HitResult.ImpactPoint);
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
void AProjectile::InitHitScanProjectileMovement(FVector StartLocation)
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
void AProjectile::UpdateHitScanProjectileMovement(float DeltaTime)
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
			DeactiveProjectile();
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
#pragma endregion

#pragma region AutoAim
void AProjectile::LaunchAutoAim(FVector StartLocation, FVector TraceDir, FVector AutoAimDir, FVector MuzzleLoc, float MaxDistance, float AutoAimRadius)
{
	FVector TraceStart = StartLocation;
	FVector TraceDirection = TraceDir;
	FVector TraceEnd = StartLocation + TraceDirection * MaxDistance;

	FCollisionQueryParams BaseParams(SCENE_QUERY_STAT(WeaponSphereSweep), /*bTraceComplex=*/false);
	if (ProjectileOwner) BaseParams.AddIgnoredActor(ProjectileOwner);
	if (Weapon->GetWeaponMesh()) BaseParams.AddIgnoredComponent(Weapon->GetWeaponMesh());
	if (ProjectileMesh) BaseParams.AddIgnoredComponent(ProjectileMesh);
	BaseParams.AddIgnoredActor(this);
	BaseParams.bReturnPhysicalMaterial = true; //TODO: ???

	FCollisionShape Sphere = FCollisionShape::MakeSphere(ProjectileRadius);

	FHitResult HitResult;

	bool bHit = GetWorld()->SweepSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		FQuat::Identity,
		ECC_GameTraceChannel7,
		Sphere,
		BaseParams);

	bool bIsBlockedByWorldStatic = false;
	bool bIsHitByHitScan = false;

	if (bHit)
	{
		//TODO: Set ProjectileMesh Movement

		if (HitResult.GetComponent()->GetCollisionObjectType() == ECC_WorldStatic)
		{
			bIsBlockedByWorldStatic = true;
		}
		else
		{
			ACharacter* Enemy = Cast<ACharacter>(HitResult.GetActor());
			if (Enemy)
			{
				bIsHitByHitScan = true;
				HitScanEndPoints.Add(Enemy->GetActorLocation());
				if (HeadShotAdditionalDamage > 0.f && CheckHeadHit(HitResult))
				{
					ApplyDamage(HitResult.GetActor(), DefaultDamage + AdditionalDamage + HeadShotAdditionalDamage,
						EDamageType::Melee, false, HitResult.BoneName, UPhysicalMaterial::DetermineSurfaceType(HitResult.PhysMaterial.Get()), TraceDirection, HitResult.ImpactPoint);

					if (OnHeadShot.IsBound())
					{
						OnHeadShot.Execute();
					}
				}
				else
				{
					ApplyDamage(HitResult.GetActor(), DefaultDamage + AdditionalDamage, EDamageType::Melee, false, HitResult.BoneName, UPhysicalMaterial::DetermineSurfaceType(HitResult.PhysMaterial.Get()), TraceDirection, HitResult.ImpactPoint);
					//UE_LOG(LogTemp, Error, TEXT("bone11-2: %s"), *HitResult.BoneName.ToString());
					if (OnBodyShot.IsBound())
					{
						OnBodyShot.Execute();
					}
				}
				bIsHitScan = true;
				InitHitScan();
				InitHitScanProjectileMovement(MuzzleLoc); //TODO: 정상화
			}
		}
	}

	if (!bHit || bIsBlockedByWorldStatic)
	{
		FHitResult AutoAimHitResult;

		TArray<FHitResult> HitResults;
		HitResults.Reserve(6);

		const FCollisionShape AutoAimSphere = FCollisionShape::MakeSphere(AutoAimRadius);

		//-------
		FCollisionResponseParams ResponseParams;
		ResponseParams.CollisionResponse.SetResponse(ECC_GameTraceChannel1, ECR_Ignore);
		ResponseParams.CollisionResponse.SetResponse(ECC_WorldStatic, ECR_Ignore);
		//-------

		const bool bAnyHit = GetWorld()->SweepMultiByChannel(
			HitResults,
			StartLocation,
			StartLocation + AutoAimDir * MaxDistance,
			FQuat::Identity,
			ECC_GameTraceChannel8,
			AutoAimSphere, BaseParams, ResponseParams);

		if (!bAnyHit)
		{
			//TODO: Launch Single Projectile
			InitPhysicsProjectile();
			InitProjectileMovement(StartLocation, TraceDir, MuzzleLoc);
			return;
		}

		float BestDistSq = TNumericLimits<float>::Max();
		bool  bFound = false;

		TSet<const AActor*> Seen;
		Seen.Reserve(HitResults.Num());

		const FVector CharLoc = ProjectileOwner->GetActorLocation();

		AActor* AutoAimHitEnemy = nullptr;
		FVector AutoAimHitLocation;
		//FHitResult AutoAimHitResult;

		for (const FHitResult& Hr : HitResults)
		{
			AActor* HitActor = Hr.GetActor();
			if (!HitActor || Seen.Contains(HitActor)) continue;
			Seen.Add(HitActor);

			const FVector CandidateLoc = HitActor->GetActorLocation();

			FCollisionResponseParams WorldStaticResponseParams;
			WorldStaticResponseParams.CollisionResponse.SetResponse(ECC_GameTraceChannel1, ECR_Ignore);
			//WorldStaticResponseParams.CollisionResponse.SetResponse(ECC_GameTraceChannel2, ECR_Ignore); //ClimbWall
			WorldStaticResponseParams.CollisionResponse.SetResponse(ECC_GameTraceChannel3, ECR_Ignore);
			WorldStaticResponseParams.CollisionResponse.SetResponse(ECC_GameTraceChannel4, ECR_Ignore);
			WorldStaticResponseParams.CollisionResponse.SetResponse(ECC_GameTraceChannel5, ECR_Ignore);
			WorldStaticResponseParams.CollisionResponse.SetResponse(ECC_GameTraceChannel6, ECR_Ignore);
			WorldStaticResponseParams.CollisionResponse.SetResponse(ECC_GameTraceChannel7, ECR_Ignore);

			FHitResult BlockHit;
			const bool bBlocked = GetWorld()->LineTraceSingleByChannel(
				BlockHit,
				StartLocation,
				CandidateLoc,
				ECC_WorldStatic,
				BaseParams,
				WorldStaticResponseParams);

			if (bBlocked) continue;

			const float DistSq = FVector::DistSquared(CharLoc, CandidateLoc);
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				AutoAimHitLocation = CandidateLoc;
				AutoAimHitEnemy = HitActor;
				AutoAimHitResult = Hr;
				bFound = true;
			}
		}

		if (bFound)
		{
			//TODO: 1. 강제 데미지 적용, 2. Set ProjectileMovement

			//TODO: Impact Point 설정하기
			ApplyDamage(AutoAimHitEnemy, DefaultDamage + AdditionalDamage,
				EDamageType::Melee, false, AutoAimHitResult.BoneName, UPhysicalMaterial::DetermineSurfaceType(AutoAimHitResult.PhysMaterial.Get()), TraceDirection);

			if (OnBodyShot.IsBound())
			{
				OnBodyShot.Execute();
			}

			HitScanEndPoints.Add(AutoAimHitLocation);
			bIsHitScan = true;
			InitHitScan();
			InitHitScanProjectileMovement(MuzzleLoc); //TODO: 정상화
		}
		else
		{
			//TODO: Launch Single Projectile
			InitPhysicsProjectile();
			InitProjectileMovement(StartLocation, TraceDir, MuzzleLoc);
			return;
		}
	}
}
#pragma endregion

#pragma region Penetration
void AProjectile::UpdatePenetration() //TODO: ?
{
	NumPenetratedObjects++;
}

void AProjectile::ResetPenetration()  //TODO: ?
{
	NumPenetratedObjects = 0;
}
#pragma endregion

#pragma region HeadShot
bool AProjectile::CheckHeadHit(const FHitResult& HitResult)
{
	//UE_LOG(LogTemp, Error, TEXT("FName: %s"), *HitResult.BoneName.ToString());
	if (HitResult.BoneName == "head")
	{
		return true;
	}
	return false;
}
bool AProjectile::CheckHeadOvelap(const AActor* OverlappedActor, const FHitResult& SweepResult)
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
#pragma endregion

#pragma region Homing
bool AProjectile::IsTargetValid()
{
	//TODO: Target�� ���� ���θ� �Ǵ��� ����� �ʿ��� -> ���� ���� �����Բ� ��û����� ��
	if (!ProjectileMovement->HomingTargetComponent.IsValid())
	{
		return false;
	}
	return true;
}
bool AProjectile::IsTargetWithInRange()
{
	if (FVector::Distance(GetActorLocation(), RecentTargetLocation) < ExlosionTriggerRadius)
	{
		return true;
	}
	return false;
}
void AProjectile::UpdateTargetInfo()
{
	if (ProjectileMovement->bIsHomingProjectile)
	{
		if (!IsTargetValid() || IsTargetWithInRange()) // TODO:
		{
			//UE_LOG(LogTemp, Error, TEXT("Target is not valid!!!"));
			SpawnExplosionEffect(GetActorLocation());
			ApplyExplosiveDamage(bIsExplosive, GetActorLocation());
			//Destroy();
			DeactiveProjectile();
		}
		else
		{
			if (TargetEnemy && IsValid(TargetEnemy))
			{
				RecentTargetLocation = TargetEnemy->GetActorLocation();
			}
		}
	}
}
#pragma endregion

#pragma region Impulse
void AProjectile::AddImpulseToEnemy(AActor* OtherActor, FVector Force)
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
bool AProjectile::CheckRicochetAngle(FVector normal, FVector vel)
{
	return FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(-normal.GetSafeNormal(), vel.GetSafeNormal()))) > MinIncidenceAngle;
}
FVector AProjectile::GetReflectionAngle(FVector normal, FVector input)
{
	FVector norm = normal.GetSafeNormal();
	FVector in = input.GetSafeNormal();

	return in - 2 * (FVector::DotProduct(in, norm) * norm);
}
#pragma endregion

#pragma region Damage Decay
void AProjectile::ApplyDamageDecay()
{
	DefaultDamage *= DamageDecayRate;
}
#pragma endregion


#pragma region ProjectileMovement
void AProjectile::InitProjectileMovement(FVector StartPos, FVector Direction, FVector MuzzlePos)
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
void AProjectile::UpdateProjectileMovement(float DeltaTime)
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
#pragma endregion

#pragma region CheckStuck
void AProjectile::CheckAndDeactivateIfStuck()
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
			DeactiveProjectile();
		}
		else
		{
			PrevProjectileLoc = GetActorLocation();
		}
	}
}
#pragma endregion


//// Called when the game starts or when spawned
//void AProjectile::BeginPlay()
//{
//	Super::BeginPlay();
//	
//}
//
void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateTrailEffect();
	UpdateTargetInfo();

	if (bIsHitScan)
	{
		//Projectile Movement Update
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
	//TODO: Activate flag에 따라 활성화 여부 결정


	CheckAndDeactivateIfStuck();
}

void AProjectile::EndPlay(const EEndPlayReason::Type EndPlayReason)
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

void AProjectile::BeginDestroy()
{
	Super::BeginDestroy();
}

void AProjectile::FellOutOfWorld(const UDamageType& dmgType)
{
	SetActorLocation(FVector::ZeroVector);
	SetActorRotation(FRotator::ZeroRotator);

	DeactiveProjectile();
}

void AProjectile::OutsideWorldBounds()
{
	SetActorLocation(FVector::ZeroVector);
	SetActorRotation(FRotator::ZeroRotator);

	DeactiveProjectile();
}


