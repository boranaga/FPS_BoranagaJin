#pragma once

#include "CoreMinimal.h"
//#include "Subsystems/GameInstanceSubsystem.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "UI/UIType.h"
#include "SoundSystem/SoundID.h"
#include "GameEndReason.h"
#include "PlayerUISubsystem.generated.h"

class UBaseUIWidget;
class UMainMenuWidget;
class UMapSelectMenuWidget;
class USaveFileSlotMenuWidget;
class UPauseMenuWidget;
class UGameOverWidget;
class UHealthWidget;
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

    template<typename TWidget>
    TWidget* CreateAndRegisterWidget(TSubclassOf<TWidget> WidgetClass);

public:
    void RegisterUIWidget(UBaseUIWidget* NewWidget);

    void ShowUI(EUIType UIType);
    void HideUI(EUIType UIType);
    void ShowUI(UBaseUIWidget* UIPtr);
    void HideUI(UBaseUIWidget* UIPtr);

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
        case EUIType::GameOver: return 9;
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
    void InitPauseMenuUI(TSubclassOf<UPauseMenuWidget> WidgetClass);
    void InitGameOverUI(TSubclassOf<UGameOverWidget> WidgetClass);
    void InitHealthBarUI(TSubclassOf<UHealthWidget> WidgetClass);
    //void InitGameplayUI(TSubclassOf<UBaseUIWidget> WidgetClass); //TODO: ¼³Á¤

    // <PauseMenu>
public:
    void TogglePauseMenu();
    void OpenPauseMenu();
    void ClosePauseMenu();
    bool IsPauseMenuOpened() const;
private:
    void HandlePauseMenuPlayRequested();
    void HandlePauseMenuOptionRequested();
    void HandlePauseMenuSaveAndExitRequested();
    // <GameOver>
public:
    void OpenGameOverUI(EGameEndReason EndReason);
    void CloseGameOverUI();
    bool IsGameOverUIOpened() const;
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
    // <GameEnded>
private:
    void HandleBackToMainMenuRequested();
    //void HandleGameEnded(EGameEndReason EndReason);
private:
    UPROPERTY()
    TMap<EUIType, FUIWidgetArray> UIWidgets;

    UPROPERTY()
    TObjectPtr<ACharacterPlayer> CharacterPlayer;
};


template<typename TWidget>
TWidget* UPlayerUISubsystem::CreateAndRegisterWidget(TSubclassOf<TWidget> WidgetClass)
{
    static_assert(TIsDerivedFrom<TWidget, UBaseUIWidget>::Value, "TWidget must inherit from UBaseUIWidget.");

    if (!WidgetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("InitGameplayUI: WidgetClass is invalid."));
        return nullptr;
    }

    ULocalPlayer* LocalPlayer = GetLocalPlayer();

    if (!IsValid(LocalPlayer))
    {
        UE_LOG(LogTemp, Warning, TEXT("InitGameplayUI: LocalPlayer is invalid."));
        return nullptr;
    }

    APlayerController* PlayerController = LocalPlayer->GetPlayerController(GetWorld());
    if (!IsValid(PlayerController))
    {
        UE_LOG(LogTemp, Warning, TEXT("InitGameplayUI: PlayerController is invalid."));
        return nullptr;
    }

    TWidget* NewWidget = CreateWidget<TWidget>(PlayerController, WidgetClass);

    if (!IsValid(NewWidget))
    {
        UE_LOG(LogTemp, Warning, TEXT("InitGameplayUI: Failed to create widget. Class: %s"), *GetNameSafe(WidgetClass));
        return nullptr;
    }

    RegisterUIWidget(NewWidget);

    return NewWidget;
}