#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/DamageInterface.h"
#include "Interface/SaveableActorInterface.h"
#include "DestructibleObject.generated.h"

class UFireAreaComponent;

class UStaticMeshComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class USoundBase;

UCLASS()
class FPS_BORANAGAJIN_API ADestructibleObject : public AActor, public IDamageableInterface, public ISaveableActorInterface
{
	GENERATED_BODY()
public:
	ADestructibleObject();

#pragma region SaveableActorInterface
#if WITH_EDITOR
	virtual void PostEditImport() override;
	virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode) override;
#endif
public:
	virtual FGuid GetInstanceID() const override
	{
		return InstanceID;
	}

	virtual bool ShouldSaveTransform() const override
	{
		return false;
	}
	virtual bool IsRuntimeSpawned() const override
	{
		return bRuntimeSpawned;
	}
	virtual void OnAfterLoad() override;
	virtual void SetRuntimeSpawned(bool bIsRuntimeSpawned) override;
protected:
	UPROPERTY(EditInstanceOnly, Category = "Save", meta = (AllowPrivateAccess = "true"))
	FGuid InstanceID;
	UPROPERTY(Transient)
	bool bRuntimeSpawned = false;
	UPROPERTY(EditAnywhere, SaveGame, Category = "Destructible")
	float CurrentDurability = 100.0f;
#pragma endregion
protected:
	virtual void BeginPlay() override;
public:
	virtual float ReceiveDamage(const FDamageParams& DamageData) override;
	virtual bool IsDead() const;

	UFUNCTION(BlueprintCallable, Category = "Fire")
	void Ignite(AActor* FireCauser = nullptr);

protected:
	UFUNCTION()
	virtual void BreakObject(const FVector& HitLocation, const FVector& HitDirection);

	UFUNCTION()
	virtual void HideBrokenMesh();

	UFUNCTION(BlueprintCallable, Category = "Fire")
	virtual void StartBurning(AActor* FireCauser = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Fire")
	virtual void StopBurning();

	UFUNCTION()
	virtual void ApplyBurnDamage();
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
	USceneComponent* SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
	UStaticMeshComponent* IntactMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
	UStaticMeshComponent* BrokenMesh;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category = "Destruction")
	float MaxHealth = 10.f;

	UPROPERTY(VisibleAnywhere, SaveGame, BlueprintReadOnly, Category = "Destruction")
	float CurrentHealth = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Destruction")
	float BreakImpulse = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Destruction")
	float BrokenMeshLifeTime = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Destruction")
	bool bDestroyActorAfterBreak = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Destruction")
	bool bHideBrokenMeshAfterDelay = true;

	UPROPERTY(VisibleAnywhere, SaveGame, BlueprintReadOnly, Category = "Destruction")
	bool bDestroyed = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	UNiagaraSystem* BreakEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	USoundBase* BreakSound;

	FTimerHandle HideBrokenMeshTimerHandle;

	// <Burning>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire")
	bool bCanBurn = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire")
	bool bCanIgniteFromDamage = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire")
	float IgnitionDamageThreshold = 0.f;

	UPROPERTY(VisibleAnywhere, SaveGame, BlueprintReadOnly, Category = "Fire")
	bool bBurning = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire")
	float BurnDamage = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire")
	float BurnDamageInterval = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire")
	float BurnDuration = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire")
	UNiagaraSystem* FireEffect;

	UPROPERTY()
	UNiagaraComponent* ActiveFireEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire")
	USoundBase* FireStartSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire")
	USoundBase* FireLoopSound;

	UPROPERTY()
	UAudioComponent* ActiveFireLoopSound;

	UPROPERTY()
	AActor* BurnCauser;

	FTimerHandle BurnDamageTimerHandle;
	FTimerHandle BurnDurationTimerHandle;

	// <Fire Component>
	protected:
		UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
		UFireAreaComponent* FireAreaComponent;

};