

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"

#include "Characters/EDamageType.h"

#include "Data/ProjectileData.h"

#include "Projectile.generated.h"

class AWeapon;
class AEnemyBase;

class USphereComponent;
class UProjectileMovementComponent;
class UNiagaraSystem;

DECLARE_DELEGATE(FHeadShotDelegate);
DECLARE_DELEGATE(FBodyShotDelegate);

UCLASS(config = Game)
class FPS_BORANAGAJIN_API AProjectile : public AActor
{
	GENERATED_BODY()
public:
	FHeadShotDelegate OnHeadShot;
	FBodyShotDelegate OnBodyShot;

	const FProjectileData* GetProjectileData() const { return ProjectileData; } //MEMO: 필요한가?
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Projectile)
	FDataTableRowHandle ProjectileDataTableHandle;

	//UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (RowType="ProjectileData"))
	//FDataTableRowHandle ProjectileDataTableHandle;

	FProjectileData* ProjectileData = nullptr;

	UPROPERTY(VisibleDefaultsOnly, Category = Projectile)
	USphereComponent* CollisionComp = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	UProjectileMovementComponent* ProjectileMovement = nullptr;

	UPROPERTY(VisibleDefaultsOnly, Category = Projectile)
	UStaticMeshComponent* ProjectileMesh = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	UNiagaraSystem* TrailEffect = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	UNiagaraSystem* ExplosionEffect = nullptr;

	UPROPERTY()
	UNiagaraComponent* TrailEffectComponent = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	UNiagaraSystem* ImpactEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	UMaterialInterface* DecalMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	float TrailOffsetDist = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float DefaultDamage = 0.f;

	float HeadShotAdditionalDamage = 0.f;

	UPROPERTY(VisibleAnywhere)
	AActor* ProjectileOwner = nullptr;

	UPROPERTY(VisibleAnywhere)
	AWeapon* Weapon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CustomProjectile")
	float InitialSpeed = 50000.f;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CustomProjectile")
	//float MaxSpeed = 50000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CustomProjectile")
	float InitialRadius = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CustomProjectile")
	float ProjectileRadius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive")
	bool bIsExplosive = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive")
	bool bVisualizeExplosionRadius = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive")
	float MaxExplosiveDamage = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive")
	float MaxExplosionRadius = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CustomProjectile")
	float HomingAccelerationMagnitude = 3000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Penetration")
	int32 NumPenetrableObjects = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ricochet")
	bool bCanSimpleBounce = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ricochet")
	int32 MaxRicochetCount = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ricochet")
	float MinIncidenceAngle = 5.f;

	int32 CurrentRicochetCount = 0;

public:
	AProjectile();
	void InitProjectile(AActor* OwnerOfProjectile, AWeapon* OwnerWeapon, float additonalDamage = 0.f, float AdditionalRadius = 0.f, int32 NumPenetrable = 0, bool HitScan = false, bool AutoAim = false);
	void InitPhysicsProjectile();
	void InitHitScan();
	void LoadProjectileData();
	void SetWeapon(AWeapon* NewWeapon);
	void SetHomingTarget(bool bIsHoming, AActor* Target);
	void LaunchProjectile(FVector MuzzlePos, FRotator Direction);

private:
	UPROPERTY(VisibleAnywhere)
	bool bIsActivated = false;
	FTimerHandle LifeTimer;
	float LifeSpan = 0.f;
public:
	void StartLifeTimer(float Seconds);
	void StopLifeTimer();
	void DeactiveProjectile();

	void ApplyExplosiveDamage(bool bCanExplosiveDamage, FVector CenterLocation);
	void ApplyDamage(AActor* OtherActor, float DamageAmount, EDamageType DamageType, bool bCanForceDamage, const FName BoneName, TEnumAsByte<EPhysicalSurface> SurfaceType = SurfaceType1, const FVector ImpulseDirection = FVector::ZeroVector, const FVector ImpactPoint = FVector::ZeroVector);
	bool SearchOverlappedActor(FVector CenterLocation, float SearchRadius, TArray<AActor*>& OverlappedActors);

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void OnComponentBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	USphereComponent* GetCollisionComp() const { return CollisionComp; }
	UProjectileMovementComponent* GetProjectileMovement() const { return ProjectileMovement; }

	void SpawnImpactEffect(FVector SpawnLocation, FRotator SpawnRotation);
	void SpawnExplosionEffect(FVector SpawnLocation);
	void SpawnTrailEffect(bool bShouldAttachedToWeapon = false);
	void SpawnDecalEffect(FVector SpawnLocation, FRotator SpawnRotation);
protected:
	bool bShouldUpdateTrailEffect = false;
	void UpdateTrailEffect();


	void DrawSphere(FVector Location, float Radius);

	//protected:
	//	// Called when the game starts or when spawned
	//	virtual void BeginPlay() override;
	//
