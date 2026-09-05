// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnemyAudioEvent.generated.h"

UENUM(BlueprintType)
enum class EEnemyAudioEvent : uint8
{
	None,

	IdleVocal,
	PatrolVocal,

	Investigate,
	Alert,

	ChaseStart,
	ChaseVocal,

	Attack,

	PainLight,
	PainHeavy,

	Flee,
	Hide,
	Recover,

	Death
};