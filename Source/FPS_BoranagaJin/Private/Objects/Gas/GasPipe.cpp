#include "Objects/Gas/GasPipe.h"
#include "Objects/Gas/GasRoomVolume.h"
#include "Objects/Gas/GasSystemSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DecalComponent.h"
#include "Components/AudioComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"

AGasPipe::AGasPipe()
{
    MaxHealth = 300.f;
    CurrentHealth = MaxHealth;
    bCanBurn = false;
    bCanIgniteFromDamage = false;
    // The fixed pipe outlet remains alive even when the inherited mesh breaks.
    bDestroyActorAfterBreak = false;
}
void AGasPipe::BeginPlay()
{
    bDestroyActorAfterBreak = false;
    Super::BeginPlay();
    if (auto* S = GetWorld()->GetSubsystem<UGasSystemSubsystem>()) S->RegisterPipe(this);
    RebuildEffects();
}
void AGasPipe::EndPlay(const EEndPlayReason::Type Reason)
{
    if (GetWorld()) if (auto* S = GetWorld()->GetSubsystem<UGasSystemSubsystem>()) S->UnregisterPipe(this);
    ClearEffects();
    Super::EndPlay(Reason);
}
void AGasPipe::OnBeforeSave() { Super::OnBeforeSave(); }
void AGasPipe::OnAfterLoad()
{
    bDestroyActorAfterBreak = false;
    Super::OnAfterLoad();
    SetLifeSpan(0.f);
    // Never serialize Niagara/audio/decal components or duplicate base health.
    Holes.RemoveAll([](const FGasPipeHole& H){return H.LocalPoint.ContainsNaN() || H.LocalNormal.ContainsNaN() || H.LocalNormal.IsNearlyZero();});
    RebuildEffects();
}
void AGasPipe::RestoreDestructibleObject()
{
    Super::RestoreDestructibleObject();
    Holes.Reset();
    RebuildEffects();
    // Existing room gas and valve state deliberately remain independent.
}
void AGasPipe::SetGasSupplyEnabled(bool bEnabled)
{
    if (!HasAuthority() || bEnabled==bGasSupplyEnabled) return;
    bGasSupplyEnabled=bEnabled;
    RebuildEffects();
}
float AGasPipe::ReceiveDamage(const FDamageParams& Data) { return ReceiveDamageInternal(Data,nullptr); }
float AGasPipe::ReceiveDamageAtHit(const FDamageParams& Data, const FHitResult& Hit) { return ReceiveDamageInternal(Data,&Hit); }
float AGasPipe::ReceiveDamageInternal(const FDamageParams& Data, const FHitResult* ExactHit)
{
    if (!HasAuthority() || !CanBeDamagedByAI() || !FMath::IsFinite(Data.DamageAmount) || Data.DamageAmount<=0.f) return 0.f;
    FDamageParams Resolved=Data;
    FVector Point, Normal;
    const bool bPunctureType = Data.DamageType==EGameDamageType::Bullet || Data.DamageType==EGameDamageType::Melee;
    if (bPunctureType)
    {
        bool bValid = false;
        if (ExactHit && !ExactHit->bStartPenetrating && ExactHit->GetComponent()==IntactMesh && !ExactHit->ImpactPoint.ContainsNaN())
        {
            Point=ExactHit->ImpactPoint; Normal=ExactHit->ImpactNormal.GetSafeNormal(); bValid=!Normal.IsNearlyZero() && !Normal.ContainsNaN();
        }
        if (!bValid) bValid=ResolvePuncture(Data,Point,Normal);
        if (bValid)
        {
            Resolved.ImpactPoint=Point;
            if (Resolved.HitDirection.IsNearlyZero()) Resolved.HitDirection=-Normal;
            if (Data.DamageAmount>=MinimumPunctureDamage) AddHole(Point,Normal);
        }
    }
    const float Applied=Super::ReceiveDamage(Resolved);
    return Applied;
}
bool AGasPipe::ResolvePuncture(const FDamageParams& Data, FVector& Point, FVector& Normal) const
{
    if (!IsValid(IntactMesh) || Data.ImpactPoint.ContainsNaN()) return false;
    const FBox Bounds = IntactMesh->Bounds.GetBox().ExpandBy(5.f);
    FVector Direction=Data.HitDirection.GetSafeNormal();
    // Some existing projectile paths store the *outward normal* in ImpulseDirection.
    // Orient the probe toward the surface using the source position when available.
    if (Direction.IsNearlyZero()) Direction=Data.ImpulseDirection.GetSafeNormal();
    if (IsValid(Data.DamageCauser))
    {
        const FVector Incoming=(Data.ImpactPoint-Data.DamageCauser->GetActorLocation()).GetSafeNormal();
        if (Direction.IsNearlyZero()) Direction=Incoming;
        else if (FVector::DotProduct(Direction,Incoming)<0.f) Direction=-Direction;
    }
    FCollisionQueryParams Params(SCENE_QUERY_STAT(GasPipeImpact),true);
    FHitResult Hit;
    if (Bounds.IsInsideOrOn(Data.ImpactPoint) && !Direction.IsNearlyZero())
    {
        // Keep the incoming impact point exactly; trace only to recover the normal.
        const float Probe=12.f;
        if ((IntactMesh->LineTraceComponent(Hit,Data.ImpactPoint-Direction*Probe,Data.ImpactPoint+Direction*Probe,Params)
            && FVector::DistSquared(Hit.ImpactPoint,Data.ImpactPoint)<=FMath::Square(5.f)) ||
            (IntactMesh->LineTraceComponent(Hit,Data.ImpactPoint+Direction*Probe,Data.ImpactPoint-Direction*Probe,Params)
            && FVector::DistSquared(Hit.ImpactPoint,Data.ImpactPoint)<=FMath::Square(5.f)))
        { Point=Data.ImpactPoint; Normal=Hit.ImpactNormal.GetSafeNormal(); return !Normal.IsNearlyZero(); }
    }
    // Legacy melee callers often omit ImpactPoint entirely. Trace from attacker
    // toward the pipe; do not use the mesh center as the wound position.
    if (Data.DamageType==EGameDamageType::Melee && IsValid(Data.DamageCauser))
    {
        FVector Start=Data.DamageCauser->GetActorLocation();
        FRotator Rot; Data.DamageCauser->GetActorEyesViewPoint(Start,Rot);
        if (IntactMesh->LineTraceComponent(Hit,Start,IntactMesh->Bounds.Origin,Params))
        { Point=Hit.ImpactPoint; Normal=Hit.ImpactNormal.GetSafeNormal(); return !Normal.IsNearlyZero(); }
        Start=Data.DamageCauser->GetActorLocation();
        if (IntactMesh->LineTraceComponent(Hit,Start,IntactMesh->Bounds.Origin,Params))
        { Point=Hit.ImpactPoint; Normal=Hit.ImpactNormal.GetSafeNormal(); return !Normal.IsNearlyZero(); }
    }
    // Legacy projectile hits may use simple collision rather than complex collision.
    if (Bounds.IsInsideOrOn(Data.ImpactPoint) && !Direction.IsNearlyZero() && !Data.ImpactPoint.IsNearlyZero())
    { Point=Data.ImpactPoint; Normal=-Direction; return true; }
    return false;
}
void AGasPipe::GetWorldHole(const FGasPipeHole& Hole, FVector& Point, FVector& Normal) const
{
    const FTransform& T=IntactMesh->GetComponentTransform();
    Point=T.TransformPosition(Hole.LocalPoint);
    const FVector Scale=T.GetScale3D();
    const FVector SafeScale(FMath::IsNearlyZero(Scale.X)?1.0:Scale.X,FMath::IsNearlyZero(Scale.Y)?1.0:Scale.Y,FMath::IsNearlyZero(Scale.Z)?1.0:Scale.Z);
    Normal=T.TransformVectorNoScale(Hole.LocalNormal/SafeScale).GetSafeNormal();
}
bool AGasPipe::AddHole(const FVector& Point, const FVector& Normal)
{
    if (!IntactMesh || Point.ContainsNaN() || Normal.ContainsNaN() || Normal.IsNearlyZero()) return false;
    for (const auto& H : Holes)
    {
        FVector P,N; GetWorldHole(H,P,N);
        if (FVector::DistSquared(P,Point)<=FMath::Square(FMath::Max(0.f,HoleMergeDistance))) return false;
    }
    if (Holes.Num()>=FMath::Clamp(MaxHoles,1,32)) return false;
    const FTransform& T=IntactMesh->GetComponentTransform();
    FGasPipeHole H;
    H.LocalPoint=T.InverseTransformPosition(Point);
    H.LocalNormal=(T.InverseTransformVectorNoScale(Normal)*T.GetScale3D()).GetSafeNormal();
    Holes.Add(H);
    RebuildEffects();
    return true;
}
void AGasPipe::BreakObject(const FVector& Point, const FVector& Direction)
{
    if (bDestroyed) return;
    // For non-puncture destruction, create a stationary outlet on the pipe.
    if (Holes.IsEmpty() && IntactMesh)
    {
        FVector D=Direction.GetSafeNormal(); if (D.IsNearlyZero()) D=GetActorForwardVector();
        const FBox B=IntactMesh->Bounds.GetBox();
        const FVector Center=B.GetCenter();
        FHitResult Hit; FCollisionQueryParams Params(SCENE_QUERY_STAT(GasPipeBreak),true);
        if (IntactMesh->LineTraceComponent(Hit,Center-D*(B.GetExtent().Size()+20.f),Center,Params)) AddHole(Hit.ImpactPoint,Hit.ImpactNormal);
        else AddHole(B.IsInsideOrOn(Point)?Point:Center,-D);
    }
    bDestroyActorAfterBreak=false;
    Super::BreakObject(Point,Direction);
    SetLifeSpan(0.f);
    RebuildEffects();
}
void AGasPipe::ClearEffects()
{
    for (auto& C:Jets) if (IsValid(C)) C->DestroyComponent();
    for (auto& C:Decals) if (IsValid(C)) C->DestroyComponent();
    for (auto& C:LeakSounds) if (IsValid(C)) { C->Stop(); C->DestroyComponent(); }
    Jets.Reset(); Decals.Reset(); LeakSounds.Reset();
}
void AGasPipe::RebuildEffects()
{
    ClearEffects();
    if (!GetWorld() || !HasActorBegunPlay() || GetNetMode()==NM_DedicatedServer || !IntactMesh || !SceneRoot) return;
    UpdateHoleMaterial();
    for (const auto& H : Holes)
    {
        FVector P,N; GetWorldHole(H,P,N);
        if (HoleDecalMaterial && !bDestroyed)
        {
            auto* D=NewObject<UDecalComponent>(this);
            D->SetupAttachment(IntactMesh); D->SetAbsolute(false,false,true); D->SetDecalMaterial(HoleDecalMaterial);
            D->DecalSize=FVector(2.f,HoleRadius,HoleRadius);
            D->RegisterComponent(); D->SetWorldLocationAndRotation(P+N*0.1f,(-N).Rotation()); Decals.Add(D);
        }
        if (!bGasSupplyEnabled) continue;
        if (JetEffect)
        {
            auto* J=NewObject<UNiagaraComponent>(this);
            J->SetupAttachment(SceneRoot); J->SetAbsolute(false,false,true); J->SetAutoActivate(false); J->SetAsset(JetEffect);
            J->RegisterComponent(); J->SetWorldLocationAndRotation(P,N.Rotation());
            J->SetVariableFloat(TEXT("User.JetSpeed"),JetSpeed);
            J->SetVariableFloat(TEXT("User.JetLength"),JetLength);
            J->SetVariableFloat(TEXT("User.JetHalfAngle"),JetHalfAngle);
            J->SetVariableFloat(TEXT("User.HoleRadius"),HoleRadius);
            J->Activate(true); Jets.Add(J);
        }
        if (LeakLoopSound)
        {
            auto* S=NewObject<UAudioComponent>(this);
            S->SetupAttachment(SceneRoot); S->bAutoActivate=false; S->SetSound(LeakLoopSound);
            S->RegisterComponent(); S->SetWorldLocation(P); S->Play(); LeakSounds.Add(S);
        }
    }
}
void AGasPipe::Simulate(float Dt)
{
    if (!HasAuthority() || Holes.IsEmpty() || !IntactMesh) return;
    if (!bDestroyed) UpdateHoleMaterial();
    if (!bGasSupplyEnabled) return;
    auto* System=GetWorld()->GetSubsystem<UGasSystemSubsystem>();
    const float Length=FMath::Max(1.f,JetLength);
    const float Tan=FMath::Tan(FMath::DegreesToRadians(FMath::Clamp(JetHalfAngle,1.f,85.f)));
    const float BroadRadius=FMath::Sqrt(FMath::Square(Length*0.5f)+FMath::Square(HoleRadius+Length*Tan));
    TMap<TWeakObjectPtr<APawn>,float> MaxDamage;
    for (int32 I=0; I<Holes.Num(); ++I)
    {
        FVector P,N; GetWorldHole(Holes[I],P,N);
        if (Jets.IsValidIndex(I) && IsValid(Jets[I])) Jets[I]->SetWorldLocationAndRotation(P,N.Rotation());
        if (LeakSounds.IsValidIndex(I) && IsValid(LeakSounds[I])) LeakSounds[I]->SetWorldLocation(P);
        const FVector Start=P+N*2.f;
        AGasRoomVolume* Room=IsValid(TargetRoom)?TargetRoom.Get():(System?System->FindRoomAt(Start):nullptr);
        if (Room) Room->AddGas(FMath::Max(0.f,GasFlowPerHole)*Dt);
        TArray<FOverlapResult> Hits;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(GasJet),false,this);
        FCollisionObjectQueryParams Objects(ECC_Pawn);
        Objects.AddObjectTypesToQuery(ECC_GameTraceChannel2); // Project PawnEnemy channel.
        GetWorld()->OverlapMultiByObjectType(Hits,P+N*Length*0.5f,FQuat::Identity,Objects,FCollisionShape::MakeSphere(BroadRadius),Params);
        TSet<TWeakObjectPtr<APawn>> Seen;
        for (const FOverlapResult& H:Hits)
        {
            APawn* Pawn=Cast<APawn>(H.GetActor());
            if (!IsValid(Pawn) || Seen.Contains(Pawn)) continue;
            Seen.Add(Pawn);
            FVector Eye; FRotator Rot; Pawn->GetActorEyesViewPoint(Eye,Rot);
            const FVector Samples[]={Pawn->GetActorLocation(),Eye};
            for (const FVector& Sample:Samples)
            {
                const FVector V=Sample-P; const double Along=FVector::DotProduct(V,N);
                if (Along<0.0 || Along>Length || (V-N*Along).SizeSquared()>FMath::Square(HoleRadius+Along*Tan)) continue;
                FHitResult Block;
                if (GetWorld()->LineTraceSingleByChannel(Block,Start,Sample,JetOcclusionChannel,Params) && Block.GetActor()!=Pawn) continue;
                const float Falloff=FMath::Lerp(1.f,0.35f,static_cast<float>(Along/Length));
                float& Damage=MaxDamage.FindOrAdd(Pawn);
                Damage=FMath::Max(Damage,FMath::Max(0.f,JetDamagePerSecond)*Falloff*Dt);
            }
        }
    }
    // Multiple nearby holes in the same pipe do not multiply direct damage.
    for (const auto& Pair:MaxDamage) if (APawn* Pawn=Pair.Key.Get())
        UGasSystemSubsystem::ApplyGasDamage(Pawn,Pair.Value,this,Pawn->GetActorLocation());
}

