


#pragma once

#include "Characters/Player/CharacterPlayer.h"
//#include "Enumerations/Enemies/EnemyEnums.h"
#include "EnemyActions.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UEnemyActions : public UInterface
{
	GENERATED_BODY()
};

/**
 *
 */
class FPS_BORANAGAJIN_API IEnemyActions
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual void Attack(ACharacterPlayer* Player) = 0;
	//virtual void SetMovementSpeed(EEnemySpeed Speed) = 0;
	//virtual void JumpWall(const FVector& Destination) = 0;
};

