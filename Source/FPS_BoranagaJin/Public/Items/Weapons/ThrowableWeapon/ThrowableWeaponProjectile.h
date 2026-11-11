#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"
#include "Engine/EngineTypes.h"

#include "PoolableActorInterface.h"
#include "Characters/GameDamageType.h"
#include "Data/ThrowableWeaponProjectileData.h"

#include "ThrowableWeaponProjectile.generated.h"

/** First-flight parameters used by the trajectory preview. Read from the same DT/BP as the projectile. */
struct FThrowableProjectileTrajectoryPhysics
{
	float Radius = 10.f;
	float GravityScale = 1.f;
	float MaxSpeed = 0.f;
	float MaxFlightTime = 0.f; // 0: no fuse/lifespan limit
	ECollisionChannel ObjectChannel = ECC_GameTraceChannel3;
	FCollisionResponseContainer CollisionResponses;
};

class AThrowableWeapon;
class AEnemyBase;

class USphereComponent;
class UProjectileMovementComponent;
class UNiagaraSystem;
class UNiagaraComponent;
class USoundBase;
class UMaterialInterface;
class UStaticMeshComponent;

DECLARE_DELEGATE(FHeadShotDelegate);
DECLARE_DELEGATE(FBodyShotDelegate);

UCLASS(config = Game)
class FPS_BORANAGAJIN_API AThrowableWeaponProjectile : public AActor, public IPoolableActorInterface
{
	GENERATED_BODY()
public:
	FHeadShotDelegate OnHeadShot;
	FBodyShotDelegate OnBodyShot;

	const FThrowableWeaponProjectileData* GetProjectileData() const { return ProjectileData; } //MEMO: �ʿ��Ѱ�?
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Data") FDataTableRowHandle ProjectileDataTableHandle;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Data") bool bUseDataTable = true;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Data", meta = (EditCondition = "!bUseDataTable", ShowOnlyInnerProperties)) FThrowableWeaponProjectileData BlueprintProjectileData;

	const FThrowableWeaponProjectileData* ProjectileData = nullptr;

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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect") UNiagaraSystem* FlashEffect = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect") UNiagaraSystem* SmokeEffect = nullptr;

	UPROPERTY() UNiagaraComponent* TrailEffectComponent = nullptr;
	UPROPERTY() UNiagaraComponent* SmokeEffectComponent = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	UNiagaraSystem* ImpactEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	UMaterialInterface* ImpactDecalMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	float TrailOffsetDist = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float DefaultDamage = 0.f;

	float HeadShotAdditionalDamage = 0.f;

	UPROPERTY(VisibleAnywhere)
	AActor* ProjectileOwner = nullptr;

	UPROPERTY(VisibleAnywhere)
	AThrowableWeapon* ThrowableWeapon = nullptr;

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

	// Unified throwable payload runtime data
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Throwable|Runtime") EThrowableProjectileType ProjectileType = EThrowableProjectileType::Grenade;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Throwable|Runtime") float FuseTime = 3.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Throwable|Runtime") bool bDetonateOnImpact = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Throwable|Runtime") bool bCanAffectOwner = true;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Throwable|Runtime") float MinExplosiveDamage = 10.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Throwable|Runtime") float ExplosionDamageFalloffExponent = 1.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Throwable|Runtime") bool bExplosionRequiresLineOfSight = true;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Throwable|Runtime") float FlashRadius = 1800.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Throwable|Runtime") float FlashMinDuration = 0.35f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Throwable|Runtime") float FlashMaxDuration = 4.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Throwable|Runtime") float FlashDistanceFalloffExponent = 1.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Throwable|Runtime") bool bFlashRequiresLineOfSight = true;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Throwable|Runtime") bool bFlashFadeAudio = true;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Throwable|Runtime") float SmokeDuration = 15.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Throwable|Runtime") float SmokeRadius = 500.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Throwable|Runtime") bool bSmokeBlocksVisibilityTrace = true;
	UPROPERTY() USoundBase* DetonationSound = nullptr;
	UPROPERTY() USoundBase* FlashSound = nullptr;
	UPROPERTY() USoundBase* SmokeStartSound = nullptr;

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
	AThrowableWeaponProjectile();
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void BeginDestroy() override;

	/** Called when the actor falls out of the world 'safely' (below KillZ and such) */
	virtual void FellOutOfWorld(const class UDamageType& dmgType) override;

	/** Called when the Actor is outside the hard limit on world bounds */
	virtual void OutsideWorldBounds() override;
	void InitProjectile(AActor* OwnerOfProjectile, AThrowableWeapon* OwnerWeapon, float additonalDamage = 0.f, int32 NumPenetrable = 0, bool HitScan = false);
	void InitPhysicsProjectile();
	void InitHitScan();
	void LoadProjectileData();
	void SetWeapon(AThrowableWeapon* NewWeapon);

	/** Blueprint/CDO-safe read: no projectile is spawned or activated for prediction. */
	bool GetTrajectoryPhysics(FThrowableProjectileTrajectoryPhysics& OutPhysics) const;
	void LaunchProjectile(FVector StartPos, FRotator Direction);
	void LaunchProjectile(
		const FVector& StartPosition,
		const FVector& Direction,
		float LaunchSpeed
	);

	UFUNCTION(BlueprintCallable, Category = "Throwable|Payload") void StartFuse();
	UFUNCTION(BlueprintCallable, Category = "Throwable|Payload") void ActivatePayload();

	UFUNCTION(BlueprintImplementableEvent, Category = "Throwable|Payload") void BP_OnPayloadActivated(EThrowableProjectileType ActivatedType, FVector ActivationLocation);
	UFUNCTION(BlueprintImplementableEvent, Category = "Throwable|Flashbang") void BP_OnFlashActorAffected(AActor* AffectedActor, float Intensity, float Duration);
	UFUNCTION(BlueprintImplementableEvent, Category = "Throwable|Smoke") void BP_OnSmokeStarted(FVector CenterLocation, float Radius, float Duration);

