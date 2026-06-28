// Fill out your copyright notice in the Description page of Project Settings.


#include "Animations/EnemyAnimInstance.h"
#include "Characters/Enemies/EnemyBase.h"
#include "Characters/Enemies/EnemyStateMachineComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

void UEnemyAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	EnemyCharacter = Cast<AEnemyBase>(TryGetPawnOwner());

	if (EnemyCharacter)
	{
		MovementComponent = EnemyCharacter->GetCharacterMovement();
	}
}

void UEnemyAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!EnemyCharacter)
	{
		EnemyCharacter = Cast<AEnemyBase>(TryGetPawnOwner());
	}

	if (!EnemyCharacter)
	{
		return;
	}

	if (!MovementComponent)
	{
		MovementComponent = EnemyCharacter->GetCharacterMovement();
	}

	const FVector Velocity = EnemyCharacter->GetVelocity();
	const FVector HorizontalVelocity(Velocity.X, Velocity.Y, 0.f);

	GroundSpeed = HorizontalVelocity.Size();

	bIsFalling = MovementComponent ? MovementComponent->IsFalling() : false;
	bIsDead = EnemyCharacter->IsDead();

	if (UEnemyStateMachineComponent* StateMachine = EnemyCharacter->GetStateMachineComponent())
	{
		EnemyState = StateMachine->GetCurrentState();
	}
}

void UEnemyAnimInstance::PlayAttackMontage(UAnimMontage* AttackMontage)
{
	if (!AttackMontage) return;
	if (Montage_IsPlaying(AttackMontage)) return;

	Montage_Play(AttackMontage);
}

void UEnemyAnimInstance::PlayHitReactMontage(UAnimMontage* HitReactMontage)
{
	if (!HitReactMontage) return;

	Montage_Play(HitReactMontage);
}

void UEnemyAnimInstance::PlayDeathMontage(UAnimMontage* DeathMontage)
{
	if (!DeathMontage) return;
	if (bIsDead && Montage_IsPlaying(DeathMontage)) return;

	Montage_Play(DeathMontage);
}
