#include "Objects/Gas/GasRoomVolume.h"
#include "Objects/Gas/GasSystemSubsystem.h"
#include "Components/BoxComponent.h"
#include "Components/PostProcessComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Interface/DamageInterface.h"

AGasRoomVolume::AGasRoomVolume()
{
    PrimaryActorTick.bCanEverTick = false;
    RoomBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("RoomBounds"));
    SetRootComponent(RoomBounds);
    RoomBounds->InitBoxExtent(FVector(250.f,250.f,150.f));
    RoomBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    RoomBounds->SetGenerateOverlapEvents(false);
    RoomFog = CreateDefaultSubobject<UNiagaraComponent>(TEXT("RoomFog"));
    RoomFog->SetupAttachment(RoomBounds);
    RoomFog->SetAbsolute(false,false,true);
    RoomFog->SetWorldScale3D(FVector::OneVector);
    RoomFog->SetAutoActivate(false);
    GasPostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("GasPostProcess"));
    GasPostProcess->SetupAttachment(RoomBounds);
    GasPostProcess->bUnbound = false;
    GasPostProcess->BlendRadius = 0.f;
    GasPostProcess->BlendWeight = 0.f;
    GasPostProcess->Settings.bOverride_ColorSaturation = true;
    GasPostProcess->Settings.ColorSaturation = FVector4(0.55,0.65,0.5,1.0);
    GasPostProcess->Settings.bOverride_ColorContrast = true;
    GasPostProcess->Settings.ColorContrast = FVector4(0.65,0.65,0.65,1.0);
}
void AGasRoomVolume::PostActorCreated()
{
    Super::PostActorCreated();
    if (!IsTemplate() && !InstanceID.IsValid()) InstanceID = FGuid::NewGuid();
}
#if WITH_EDITOR
void AGasRoomVolume::PostEditImport() { Super::PostEditImport(); if (!IsTemplate()) InstanceID = FGuid::NewGuid(); }
void AGasRoomVolume::PostDuplicate(EDuplicateMode::Type Mode)
{
    Super::PostDuplicate(Mode);
    if (!IsTemplate() && Mode != EDuplicateMode::PIE) InstanceID = FGuid::NewGuid();
}
#endif
void AGasRoomVolume::SetRuntimeSpawned(bool bValue) { bRuntimeSpawned = bValue; if (bValue && !InstanceID.IsValid()) InstanceID = FGuid::NewGuid(); }
void AGasRoomVolume::BeginPlay()
{
    Super::BeginPlay();
    if (!InstanceID.IsValid()) UE_LOG(LogTemp, Error, TEXT("GasRoom %s has no persistent Save ID; assign one in editor."), *GetName());
    if (RoomFogSystem) RoomFog->SetAsset(RoomFogSystem);
    if (auto* S = GetWorld()->GetSubsystem<UGasSystemSubsystem>()) S->RegisterRoom(this);
    UpdateVisuals();
}
void AGasRoomVolume::EndPlay(const EEndPlayReason::Type Reason)
{
    if (GetWorld()) if (auto* S = GetWorld()->GetSubsystem<UGasSystemSubsystem>()) S->UnregisterRoom(this);
    Super::EndPlay(Reason);
}
void AGasRoomVolume::OnBeforeSave()
{
    // Flush pending input without advancing damage or time.
    Concentration = FMath::Clamp(Concentration + PendingGas / GetRoomVolume(), 0.f, 1.f);
    PendingGas = 0.f;
}
void AGasRoomVolume::OnAfterLoad()
{
    PendingGas = 0.f;
    Concentration = FMath::Clamp(Concentration, 0.f, 1.f);
    VentilationRate = FMath::Max(0.f, VentilationRate);
    UpdateVisuals();
}
void AGasRoomVolume::SetVentilationRate(float NewRate) { if (HasAuthority() && FMath::IsFinite(NewRate)) VentilationRate = FMath::Max(0.f, NewRate); }
void AGasRoomVolume::PurgeGas() { if (HasAuthority()) { Concentration=0.f; PendingGas=0.f; ExposureSeconds.Reset(); UpdateVisuals(); } }
void AGasRoomVolume::AddGas(float Amount) { if (HasAuthority() && FMath::IsFinite(Amount) && Amount > 0.f) PendingGas += Amount; }
float AGasRoomVolume::GetRoomVolume() const
{
    const FVector E = RoomBounds->GetScaledBoxExtent();
    return FMath::Max(0.01f, static_cast<float>(8.0 * E.X * E.Y * E.Z / 1000000.0));
}
bool AGasRoomVolume::ContainsPoint(const FVector& Point) const
{
    const FVector P = RoomBounds->GetComponentTransform().InverseTransformPosition(Point);
    const FVector E = RoomBounds->GetUnscaledBoxExtent();
    return FMath::Abs(P.X)<=E.X && FMath::Abs(P.Y)<=E.Y && FMath::Abs(P.Z)<=E.Z;
}
float AGasRoomVolume::GetOpticalDepth(const FVector& Start, const FVector& End) const
{
    if (Concentration <= KINDA_SMALL_NUMBER) return 0.f;
    // Segment vs oriented box in local space; t is unchanged by transforms.
    const FTransform& T = RoomBounds->GetComponentTransform();
    const FVector A = T.InverseTransformPosition(Start);
    const FVector D = T.InverseTransformPosition(End) - A;
    const FVector E = RoomBounds->GetUnscaledBoxExtent();
    double Enter = 0.0, Exit = 1.0;
    for (int32 Axis=0; Axis<3; ++Axis)
    {
        if (FMath::IsNearlyZero(D[Axis])) { if (FMath::Abs(A[Axis]) > E[Axis]) return 0.f; continue; }
        double T0=(-E[Axis]-A[Axis])/D[Axis], T1=(E[Axis]-A[Axis])/D[Axis];
        if (T0>T1) Swap(T0,T1);
        Enter=FMath::Max(Enter,T0); Exit=FMath::Min(Exit,T1);
        if (Exit<=Enter) return 0.f;
    }
    return static_cast<float>(FVector::Distance(Start,End)*(Exit-Enter)/100.0) * Concentration * FMath::Max(0.f,ExtinctionPerMeter);
}
void AGasRoomVolume::Simulate(float Dt)
{
    if (!HasAuthority() || (Concentration <= 0.f && PendingGas <= 0.f && ExposureSeconds.IsEmpty())) return;
    const float Input = PendingGas / GetRoomVolume(); PendingGas = 0.f;
    const float Rate = FMath::Max(0.f,VentilationRate);
    // Exact well-mixed solution for a constant input rate over this step.
    const float Decay = FMath::Exp(-Rate*Dt);
    const float Added = Rate > 0.0001f ? Input*(1.f-Decay)/(Rate*Dt) : Input;
    Concentration = FMath::Clamp(Concentration*Decay+Added,0.f,1.f);
    if (Concentration < 0.0001f && Input <= 0.f) Concentration=0.f;
    UpdateVisuals(); UpdateExposure(Dt);
}
FString AGasRoomVolume::GetVictimKey(const APawn* Pawn) const
{
    if (const auto* Save = Cast<ISaveableActorInterface>(Pawn))
        if (Save->GetInstanceID().IsValid()) return Save->GetInstanceID().ToString();
    if (const auto* PC = Cast<APlayerController>(Pawn->GetController()))
        if (PC->IsLocalController()) return TEXT("LocalPlayer0"); // Single-player project.
    // Placed enemies have stable names within the same map; runtime-only enemies may reset grace on load.
    return Pawn->GetName();
}
void AGasRoomVolume::UpdateExposure(float Dt)
{
    if (Concentration <= ToxicThreshold) { ExposureSeconds.Reset(); return; }
    TArray<FOverlapResult> Hits;
    FCollisionObjectQueryParams Objects(ECC_Pawn);
    Objects.AddObjectTypesToQuery(ECC_GameTraceChannel2); // Existing project PawnEnemy channel.
    FCollisionQueryParams Params(SCENE_QUERY_STAT(GasRoomPawns),false,this);
    GetWorld()->OverlapMultiByObjectType(Hits, RoomBounds->GetComponentLocation(), RoomBounds->GetComponentQuat(), Objects,
        FCollisionShape::MakeBox(RoomBounds->GetScaledBoxExtent()), Params);
    TSet<FString> Present;
    auto* System = GetWorld()->GetSubsystem<UGasSystemSubsystem>();
    const float Strength = FMath::Clamp((Concentration-ToxicThreshold)/FMath::Max(0.001f,1.f-ToxicThreshold),0.f,1.f);
    for (const FOverlapResult& Hit : Hits)
    {
        APawn* Pawn = Cast<APawn>(Hit.GetActor());
        auto* Damageable = IsValid(Pawn) ? Cast<IDamageableInterface>(Pawn) : nullptr;
        if (!Damageable || Damageable->IsDead()) continue;
        FVector Eye; FRotator Rot; Pawn->GetActorEyesViewPoint(Eye,Rot);
        if (!ContainsPoint(Eye) || (System && System->FindRoomAt(Eye)!=this)) continue;
        const FString Key = GetVictimKey(Pawn);
        if (Present.Contains(Key)) continue;
        Present.Add(Key);
        float& Seconds = ExposureSeconds.FindOrAdd(Key);
        const float Grace = FMath::Max(0.f,ExposureGraceSeconds);
        const float Old = Seconds; Seconds = FMath::Min(Seconds+Dt,Grace+Dt);
        const float HarmTime = FMath::Max(0.f, FMath::Min(Dt,Old+Dt-Grace));
        UGasSystemSubsystem::ApplyGasDamage(Pawn,MaxDamagePerSecond*Strength*HarmTime,this,Eye);
    }
    for (auto It=ExposureSeconds.CreateIterator(); It; ++It) if (!Present.Contains(It.Key())) It.RemoveCurrent();
}
void AGasRoomVolume::UpdateVisuals()
{
    if (GetNetMode()==NM_DedicatedServer) return;
    GasPostProcess->BlendWeight = FMath::Clamp(Concentration*MaxPostProcessWeight,0.f,1.f);
    if (!RoomFogSystem) return;
    RoomFog->SetVariableFloat(TEXT("User.GasConcentration"),Concentration);
    RoomFog->SetVariableVec3(TEXT("User.RoomHalfExtent"),RoomBounds->GetScaledBoxExtent());
    if (Concentration > 0.001f) { if (!RoomFog->IsActive()) RoomFog->Activate(); }
    else if (RoomFog->IsActive()) RoomFog->Deactivate();
}
