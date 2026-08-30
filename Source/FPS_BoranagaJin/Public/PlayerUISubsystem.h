#pragma once

#include "CoreMinimal.h"
//#include "Subsystems/GameInstanceSubsystem.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "UI/UIType.h"
#include "SoundSystem/SoundID.h"
#include "PlayerUISubsystem.generated.h"


class UBaseUIWidget;
class UMainMenuWidget;
class UMapSelectMenuWidget;
class USaveFileSlotMenuWidget;
class UPauseMenuWidget;
class AFPSPlayerController;
class ACharacterPlayer;
class UGameAudioSubsystem;

struct FPlayableMapInfo;

USTRUCT()
struct FUIWidgetArray
{
    GENERATED_BODY()
    UPROPERTY()
    TArray<TObjectPtr<UBaseUIWidget>> Widgets;
};

UCLASS()
class FPS_BORANAGAJIN_API UPlayerUISubsystem : public ULocalPlayerSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
public:
    void RegisterUIWidget(UBaseUIWidget* NewWidget);

    void ShowUI(EUIType UIType);
    void HideUI(EUIType UIType);

    UBaseUIWidget* GetUIWidget(EUIType UIType) const;

    void SetControlledCharacter(ACharacterPlayer* NewCharacter);

private:
    AFPSPlayerController* GetFPSPlayerController() const;
    APlayerController* GetCustomPlayerController() const;
    UGameAudioSubsystem* GetAudioSubsystem() const;

    FORCEINLINE int32 GetUIZOrder(EUIType Type)
    {
        switch (Type)
        {
        case EUIType::Stamina: return 0;
        case EUIType::Health: return 0;
        case EUIType::Interaction: return 1;
        case EUIType::WeaponAim: return 2;
        case EUIType::AmmoCounter: return 2;
        case EUIType::MainMenu: return 3;
        case EUIType::MapSelectMenu: return 3;
        case EUIType::SaveFileSlotMenu: return 3;
        case EUIType::PauseMenu: return 10;
        case EUIType::Inventory: return 3;
        case EUIType::ThrowableWeaponInventory: return 3;
        case EUIType::Base: return 0;
        default: return 0;
        }
    }

    void PlayUISound(ESoundID SoundID);

    void BindCharacterDelegates();
    void UnbindCharacterDelegates();

    void SetUIOnlyInput(UBaseUIWidget* FocusWidget);
    void SetGameOnlyInput();
public:
    void InitMainMenuUI(TSubclassOf<UMainMenuWidget> WidgetClass);
    void InitMapSelectUI(TSubclassOf<UMapSelectMenuWidget> WidgetClass);
    void InitSaveFileSlotUI(TSubclassOf<USaveFileSlotMenuWidget> WidgetClass);
    void InitGameplayUI();

    // <PauseMenu>
public:
    void InitPauseMenuUI(TSubclassOf<UPauseMenuWidget> WidgetClass);
    void TogglePauseMenu();
    void OpenPauseMenu();
    void ClosePauseMenu();
    bool IsPauseMenuOpened() const;
private:
    void HandlePauseMenuPlayRequested();
    void HandlePauseMenuOptionRequested();
    void HandlePauseMenuSaveAndExitRequested();
private:
    // <MainMenu>
    void HandlePlayRequested();
    void HandleContinueRequested();
    void HandleOptionRequested();
    void HandleExitRequested();
    // <MapSelect>
private:
    void HandlePlayableMapSelected(FPlayableMapInfo MapInfo);
    void HandleMapSelectBackRequested();
    // <SaveSlotMenu>
    //void HandleSaveFileSlotSelected(int32 SlotIndex);
    void HandleSaveFileSlotSelected(FString SlotName);
    void HandleSaveFileSlotBackRequested();
private:
    UPROPERTY()
    TMap<EUIType, FUIWidgetArray> UIWidgets;

    UPROPERTY()
    TObjectPtr<ACharacterPlayer> CharacterPlayer;
};