#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ThrowableTrajectoryIndicator.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;
class USceneComponent;
class USplineMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;

/** Client-only, non-colliding 3D mesh preview. Created and owned by AThrowableWeapon. */
UCLASS(Blueprintable, NotPlaceable)
class FPS_BORANAGAJIN_API AThrowableTrajectoryIndicator : public AActor
{
    GENERATED_BODY()

public:
    AThrowableTrajectoryIndicator();

    /** Points are world-space collision-center positions; HitPoint is the surface point. */
    void SetTrajectory(const TArray<FVector>& WorldPoints, bool bHit, const FVector& HitPoint, const FVector& HitNormal);
    void HideTrajectory();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trajectory|Components")
    TObjectPtr<USceneComponent> SceneRoot = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trajectory|Components")
    TObjectPtr<UStaticMeshComponent> ImpactSphere = nullptr;

    // Engine BasicShapes Cylinder/Sphere are provided by default; override in BP if desired.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trajectory|Visual")
    TObjectPtr<UStaticMesh> ArcMesh = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trajectory|Visual")
    TObjectPtr<UStaticMesh> SphereMesh = nullptr;

    // Must be a translucent/unlit material with parameters 'TintColor' and 'Opacity'.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trajectory|Visual")
    TObjectPtr<UMaterialInterface> ArcMaterial = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trajectory|Visual")
    TObjectPtr<UMaterialInterface> ImpactMaterial = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trajectory|Visual", meta = (ClampMin = "0.1"))
    float ArcRadius = 3.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trajectory|Visual", meta = (ClampMin = "1.0"))
    float ImpactSphereRadius = 12.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trajectory|Visual", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ArcStartOpacity = 0.06f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trajectory|Visual", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ArcEndOpacity = 0.65f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trajectory|Visual", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ImpactOpacity = 0.45f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trajectory|Visual")
    FLinearColor TrajectoryColor = FLinearColor(1.0f, 0.025f, 0.035f, 1.0f);

private:
    void EnsureSegmentCount(int32 NeededSegments);
    void UpdateImpactMaterial();

    UPROPERTY(Transient)
    TArray<TObjectPtr<USplineMeshComponent>> ArcSegments;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInstanceDynamic>> SegmentMaterials;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> DynamicImpactMaterial = nullptr;
};
