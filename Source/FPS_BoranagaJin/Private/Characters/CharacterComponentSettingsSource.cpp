#include "Characters/CharacterComponentSettingsSource.h"
#include "Characters/CharacterComponentSettings.h"
#include "GameFramework/Actor.h"

const FCharacterComponentSettingsRow* CharacterComponentSettings::Resolve(
    AActor* OwnerActor, const FString& ContextString)
{
    if (!IsValid(OwnerActor))
    {
        return nullptr;
    }

    const ICharacterComponentSettingsSource* Provider =
        Cast<ICharacterComponentSettingsSource>(OwnerActor);
    if (!Provider)
    {
        // A generic actor without the interface is valid: use component defaults.
        return nullptr;
    }

    const FDataTableRowHandle Handle = Provider->GetCharacterComponentSettingsRow();
    if (!Handle.DataTable || Handle.RowName.IsNone())
    {
        // No owner DataTable assigned: component/Blueprint defaults remain valid.
        return nullptr;
    }

    const FString Context = FString::Printf(TEXT("%s [%s]"),
        *ContextString, *GetNameSafe(OwnerActor));
    const FCharacterComponentSettingsRow* Row =
        Handle.GetRow<FCharacterComponentSettingsRow>(Context);

    if (!Row)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CharacterComponentSettings] No row '%s' for owner '%s' in '%s'. Keeping current component settings."),
            *Handle.RowName.ToString(), *GetNameSafe(OwnerActor), *GetNameSafe(Handle.DataTable.Get()));
    }
    return Row;
}
