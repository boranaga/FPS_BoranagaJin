#pragma once
#include "CoreMinimal.h"
#include "Interface/DamageInterface.h"

/** Native helper for hitscan / projectile / melee callers holding a real hit. */
class FPS_BORANAGAJIN_API FGasImpactLibrary
{
public:
    // Replaces the normal ReceiveDamage call for this hit; never call both.
    static bool TryApplyBulletHit(AActor* Target, float Amount, AActor* Source, const FHitResult& Hit);
    static float ApplyHitDamage(AActor* Target, const FDamageParams& Damage, const FHitResult& Hit);
};
