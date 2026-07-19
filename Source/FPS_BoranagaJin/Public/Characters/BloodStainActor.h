#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BloodStainActor.generated.h"

class UDecalComponent;

UCLASS()
class FPS_BORANAGAJIN_API ABloodStainActor : public AActor
{
	GENERATED_BODY()
public:
	ABloodStainActor();
protected:
	virtual void BeginPlay() override;
private:
	UPROPERTY(VisibleAnywhere, Category = "Blood")
	TObjectPtr<UDecalComponent> DecalComponent;
	UPROPERTY()
	TWeakObjectPtr<AActor> TrailOwner;
	UPROPERTY(VisibleAnywhere, Category = "Blood")
	int32 SequenceIndex = INDEX_NONE;
	float BloodSpawnTime = 0.f;
public:
	void InitializeBloodStain(AActor* InTrailOwner, int32 InSequenceIndex, const FVector& SurfaceNormal);

	AActor* GetTrailOwner() const;
	int32 GetSequenceIndex() const;
	float GetBloodSpawnTime() const;
};