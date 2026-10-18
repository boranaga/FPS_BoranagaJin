#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MapEscapeTrigger.generated.h"

class UBoxComponent;

UCLASS()
class FPS_BORANAGAJIN_API AMapEscapeTrigger : public AActor
{
	GENERATED_BODY()
public:
	AMapEscapeTrigger();
protected:
	virtual void BeginPlay() override;
private:
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> TriggerBox;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escape", meta = (AllowPrivateAccess = "true"))
	bool bEnabled = true;

	bool bTriggered = false;
};