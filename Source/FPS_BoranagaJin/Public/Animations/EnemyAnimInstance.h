// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Characters/Enemies/EnemyStateType.h"
#include "EnemyAnimInstance.generated.h"

class AEnemyBase;
class UCharacterMovementComponent;

UCLASS()
class FPS_BORANAGAJIN_API UEnemyAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

private:
	UPROPERTY()
	TObjectPtr<AEnemyBase> EnemyCharacter;

	UPROPERTY()
	TObjectPtr<UCharacterMovementComponent> MovementComponent;

public:
	UPROPERTY(VisibleAnywhere, Category = "Enemy|Animation")
	float GroundSpeed = 0.f;

	UPROPERTY(VisibleAnywhere, Category = "Enemy|Animation")
	bool bIsFalling = false;

	UPROPERTY(VisibleAnywhere, Category = "Enemy|Animation")
	bool bIsDead = false;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Animation")
	EEnemyStateType EnemyState = EEnemyStateType::Idle;

public:
	void PlayAttackMontage(UAnimMontage* AttackMontage);
	void PlayHitReactMontage(UAnimMontage* HitReactMontage);
	void PlayDeathMontage(UAnimMontage* DeathMontage);
};
