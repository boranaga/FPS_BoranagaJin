#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemyStateType.generated.h"

UENUM(BlueprintType)
enum class EEnemyStateType : uint8
{
	Idle	UMETA(DisplayName = "Idle"),
	Patrol	UMETA(DisplayName = "Patrol"),
	Chase	UMETA(DisplayName = "Chase"),
	Attack	UMETA(DisplayName = "Attack"),
	Investigate UMETA(DisplayName = "Investigate"),
	TrackBlood UMETA(DisplayName = "TrackBlood"),
	Flee UMETA(DisplayName = "Flee"),
	Hide UMETA(DisplayName = "Hide"),
	Recover UMETA(DisplayName = "Recover"),
	Dead	UMETA(DisplayName = "Dead")
};

UCLASS()
class FPS_BORANAGAJIN_API AEnemyStateType : public AActor
{
	GENERATED_BODY()
public:
	AEnemyStateType();
};