void AGasPipe::UpdateHoleMaterial()
{
    if (!bUseMaskedHoleMaterial || !IntactMesh || GetNetMode()==NM_DedicatedServer) return;
    // Use the existing assigned masked material; no hard-coded asset reference.
    if (HoleMaterials.IsEmpty())
    {
        for (int32 Slot=0; Slot<IntactMesh->GetNumMaterials(); ++Slot)
            if (auto* MID=IntactMesh->CreateDynamicMaterialInstance(Slot)) HoleMaterials.Add(MID);
    }
    const int32 Count=bDestroyed?0:FMath::Min(Holes.Num(),32);
    for (const auto& MID:HoleMaterials)
    {
        if (!IsValid(MID)) continue;
        MID->SetScalarParameterValue(TEXT("GasHoleCount"),static_cast<float>(Count));
        MID->SetScalarParameterValue(TEXT("GasHoleRadius"),HoleRadius);
        for (int32 Index=0;Index<Count;++Index)
        {
            FVector Point,Normal; GetWorldHole(Holes[Index],Point,Normal);
            MID->SetVectorParameterValue(FName(*FString::Printf(TEXT("GasHoleWorld_%d"),Index)),
                FLinearColor(static_cast<float>(Point.X),static_cast<float>(Point.Y),static_cast<float>(Point.Z),1.f));
        }
    }
}