protected:
	void ApplyProjectileData();
	void ConfigureFlightCollision();
	void StopProjectileMovement();
	void StopTrailEffect();
	void ClearThrowableTimers();

	UFUNCTION() void OnFuseExpired();
	void ActivateGrenadePayload();
	void ActivateFlashbangPayload();
	void ActivateSmokePayload();
	void FinishSmokeEffect();

	void ApplyFlashEffect(const FVector& CenterLocation);
	float CalculateFlashIntensity(const FVector& CenterLocation, const AActor* TargetActor) const;
	bool HasEffectLineOfSight(const FVector& CenterLocation, const AActor* TargetActor) const;
	bool FindActorsInRadius(const FVector& CenterLocation, float SearchRadius, TArray<AActor*>& OutActors, bool bIncludeOwner) const;

private:
	UPROPERTY(VisibleAnywhere)
	bool bIsActivated = false;
	FTimerHandle LifeTimer;
	FTimerHandle FuseTimerHandle;
	FTimerHandle PersistentEffectTimerHandle;
	float LifeSpan = 0.f;
	bool bUseThrowableBehavior = true;
	bool bPayloadActivated = false;
public:
	void StartLifeTimer(float Seconds);
	void StopLifeTimer();

	void ApplyExplosiveDamage(bool bCanExplosiveDamage, FVector CenterLocation);
	void ApplyDamage(AActor* OtherActor, float DamageAmount, EGameDamageType DamageType, bool bCanForceDamage, const FName BoneName, TEnumAsByte<EPhysicalSurface> SurfaceType = SurfaceType1, const FVector ImpulseDirection = FVector::ZeroVector, const FVector ImpactPoint = FVector::ZeroVector);
	bool SearchOverlappedActor(FVector CenterLocation, float SearchRadius, TArray<AActor*>& OverlappedActors);

	UFUNCTION()
	void HandleProjectileBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity);

	UFUNCTION()
	void HandleProjectileStop(const FHitResult& ImpactResult);

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
	void PlaySoundAtLocationByMaterial(EPhysicalSurface SurfaceType, FVector Location); //TODO: SoundSystem�� ����
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

#pragma region AISense_Hearing
protected:
	UPROPERTY(EditAnywhere, Category = "Noise")
	float ProjectileHitNoiseLoudness = 2.0f;

	UPROPERTY(EditAnywhere, Category = "Noise")
	float ProjectileHitNoiseRange = 3000.f;
protected:
	void ReportNoiseToAI();
#pragma endregion

#pragma region PoolableActorInterface
public:
	virtual void SetOwningPool(UObjectPoolSubsystem* NewPool) override;
	virtual void OnActivateFromPool() override;
	virtual void OnDeactivateToPool() override;
	virtual bool IsActiveInPool() const override;
	void DeactivateProjectile();
private:
	UPROPERTY()
	TObjectPtr<UObjectPoolSubsystem> OwningPool;
	bool bIsActiveInPool = false;
#pragma endregion
};

