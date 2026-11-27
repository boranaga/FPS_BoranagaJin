

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PoolableActorInterface.h"
#include "ProjectileShell.generated.h"

class UCapsuleComponent;
class UStaticMeshComponent;

UCLASS()
class FPS_BORANAGAJIN_API AProjectileShell : public AActor, public IPoolableActorInterface
{
	GENERATED_BODY()
public:
	// Sets default values for this actor's properties
	AProjectileShell();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void SetOwningPool(UObjectPoolSubsystem* NewPool) override;
	virtual void OnActivateFromPool() override;
	virtual void OnDeactivateToPool() override;
	virtual bool IsActiveInPool() const override { return bIsActiveInPool; }

	/** Called when the actor falls out of the world 'safely' (below KillZ and such) */
	virtual void FellOutOfWorld(const class UDamageType& dmgType) override;

	/** Called when the Actor is outside the hard limit on world bounds */
	virtual void OutsideWorldBounds() override;

protected:
	UPROPERTY(EditAnywhere)
	UCapsuleComponent* CapsuleComponent = nullptr;
	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* ProjectileShellMesh = nullptr;
	UPROPERTY(EditAnywhere)
	float VisibleDuration = 5.f;
	FTimerHandle InvisibleTimer;

protected:
	void InitShell();
	void HideProjectileShell();
public:
	void EjectShell(const FVector EjectLocation, const FRotator EjectRotation, const FVector EjectImpulse);

private:
	UPROPERTY(Transient)
	TObjectPtr<UObjectPoolSubsystem> OwningPool = nullptr;
	bool bIsActiveInPool = false;
	ECollisionEnabled::Type ActiveCollisionMode = ECollisionEnabled::QueryAndPhysics;
	void ResetShellRuntime();

};
