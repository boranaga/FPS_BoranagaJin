

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DefaultGameMode.generated.h"

class ACharacterPlayer;
/**
 *
 */
UCLASS()
class FPS_BORANAGAJIN_API ADefaultGameMode : public AGameModeBase
{
	GENERATED_BODY()
protected:

	//UPROPERTY(EditDefaultsOnly)
	//float PlayerFellRespawnHealthReduction = 10.f;

	//UPROPERTY(EditDefaultsOnly)
	//float PlayerMinimumRespawnHealth = 20.f;
public:

	virtual void BeginPlay() override;

	//void RespawnToLastCheckpoint(ACharacterPlayer* Player);

	//void OnPlayerFellOutOfWorld(ACharacterPlayer* Player);

	//void TeleportToLastCheckpoint();
};
