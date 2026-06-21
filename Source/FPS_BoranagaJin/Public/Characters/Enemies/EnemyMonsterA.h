// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/Enemies/EnemyBase.h"
#include "EnemyMonsterA.generated.h"

/**
 * 
 */
UCLASS()
class FPS_BORANAGAJIN_API AEnemyMonsterA : public AEnemyBase
{
	GENERATED_BODY()

	FTimerHandle RotationHandle;

	UPROPERTY()
	const ACharacterPlayer* Player;

	void RotateTowardPlayer();

protected:
	virtual void BeginPlay() override;

public:
	AEnemyMonsterA();

	virtual void Attack(ACharacterPlayer* Player) override;
};
