#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "FireAreaComponent.generated.h"

class ADestructibleObject;

UCLASS()
class FPS_BORANAGAJIN_API UFireAreaComponent : public USphereComponent
{
	GENERATED_BODY()

public:
	UFireAreaComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	void ActivateFireArea(AActor* InFireCauser);
	void DeactivateFireArea();

protected:
	UFUNCTION()
	void ApplyFireAreaDamage();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire Area")
	float FireAreaDamage = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire Area")
	float DamageInterval = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire Area")
	float FireAreaRadius = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire Area")
	bool bDamageOwner = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire Area")
	bool bIgniteOtherDestructibleObjects = true;

	UPROPERTY()
	AActor* FireCauser;

	FTimerHandle FireDamageTimerHandle;
};