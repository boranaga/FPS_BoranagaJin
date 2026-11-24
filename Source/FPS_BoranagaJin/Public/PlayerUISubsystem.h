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
class UStaminaWidget;
class UPlayerDisplayWidget; //Inventory Widget
class UThrowableWeaponInventoryWidget;
class UQuickSlotWidget;
class UInteractionWidget;
class UExitDoorWidget;
class AExitDoor;
class APlayerController;
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
    void UnregisterUIWidget(UBaseUIWidget* Widget);

    void ShowUI(EUIType UIType);
    void HideUI(EUIType UIType);
    void ShowUI(UBaseUIWidget* UIPtr);
    void HideUI(UBaseUIWidget* UIPtr);

    UBaseUIWidget* GetUIWidget(EUIType UIType) const;

    void SetControlledCharacter(ACharacterPlayer* NewCharacter);

    /** True whenever a modal UI must suppress world interaction and pickup prompts. */
    bool IsWorldInteractionInputBlocked() const;

    // Exit-door UI is shared per local player, not instantiated by each door.
    void RegisterNearbyExitDoor(AExitDoor* Door);
    void UnregisterNearbyExitDoor(AExitDoor* Door);
    /** The interaction key is the only way to show the puzzle panel. */
    bool OpenExitDoorUI(AExitDoor* Door, ACharacterPlayer* InteractingPlayer);
    void CloseExitDoorUI();
    bool IsExitDoorUIOpen() const { return bExitDoorPanelOpen; }
    // The active exit may update the shared widget returned by GetUIWidget(EUIType::ExitDoor).
    bool IsActiveExitDoor(const AExitDoor* Door) const;
    void SetExitDoorCursorWanted(const AExitDoor* Door, bool bWantCursor);

private:
    AFPSPlayerController* GetFPSPlayerController() const;
    APlayerController* GetCustomPlayerController() const;
    UGameAudioSubsystem* GetAudioSubsystem() const;

    FORCEINLINE int32 GetUIZOrder(EUIType Type)
    {
        switch (Type)
        {
        case EUIType::Stamina: return 1;
        case EUIType::Health: return 1;
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
        case EUIType::QuickSlot: return 2;
        case EUIType::ExitDoor: return 5;
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
    void Init_BeginPlay();
    void InitMainMenuUI(TSubclassOf<UMainMenuWidget> WidgetClass);
    void InitMapSelectUI(TSubclassOf<UMapSelectMenuWidget> WidgetClass);
    void InitSaveFileSlotUI(TSubclassOf<USaveFileSlotMenuWidget> WidgetClass);
    void InitPauseMenuUI(TSubclassOf<UPauseMenuWidget> WidgetClass);
    void InitGameOverUI(TSubclassOf<UGameOverWidget> WidgetClass);
    void InitHealthBarUI(TSubclassOf<UHealthWidget> WidgetClass);
    void InitStaminaUI(TSubclassOf<UStaminaWidget> WidgetClass);

    void InitInteractionWidget(TSubclassOf<UInteractionWidget> WidgetClass);
    void InitInventoryUI(TSubclassOf<UPlayerDisplayWidget> WidgetClass);
    void InitThrowableWeaponInventoryUI(TSubclassOf<UThrowableWeaponInventoryWidget> WidgetClass);
    void InitQuickSlotUI(TSubclassOf<UQuickSlotWidget> WidgetClass);

    //void InitGameplayUI(TSubclassOf<UBaseUIWidget> WidgetClass); //TODO: ����

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


    // <HealthBar>


    // <StaminaBar>
public:
    void UpdateStaminaBar(float maxstamina, float currstamina);

    // <Inventory>
public:
    void OpenInventory();
    void CloseInventory();
    void RequestSwapInventorySlots(FName InventoryName, int32 FromIndex, int32 ToIndex);
    void RequestDropInventorySlot(FName InventoryName, int32 SlotIndex);
    void RequestUseInventorySlot(FName InventoryName, int32 SlotIndex);

    // <ThrowableWeaponInventory>
private:
    void HandleThrowableWeaponEquipRequested(int32 SlotIndex);
public:
    void OpenThrowableWeaponInventory();
    void CloseThrowableWeaponInventory();

    // <Interaction>
public:
    void PlayPopUpInteractionWidgetAnim();
    void UpdateInteractionUI(bool bFlag = false, FVector NewLocation = FVector::ZeroVector);


protected:
    TTuple<FVector2D, bool> GetScreenPositionOfWorldLocation(const FVector& SearchLocation) const;
    bool IsInViewport(FVector2D ActorScreenPosition, float ScreenRatio_Width = 0.0f, float ScreenRatio_Height = 0.0f) const;


private:
    bool EnsureExitDoorWidget(TSubclassOf<UExitDoorWidget> WidgetClass);
    void ReleaseExitDoorWidget();
    void ReleaseExitDoorCursor();
    bool IsOtherModalUIVisible() const;
    void HandleExitDoorSlotClicked(int32 SlotIndex);

    // Weak actor references: level actors must not be kept alive by a local-player subsystem.
    TArray<TWeakObjectPtr<AExitDoor>> NearbyExitDoors;
    TWeakObjectPtr<AExitDoor> ActiveExitDoor;
    TWeakObjectPtr<APlayerController> ExitDoorCursorController;
    bool bExitDoorOwnsCursor = false;
    bool bExitDoorPanelOpen = false;
    // Prevent Escape from closing the panel and opening PauseMenu in the same frame.
    uint64 LastExitDoorClosedFrame = MAX_uint64;

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

    UE_LOG(LogTemp, Verbose, TEXT("PlayerUISubsystem: registered widget %s"), *GetNameSafe(NewWidget));

    return NewWidget;
}