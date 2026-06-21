// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/Enemies/EnemyMonsterA.h"
//#include "Structures/Enemies/EnemyAttributesData.h"
#include "Kismet/KismetMathLibrary.h"

AEnemyMonsterA::AEnemyMonsterA()
{
	EnemyType = "Melee";
}

void AEnemyMonsterA::BeginPlay()
{
	Super::BeginPlay();
}

void AEnemyMonsterA::RotateTowardPlayer()
{
	float NewRotationYaw = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), Player->GetActorLocation()).Yaw;

	SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(GetActorRotation().Pitch, NewRotationYaw, GetActorRotation().Roll), GetWorld()->GetDeltaSeconds(), 15.f));
}

void AEnemyMonsterA::Attack(ACharacterPlayer* AttackTarget)
{
	Player = AttackTarget;

	// Rotate to face the player for attacks
	GetWorldTimerManager().SetTimer(
		RotationHandle,
		this, &AEnemyMonsterA::RotateTowardPlayer,
		0.01f,
		true
	);

	/*if (!AttackAnimations.IsEmpty())
	{
		UAnimInstance* const EnemyAnimInstance = GetMesh()->GetAnimInstance();
		EnemyAnimInstance->Montage_Play(GetRandomAnimationMontage(AttackAnimations));
	}*/

	// To stop reset rotation timer
	FTimerHandle ClearTimerHandle;

	GetWorldTimerManager().SetTimer(
		ClearTimerHandle,
		FTimerDelegate::CreateWeakLambda(this, [this]() { GetWorldTimerManager().ClearTimer(RotationHandle); }),
		1.f,
		false
	);
}

