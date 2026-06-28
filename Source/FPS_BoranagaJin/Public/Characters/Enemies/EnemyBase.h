#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Characters/DamageInterface.h"
#include "EnemyBase.generated.h"

class ACharacterPlayer;
class UEnemyStateMachineComponent;

UCLASS()
class FPS_BORANAGAJIN_API AEnemyBase : public ACharacter, public IDamageInterface
{
	GENERATED_BODY()

public:
	AEnemyBase();
protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|AI")
	UEnemyStateMachineComponent* StateMachineComponent;

public:
	UEnemyStateMachineComponent* GetStateMachineComponent() const { return StateMachineComponent; };

public:

	//TODO: Damage interface로 옮겨야 할 듯?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Combat")
	float MaxHealth = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Combat")
	float CurrentHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Combat")
	float AttackRange = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Combat")
	float AttackDamage = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Combat")
	float AttackCooldown = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Combat")
	float AttackRotationSpeed = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Combat")
	UAnimMontage* AttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Detection")
	float LoseTargetDistance = 1500.f;

	void TakeEnemyDamage(float DamageAmount);

	void AttackTarget(AActor* Target);
	void ApplyAttackDamage(AActor* Target);

#pragma region DamageInterface
	virtual float ReceiveDamage(const FDamageParams& DamageInfo) override;
	virtual bool IsDead() const override;
#pragma endregion


#pragma region Death
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Death")
	UAnimMontage* DeathMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Death")
	bool bUseRagdollOnDeath = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Death")
	float DestroyDelayAfterDeath = 5.f;

	UFUNCTION()
	void Die();

	void DisableEnemyCollision();
	void StartRagdoll();
#pragma endregion

};