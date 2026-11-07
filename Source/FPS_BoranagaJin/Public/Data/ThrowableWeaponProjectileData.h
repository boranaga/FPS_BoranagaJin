#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ThrowableWeaponProjectileData.generated.h"

class UNiagaraSystem;
class UMaterialInterface;
class USoundBase;

UENUM(BlueprintType)
enum class EThrowableProjectileType : uint8
{
	Grenade		UMETA(DisplayName = "Grenade"),
	Flashbang	UMETA(DisplayName = "Flashbang"),
	SmokeGrenade UMETA(DisplayName = "Smoke Grenade")
};

USTRUCT(BlueprintType)
struct FPS_BORANAGAJIN_API FThrowableWeaponProjectileData : public FTableRowBase
{
	GENERATED_BODY()

	// ---------------------------------------------------------------------
	// General
	// ---------------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|General") EThrowableProjectileType ProjectileType = EThrowableProjectileType::Grenade;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|General", meta = (ClampMin = "0.0")) float FuseTime = 3.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|General", meta = (ClampMin = "0.0")) float InitialLifeSpan = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|General") bool bDetonateOnImpact = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|General") bool bCanAffectOwner = true;

	// ---------------------------------------------------------------------
	// Projectile Movement
	// Launch speed is normally supplied by AThrowableWeapon.
	// MaxSpeed = 0 means no speed clamp.
	// ---------------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|Movement", meta = (ClampMin = "0.0")) float InitialSpeed = 1000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|Movement", meta = (ClampMin = "0.0")) float MaxSpeed = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|Movement", meta = (ClampMin = "0.0")) float InitialRadius = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|Movement", meta = (ClampMin = "0.0")) float ProjectileGravityScale = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|Movement") bool bCanSimpleBounce = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|Movement", meta = (ClampMin = "0.0")) float Bounciness = 0.35f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|Movement", meta = (ClampMin = "0.0")) float Friction = 0.6f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|Movement", meta = (ClampMin = "0.0")) float BounceVelocityStopSimulatingThreshold = 50.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|Movement") bool bForceSubStepping = true;

	// ---------------------------------------------------------------------
	// Effects
	// ---------------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|Effect") UNiagaraSystem* TrailEffect = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|Effect") UNiagaraSystem* ImpactEffect = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|Effect") UNiagaraSystem* ExplosionEffect = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|Effect") UNiagaraSystem* FlashEffect = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|Effect") UNiagaraSystem* SmokeEffect = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|Effect") UMaterialInterface* ImpactDecal = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|Effect", meta = (ClampMin = "0.0")) float TrailOffsetDist = 100.f;

	// ---------------------------------------------------------------------
	// Sound
	// ---------------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|Sound") USoundBase* HitSound = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|Sound") USoundBase* HitSound_Default = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|Sound") USoundBase* HitSound_Metal = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|Sound") USoundBase* HitSound_Glass = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|Sound") USoundBase* HitSound_Enemy = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|Sound") USoundBase* HitSound_Energy = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|Sound") USoundBase* DetonationSound = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|Sound") USoundBase* FlashSound = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|Sound") USoundBase* SmokeStartSound = nullptr;

	// ---------------------------------------------------------------------
	// Direct-hit compatibility data (legacy projectile features)
	// ---------------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|Damage") float DefaultDamage = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|Damage") float HeadShotAdditionalDamage = 1.f;

	// ---------------------------------------------------------------------
	// Grenade
	// ---------------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|Grenade") bool bIsExplosive = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|Grenade") bool bVisualizeExplosionRadius = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|Grenade", meta = (ClampMin = "0.0")) float MaxExplosiveDamage = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|Grenade", meta = (ClampMin = "0.0")) float MinExplosiveDamage = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|Grenade", meta = (ClampMin = "0.0")) float MaxExplosionRadius = 450.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|Grenade", meta = (ClampMin = "0.01")) float ExplosionDamageFalloffExponent = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|Grenade") bool bExplosionRequiresLineOfSight = true;

	// ---------------------------------------------------------------------
	// Flashbang
	// ---------------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|Flashbang", meta = (ClampMin = "0.0")) float FlashRadius = 1800.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|Flashbang", meta = (ClampMin = "0.0")) float FlashMinDuration = 0.35f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|Flashbang", meta = (ClampMin = "0.0")) float FlashMaxDuration = 4.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|Flashbang", meta = (ClampMin = "0.01")) float FlashDistanceFalloffExponent = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|Flashbang") bool bFlashRequiresLineOfSight = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|Flashbang") bool bFlashFadeAudio = true;

	// ---------------------------------------------------------------------
	// Smoke Grenade
	// ---------------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|Smoke", meta = (ClampMin = "0.0")) float SmokeDuration = 15.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|Smoke", meta = (ClampMin = "0.0")) float SmokeRadius = 500.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|Smoke") bool bSmokeBlocksVisibilityTrace = true;

	// ---------------------------------------------------------------------
	// Legacy / optional features preserved from the existing projectile
	// ---------------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|Impulse") bool bCanApplyImpulseToEnemy = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|Impulse") float HitImpulseToEnemy = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|Ricochet") int32 MaxRicochetCount = 4;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|Ricochet") float MinIncidenceAngle = 5.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|HitScan") bool bDebugHitScan = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable|CustomMovement") float PM_Cam_To_d_Len = 500.f;
};
