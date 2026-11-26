#include "Objects/Gas/GasImpactLibrary.h"
#include "Objects/Gas/GasPipe.h"

float FGasImpactLibrary::ApplyHitDamage(AActor* Target, const FDamageParams& Damage, const FHitResult& Hit)
{
    if (!IsValid(Target)) return 0.f;
    if (AGasPipe* Pipe = Cast<AGasPipe>(Target)) return Pipe->ReceiveDamageAtHit(Damage, Hit);
    if (auto* Receiver = Cast<IDamageableInterface>(Target)) return Receiver->ReceiveDamage(Damage);
    return 0.f;
}

bool FGasImpactLibrary::TryApplyBulletHit(AActor* Target, float Amount, AActor* Source, const FHitResult& Hit)
{
    AGasPipe* Pipe = Cast<AGasPipe>(Target);
    if (!IsValid(Pipe)) return false;
    FDamageParams Damage;
    Damage.DamageAmount = Amount;
    Damage.DamageType = EGameDamageType::Bullet;
    Damage.DamageCauser = Source;
    Damage.ImpactPoint = Hit.ImpactPoint;
    Damage.HitDirection = (Hit.TraceEnd-Hit.TraceStart).GetSafeNormal();
    if (Damage.HitDirection.IsNearlyZero()) Damage.HitDirection = -Hit.ImpactNormal;
    Damage.HitBoneName = Hit.BoneName;
    Pipe->ReceiveDamageAtHit(Damage,Hit);
    return true;
}
