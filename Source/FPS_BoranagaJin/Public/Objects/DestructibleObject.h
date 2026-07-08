#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Characters/DamageInterface.h"
#include "DestructibleObject.generated.h"

class UStaticMeshComponent;
class UNiagaraSystem;
class USoundBase;

UCLASS()
class FPS_BORANAGAJIN_API ADestructibleObject : public AActor, public IDamageInterface
{
	GENERATED_BODY()
public:
	ADestructibleObject();
protected:
	virtual void BeginPlay() override;
public:

	//TODO: Delete
	//virtual void TakeObjectDamage(
	//	float Damage,
	//	const FVector& HitLocation,
	//	const FVector& HitDirection,
	//	AActor* DamageCauser
	//);

	virtual float ReceiveDamage(const FDamageParams& DamageData) override;

	//TODO: 필요한가?
	virtual bool IsDead() const;

protected:
	UFUNCTION()
	virtual void BreakObject(const FVector& HitLocation, const FVector& HitDirection);

	UFUNCTION()
	virtual void HideBrokenMesh();
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
	USceneComponent* SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
	UStaticMeshComponent* IntactMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
	UStaticMeshComponent* BrokenMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Destruction")
	float MaxHealth = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Destruction")
	float CurrentHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Destruction")
	float BreakImpulse = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Destruction")
	float BrokenMeshLifeTime = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Destruction")
	bool bDestroyActorAfterBreak = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Destruction")
	bool bHideBrokenMeshAfterDelay = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Destruction")
	bool bDestroyed = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	UNiagaraSystem* BreakEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	USoundBase* BreakSound;

	FTimerHandle HideBrokenMeshTimerHandle;
};