public:
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void BeginDestroy() override;

	/** Called when the actor falls out of the world 'safely' (below KillZ and such) */
	virtual void FellOutOfWorld(const class UDamageType& dmgType) override;

	/** Called when the Actor is outside the hard limit on world bounds */
	virtual void OutsideWorldBounds() override;


#pragma region Sound
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* HitSound_Default = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* HitSound_Metal = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* HitSound_Glass = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* HitSound_Enemy = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* HitSound_Energy = nullptr;
	void PlaySoundAtLocationByMaterial(EPhysicalSurface SurfaceType, FVector Location);
#pragma endregion

#pragma region HitScan
protected:
	bool bDebugHitScan = false;
	UPROPERTY(VisibleAnywhere)
	bool bIsHitScan = false;
	UPROPERTY(VisibleAnywhere)
	bool bActivatedMeshMovementForHitScan = false;
	TArray<FVector> HitScanEndPoints;
	int32 CurrEndPointIdx = 0;
	float HitScanProjectileVelocity = 0.f;
	UPROPERTY(VisibleAnywhere)
	float DistanceMoved = 0.f;
	UPROPERTY(VisibleAnywhere)
	float TargetDistance = 0.f;
	FVector MovementDirection;
	void PerformHitScan(FVector StartLocation, FVector TraceDirection, float MaxDistance, float SphereRadius, TArray<FVector>& OutHitLocations);
	void InitHitScanProjectileMovement(FVector StartLocation);
	void UpdateHitScanProjectileMovement(float DeltaTime);
public:
	void SetHitScanActive(bool bflag);
	void LaunchHitScan(FVector StartLocation, FVector TraceDirection, FVector MuzzlePos);
#pragma endregion

#pragma region AutoAim
protected:
	UPROPERTY(VisibleAnywhere)
	bool bIsAutoAim = false;
public:
	void LaunchAutoAim(FVector StartLocation, FVector TraceDir, FVector AutoAimDir, FVector MuzzleLoc, float MaxDistance, float AutoAimRadius);
#pragma endregion

#pragma region Penetration
protected:
	int32 NumPenetratedObjects = 0;

	float AdditionalDamage = 0.f;
protected:
	void UpdatePenetration();
	void ResetPenetration();
#pragma endregion

#pragma region HeadShot
protected:
	bool CheckHeadHit(const FHitResult& Hit);
	bool CheckHeadOvelap(const AActor* OverlappedActor, const FHitResult& SweepResult);
#pragma endregion

#pragma region Homing
protected:
	float ExlosionTriggerRadius = 10.f; //TODO: DT

	UPROPERTY()
	AEnemyBase* TargetEnemy = nullptr;
	FVector RecentTargetLocation;
protected:
	bool IsTargetValid();
	bool IsTargetWithInRange();
	void UpdateTargetInfo();
#pragma endregion

#pragma region Impulse
protected:
	bool bCanApplyImpulseToEnemy = false;
	float HitImpulseToEnemy = 100.f;
protected:
	void AddImpulseToEnemy(AActor* OtherActor, FVector Force);
#pragma endregion

#pragma region Ricochet
protected:
	bool CheckRicochetAngle(FVector normal, FVector vel);
	FVector GetReflectionAngle(FVector normal, FVector input);
#pragma endregion;

#pragma region Damage Decay
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DamageDecay")
	float DamageDecayTime = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DamageDecay")
	float DamageDecayRate = 0.5f;
	FTimerHandle DamageDecayTimer;
	void ApplyDamageDecay();
#pragma endregion

#pragma region ProjectileMovement
protected:
	UPROPERTY(VisibleAnywhere)
	bool bUseCustomProjectieMovement = false;
	UPROPERTY(EditAnywhere)
	float PM_Cam_To_d_Len = 0.f;
	float PM_Start_To_d_Len = 0.f;

	float PM_Vel = 0.f;

	float PM_k_by_d = 0.f;

	FVector PM_Cam_Pos;
	FVector PM_d_Pos;
	FVector PM_Start_Pos;
	FVector PM_Dir;
	FVector PM_Dir_d_To_Muzzle;

	FVector PM_Muzzle_Start_Pos;

public:
	void InitProjectileMovement(FVector StartPos, FVector Direction, FVector MuzzlePos);

protected:
	void UpdateProjectileMovement(float DeltaTime);
#pragma endregion

#pragma region CheckStuck
protected:
	UPROPERTY(EditAnywhere)
	int32 MaxStuckCount = 60;

	UPROPERTY(VisibleAnywhere)
	int32 StuckCount = 0;

	UPROPERTY(VisibleAnywhere)
	FVector PrevProjectileLoc = FVector::Zero();

	void CheckAndDeactivateIfStuck();

#pragma endregion

};

