#include "Items/Weapons/ThrowableWeapon/ThrowableTrajectoryIndicator.h"

#include "Components/SceneComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"

AThrowableTrajectoryIndicator::AThrowableTrajectoryIndicator()
{
    PrimaryActorTick.bCanEverTick = false;
    SetActorEnableCollision(false);

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    ImpactSphere = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ImpactSphere"));
    ImpactSphere->SetupAttachment(SceneRoot);
    ImpactSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ImpactSphere->SetGenerateOverlapEvents(false);
    ImpactSphere->SetCastShadow(false);
    ImpactSphere->SetVisibility(false);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderAsset(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (CylinderAsset.Succeeded())
    {
        ArcMesh = CylinderAsset.Object;
    }

    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereAsset(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (SphereAsset.Succeeded())
    {
        SphereMesh = SphereAsset.Object;
        ImpactSphere->SetStaticMesh(SphereMesh);
    }

    SetActorHiddenInGame(true);
}

void AThrowableTrajectoryIndicator::BeginPlay()
{
    Super::BeginPlay();

    if (SphereMesh && ImpactSphere)
    {
        ImpactSphere->SetStaticMesh(SphereMesh);
    }

    UpdateImpactMaterial();

    if (!ArcMaterial)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ThrowableTrajectory] ArcMaterial is not assigned on %s"), *GetName());
    }
}

void AThrowableTrajectoryIndicator::EnsureSegmentCount(int32 NeededSegments)
{
    while (ArcSegments.Num() < NeededSegments)
    {
        const FName ComponentName(*FString::Printf(TEXT("ArcSegment_%d"), ArcSegments.Num()));
        USplineMeshComponent* Segment = NewObject<USplineMeshComponent>(this, ComponentName);
        Segment->CreationMethod = EComponentCreationMethod::Instance;
        Segment->SetMobility(EComponentMobility::Movable);
        Segment->SetupAttachment(SceneRoot);
        Segment->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Segment->SetGenerateOverlapEvents(false);
        Segment->SetCastShadow(false);
        Segment->SetForwardAxis(ESplineMeshAxis::Z, false); // Engine Cylinder points along local Z
        Segment->SetStaticMesh(ArcMesh);
        Segment->SetVisibility(false);
        AddInstanceComponent(Segment);
        Segment->RegisterComponent();
        ArcSegments.Add(Segment);

        UMaterialInstanceDynamic* MID = nullptr;
        if (ArcMaterial)
        {
            MID = UMaterialInstanceDynamic::Create(ArcMaterial, this);
            Segment->SetMaterial(0, MID);
            MID->SetVectorParameterValue(TEXT("TintColor"), TrajectoryColor);
        }
        SegmentMaterials.Add(MID);
    }
}

void AThrowableTrajectoryIndicator::UpdateImpactMaterial()
{
    if (!ImpactSphere)
    {
        return;
    }

    UMaterialInterface* Material = ImpactMaterial ? ImpactMaterial.Get() : ArcMaterial.Get();
    if (!Material)
    {
        return;
    }

    DynamicImpactMaterial = UMaterialInstanceDynamic::Create(Material, this);
    ImpactSphere->SetMaterial(0, DynamicImpactMaterial);
    DynamicImpactMaterial->SetVectorParameterValue(TEXT("TintColor"), TrajectoryColor);
    DynamicImpactMaterial->SetScalarParameterValue(TEXT("Opacity"), ImpactOpacity);
}

void AThrowableTrajectoryIndicator::SetTrajectory(const TArray<FVector>& WorldPoints, bool bHit, const FVector& HitPoint, const FVector& HitNormal)
{
    if (!ArcMesh || !ArcMaterial || WorldPoints.Num() < 2)
    {
        HideTrajectory();
        return;
    }

    const int32 NumSegments = WorldPoints.Num() - 1;
    EnsureSegmentCount(NumSegments);

    // Actor spawned at identity transform and never attached to the weapon:
    // local positions of these components are world positions.
    const float TubeScale = ArcRadius / 50.f; // Engine basic cylinder radius is 50 cm
    for (int32 Index = 0; Index < ArcSegments.Num(); ++Index)
    {
        USplineMeshComponent* Segment = ArcSegments[Index];
        if (!IsValid(Segment))
        {
            continue;
        }

        const bool bUsed = Index < NumSegments;
        Segment->SetVisibility(bUsed);
        if (!bUsed)
        {
            continue;
        }

        const FVector P0 = WorldPoints[Index];
        const FVector P1 = WorldPoints[Index + 1];
        const FVector Before = Index > 0 ? WorldPoints[Index - 1] : P0;
        const FVector After = Index + 2 < WorldPoints.Num() ? WorldPoints[Index + 2] : P1;

        // Hermite tangents keep adjacent segments visually continuous.
        const FVector StartTangent = Index == 0 ? P1 - P0 : (P1 - Before) * 0.5f;
        const FVector EndTangent = Index == NumSegments - 1 ? P1 - P0 : (After - P0) * 0.5f;

        const FVector Axis = (P1 - P0).GetSafeNormal();
        const FVector UpAxis = FMath::Abs(FVector::DotProduct(Axis, FVector::UpVector)) > 0.95f
            ? FVector::YAxisVector : FVector::UpVector;
        Segment->SetSplineUpDir(UpAxis, false);
        Segment->SetStartScale(FVector2D(TubeScale, TubeScale), false);
        Segment->SetEndScale(FVector2D(TubeScale, TubeScale), false);
        Segment->SetStartAndEnd(P0, StartTangent, P1, EndTangent, true);

        if (SegmentMaterials.IsValidIndex(Index) && SegmentMaterials[Index])
        {
            const float Alpha = (static_cast<float>(Index) + 0.5f) / static_cast<float>(NumSegments);
            const float Opacity = FMath::Lerp(ArcStartOpacity, ArcEndOpacity, Alpha);
            SegmentMaterials[Index]->SetScalarParameterValue(TEXT("Opacity"), Opacity);
        }
    }

    if (ImpactSphere)
    {
        const bool bShowImpact = bHit && (ImpactMaterial || ArcMaterial) && SphereMesh;
        ImpactSphere->SetVisibility(bShowImpact);
        if (bShowImpact)
        {
            // Slight lift from the surface keeps the translucent sphere visible through walls/floors.
            const FVector Center = HitPoint + HitNormal.GetSafeNormal() * ImpactSphereRadius * 0.25f;
            ImpactSphere->SetWorldLocation(Center);
            ImpactSphere->SetWorldScale3D(FVector(ImpactSphereRadius / 50.f));
        }
    }

    SetActorHiddenInGame(false);
}

void AThrowableTrajectoryIndicator::HideTrajectory()
{
    SetActorHiddenInGame(true);
    if (ImpactSphere)
    {
        ImpactSphere->SetVisibility(false);
    }

    for (USplineMeshComponent* Segment : ArcSegments)
    {
        if (IsValid(Segment))
        {
            Segment->SetVisibility(false);
        }
    }
}
