#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Engine/DataTable.h"
#include "CharacterComponentSettingsSource.generated.h"

struct FCharacterComponentSettingsRow;
class AActor;

/** This interface is deliberately independent of ACharacter/APawn.
 * Implemented by both AEnemyBase : ACharacter and ACharacterPlayer : APawn.
 * C++-only: no Blueprint interface dispatch or cast to a particular character.
 */
UINTERFACE(meta = (CannotImplementInterfaceInBlueprint))
class FPS_BORANAGAJIN_API UCharacterComponentSettingsSource : public UInterface
{
    GENERATED_BODY()
};

class FPS_BORANAGAJIN_API ICharacterComponentSettingsSource
{
    GENERATED_BODY()
public:
    /** Owner selects the shared DataTable AND its row. May be empty. */
    virtual FDataTableRowHandle GetCharacterComponentSettingsRow() const = 0;
};

/** Shared C++ lookup helper used by all three components. */
namespace CharacterComponentSettings
{
    FPS_BORANAGAJIN_API const FCharacterComponentSettingsRow* Resolve(
        AActor* OwnerActor, const FString& ContextString);
}
