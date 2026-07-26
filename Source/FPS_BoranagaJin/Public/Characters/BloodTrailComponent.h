#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BloodTrailComponent.generated.h"

class ABloodStainActor;
class UHealthComponent;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FPS_BORANAGAJIN_API UBloodTrailComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBloodTrailComponent();
protected:
	virtual void BeginPlay() override;
public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
private:
	UPROPERTY()
	TObjectPtr<UHealthComponent> HealthComponent; //TODO: 이 방법이 유일한가?
	UPROPERTY(EditAnywhere, Category = "Blood Trail")
	TSubclassOf<ABloodStainActor> BloodStainClass;

	/*
	 * HP가 이 비율 이하이면 지속적으로 출혈한다.
	 */
	UPROPERTY(EditAnywhere, Category = "Blood Trail", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BleedingHealthThresholdPercent = 0.6f;

	/*
	 * 한 번에 이 이상의 피해를 입으면 출혈을 시작한다.
	 */
	UPROPERTY(EditAnywhere, Category = "Blood Trail")
	float HeavyDamageThreshold = 25.f;

	UPROPERTY(EditAnywhere, Category = "Blood Trail")
	float MinimumBloodDropInterval = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Blood Trail")
	float MinimumBloodDropDistance = 100.f;

	UPROPERTY(EditAnywhere, Category = "Blood Trail")
	float GroundTraceDistance = 250.f;

	UPROPERTY(EditAnywhere, Category = "Blood Trail")
	float BleedingDurationAfterHeavyDamage = 8.f;

	bool bIsBleeding = false;

	float LastBloodDropTime = -BIG_NUMBER;
	float BleedingEndTime = 0.f;

	FVector LastBloodDropLocation = FVector::ZeroVector;

	int32 NextSequenceIndex = 0;

public:
	//void NotifyDamageReceived(float AppliedDamage);
	void StartBleeding(bool bLoop = true, float duration = 0.f);
	void StopBleeding();
	bool IsBleeding() const;
	float GetBleedingEndTime() const { return bIsBleeding; }
private:
	void UpdateBleedingState();
	bool CanDropBlood() const;
	bool FindGroundBelow(FHitResult& OutHitResult) const;
	void SpawnBloodStain();
};