#include "PlayerUISubsystem.h"
#include "GameFlowSubsystem.h"

#include "UI/MainMenuWidget.h"
#include "UI/MapSelectMenuWidget.h"
#include "UI/SaveFileSlotMenuWidget.h"
#include "UI/PauseMenuWidget.h"

#include "Instance/DefaultGameInstance.h"
#include "Characters/Player/FPSPlayerController.h"
#include "Characters/Player/CharacterPlayer.h"
#include "SaveSystem/SaveGameSubsystem.h"
//#include "SaveSystem/SaveGameCustom.h"

#include "SoundSystem/GameAudioSubsystem.h"

#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

void UPlayerUISubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UPlayerUISubsystem::Deinitialize()
{
    UnbindCharacterDelegates();

    for (TPair<EUIType, FUIWidgetArray>& Pair : UIWidgets)
    {
        for (UBaseUIWidget* Widget : Pair.Value.Widgets)
        {
            if (IsValid(Widget))
            {
                Widget->RemoveFromParent();
            }
        }
        Pair.Value.Widgets.Reset();
    }

    UIWidgets.Reset();
    CharacterPlayer = nullptr;

    Super::Deinitialize();
}

void UPlayerUISubsystem::RegisterUIWidget(UBaseUIWidget* NewWidget)
{
    if (!IsValid(NewWidget)) { return; }

    const EUIType UIType = NewWidget->GetUIType();
    const int32 Layer = GetUIZOrder(UIType);

    if (!Layer)
    {
        UE_LOG(LogTemp, Error, TEXT("UI layer is missing: %s"), *UEnum::GetValueAsString(UIType));
        return;
    }

    FUIWidgetArray& WidgetArray = UIWidgets.FindOrAdd(UIType);

    WidgetArray.Widgets.Add(NewWidget);

    if (!NewWidget->IsInViewport())
    {
        NewWidget->AddToPlayerScreen(Layer);
    }
}

void UPlayerUISubsystem::ShowUI(EUIType UIType)
{
    FUIWidgetArray* WidgetArray = UIWidgets.Find(UIType);

    if (!WidgetArray) { return; }

    for (UBaseUIWidget* Widget : WidgetArray->Widgets)
    {
        if (IsValid(Widget))
        {
            Widget->SetVisibility(ESlateVisibility::Visible);
        }
    }
}

void UPlayerUISubsystem::HideUI(EUIType UIType)
{
    FUIWidgetArray* WidgetArray = UIWidgets.Find(UIType);

    if (!WidgetArray) { return; }

    for (UBaseUIWidget* Widget : WidgetArray->Widgets)
    {
        if (IsValid(Widget))
        {
            Widget->SetVisibility(ESlateVisibility::Collapsed);
            //Widget->SetVisibility(ESlateVisibility::Hidden);
        }
    }
}

UBaseUIWidget* UPlayerUISubsystem::GetUIWidget(EUIType UIType) const
{
    const FUIWidgetArray* WidgetArray = UIWidgets.Find(UIType);

    if (!WidgetArray) { return nullptr; }

    for (UBaseUIWidget* Widget : WidgetArray->Widgets)
    {
        if (IsValid(Widget))
        {
            return Widget;
        }
    }

    return nullptr;
}

void UPlayerUISubsystem::SetControlledCharacter(ACharacterPlayer* NewCharacter)
{
    if (CharacterPlayer == NewCharacter) { return; }

    UnbindCharacterDelegates();

    CharacterPlayer = NewCharacter;

    BindCharacterDelegates();
}

AFPSPlayerController* UPlayerUISubsystem::GetFPSPlayerController() const
{
    const ULocalPlayer* LocalPlayer = GetLocalPlayer();

    if (!LocalPlayer)
    {
        return nullptr;
    }

    return Cast<AFPSPlayerController>(LocalPlayer->GetPlayerController(GetWorld()));
}

APlayerController* UPlayerUISubsystem::GetCustomPlayerController() const
{
    const ULocalPlayer* LocalPlayer = GetLocalPlayer();
    if (!LocalPlayer) { return nullptr; }
    return Cast<APlayerController>(LocalPlayer->GetPlayerController(GetWorld()));
}

UGameAudioSubsystem* UPlayerUISubsystem::GetAudioSubsystem() const
{
    ULocalPlayer* LocalPlayer = GetLocalPlayer();
    if (!IsValid(LocalPlayer)) { return nullptr; }

    UGameInstance* GameInstance = LocalPlayer->GetGameInstance();

    if (!IsValid(GameInstance))
    {
        return nullptr;
    }

    return GameInstance->GetSubsystem<UGameAudioSubsystem>();
}

void UPlayerUISubsystem::PlayUISound(ESoundID SoundID)
{
    UGameAudioSubsystem* AudioSubsystem = GetAudioSubsystem();
    if (!IsValid(AudioSubsystem)) { return; }
    AudioSubsystem->PlaySound2D(SoundID);
}

void UPlayerUISubsystem::BindCharacterDelegates()
{
    if (!IsValid(CharacterPlayer)) { return; }

    // Gameplay UI Delegate 연결
}

void UPlayerUISubsystem::UnbindCharacterDelegates()
{
    if (!IsValid(CharacterPlayer)) { return; }

    // Gameplay UI Delegate 해제
}

void UPlayerUISubsystem::SetUIOnlyInput(UBaseUIWidget* FocusWidget)
{
    APlayerController* PlayerController = GetCustomPlayerController();
    if (!IsValid(PlayerController)) { return; }
    PlayerController->SetShowMouseCursor(true);
    FInputModeUIOnly InputMode;
    if (IsValid(FocusWidget))
    {
        InputMode.SetWidgetToFocus(FocusWidget->TakeWidget());
    }
    PlayerController->SetInputMode(InputMode);
}

void UPlayerUISubsystem::SetGameOnlyInput()
{
    APlayerController* PlayerController = GetCustomPlayerController();

    if (!IsValid(PlayerController)) { return; }

    PlayerController->SetShowMouseCursor(false);

    FInputModeGameOnly InputMode;
    PlayerController->SetInputMode(InputMode);
}

void UPlayerUISubsystem::InitMainMenuUI(TSubclassOf<UMainMenuWidget> WidgetClass)
{
    if (!WidgetClass) { return; }

    APlayerController* PlayerController = GetCustomPlayerController();

    if (!PlayerController) { return; }

    UMainMenuWidget* MainMenuWidget = CreateWidget<UMainMenuWidget>(PlayerController, WidgetClass);

    if (!MainMenuWidget) { return; }

    RegisterUIWidget(MainMenuWidget);

    MainMenuWidget->OnPlayNewGameRequested.AddUObject(this, &UPlayerUISubsystem::HandlePlayRequested);
    MainMenuWidget->OnContinueRequested.AddUObject(this, &UPlayerUISubsystem::HandleContinueRequested);
    MainMenuWidget->OnOptionRequested.AddUObject(this, &UPlayerUISubsystem::HandleOptionRequested);
    MainMenuWidget->OnExitRequested.AddUObject(this, &UPlayerUISubsystem::HandleExitRequested);

    ShowUI(EUIType::MainMenu);
    SetUIOnlyInput(MainMenuWidget);
}

void UPlayerUISubsystem::InitMapSelectUI(TSubclassOf<UMapSelectMenuWidget> WidgetClass)
{
    if (!WidgetClass) { return; }
    APlayerController* PlayerController = GetCustomPlayerController();
    if (!IsValid(PlayerController)) { return; }
    UMapSelectMenuWidget* MapSelectMenuWidget = CreateWidget<UMapSelectMenuWidget>(PlayerController, WidgetClass);
    if (!IsValid(MapSelectMenuWidget)) { return; }
    RegisterUIWidget(MapSelectMenuWidget);
    MapSelectMenuWidget->OnPlayableMapSelected.AddUObject(this, &UPlayerUISubsystem::HandlePlayableMapSelected);
    MapSelectMenuWidget->OnBackRequested.AddUObject(this, &UPlayerUISubsystem::HandleMapSelectBackRequested);
    HideUI(EUIType::MapSelectMenu);
}

void UPlayerUISubsystem::InitSaveFileSlotUI(TSubclassOf<USaveFileSlotMenuWidget> WidgetClass)
{
    if (!WidgetClass) { return; }
    APlayerController* PlayerController = GetCustomPlayerController();
    if (!IsValid(PlayerController)) { return; }
    USaveFileSlotMenuWidget* SaveFileSlotMenuWidget = CreateWidget<USaveFileSlotMenuWidget>(PlayerController, WidgetClass);
    if (!IsValid(SaveFileSlotMenuWidget)) { return; }
    RegisterUIWidget(SaveFileSlotMenuWidget);
    SaveFileSlotMenuWidget->OnSaveFileSlotSelected.AddUObject(this, &UPlayerUISubsystem::HandleSaveFileSlotSelected);
    SaveFileSlotMenuWidget->OnBackRequested.AddUObject(this, &UPlayerUISubsystem::HandleSaveFileSlotBackRequested);
    HideUI(EUIType::SaveFileSlotMenu);
}

void UPlayerUISubsystem::InitGameplayUI()
{
    SetGameOnlyInput();

    HideUI(EUIType::MainMenu);
    HideUI(EUIType::SaveFileSlotMenu);

    // Health, Stamina, AmmoCounter 등의 Gameplay UI 초기화
}

void UPlayerUISubsystem::InitPauseMenuUI(TSubclassOf<UPauseMenuWidget> WidgetClass)
{
    if (!WidgetClass) { return; }

    APlayerController* PlayerController = GetCustomPlayerController();
    if (!IsValid(PlayerController)) { return; }

    UPauseMenuWidget* PauseMenuWidget = CreateWidget<UPauseMenuWidget>(PlayerController, WidgetClass);
    if (!IsValid(PauseMenuWidget)) { return; }

    RegisterUIWidget(PauseMenuWidget);

    PauseMenuWidget->OnPlayRequested.AddUObject(this, &UPlayerUISubsystem::HandlePauseMenuPlayRequested);
    PauseMenuWidget->OnOptionRequested.AddUObject(this, &UPlayerUISubsystem::HandlePauseMenuOptionRequested);
    PauseMenuWidget->OnSaveAndExitRequested.AddUObject(this, &UPlayerUISubsystem::HandlePauseMenuSaveAndExitRequested);

    HideUI(EUIType::PauseMenu);

    UE_LOG(LogTemp, Error, TEXT("void UPlayerUISubsystem::InitPauseMenuUI(TSubclassOf<UPauseMenuWidget> WidgetClass)"));
}

bool UPlayerUISubsystem::IsPauseMenuOpened() const
{
    const UBaseUIWidget* PauseMenuWidget = GetUIWidget(EUIType::PauseMenu);

    if (!IsValid(PauseMenuWidget)) 
    { 
        return false; 
        UE_LOG(LogTemp, Error, TEXT("UPlayerUISubsystem::IsPauseMenuOpened(): PauseMenuWidget is invalid"));
    }

    return PauseMenuWidget->GetVisibility() != ESlateVisibility::Collapsed;
}

void UPlayerUISubsystem::TogglePauseMenu()
{
    UE_LOG(LogTemp, Error, TEXT("void UPlayerUISubsystem::TogglePauseMenu()"));

    if (IsPauseMenuOpened())
    {
        ClosePauseMenu();
    }
    else
    {
        OpenPauseMenu();
    }
}

void UPlayerUISubsystem::OpenPauseMenu()
{
    UE_LOG(LogTemp, Error, TEXT("void UPlayerUISubsystem::OpenPauseMenu()"));

    UPauseMenuWidget* PauseMenuWidget = Cast<UPauseMenuWidget>(GetUIWidget(EUIType::PauseMenu));

    if (!IsValid(PauseMenuWidget)) { return; }

    APlayerController* PlayerController = GetCustomPlayerController();

    if (!IsValid(PlayerController)) { return; }

    //TODO: GamePause는 안하는 방식이 좋을 듯함
    //UGameplayStatics::SetGamePaused(this, true);

    ShowUI(EUIType::PauseMenu);
    PlayUISound(ESoundID::UI_Open);

    PlayerController->SetShowMouseCursor(true);

    FInputModeGameAndUI InputMode;
    //FInputModeUIOnly InputMode;
    InputMode.SetWidgetToFocus(PauseMenuWidget->TakeWidget());
    InputMode.SetHideCursorDuringCapture(false);

    PlayerController->SetInputMode(InputMode);


    //FInputModeUIOnly InputMode;
    //if (IsValid(FocusWidget))
    //{
    //    InputMode.SetWidgetToFocus(FocusWidget->TakeWidget());
    //}
    //PlayerController->SetInputMode(InputMode);
}

void UPlayerUISubsystem::ClosePauseMenu()
{
    UE_LOG(LogTemp, Error, TEXT("void UPlayerUISubsystem::ClosePauseMenu()"));

    APlayerController* PlayerController = GetCustomPlayerController();
    if (!IsValid(PlayerController)) { return; }
    HideUI(EUIType::PauseMenu);
    PlayUISound(ESoundID::UI_Close);

    //TODO: 일시정지에 대한 고려 필요
    //UGameplayStatics::SetGamePaused(this, false);

    PlayerController->SetShowMouseCursor(false);

    //TODO: 근데 Inventory를 킨 상태에서 esc를 누르면 어떻게 동작되어야 하는지 정의가 필요함
    FInputModeGameOnly InputMode;
    PlayerController->SetInputMode(InputMode);
}

void UPlayerUISubsystem::HandlePauseMenuPlayRequested()
{
    ClosePauseMenu();
}

void UPlayerUISubsystem::HandlePauseMenuOptionRequested()
{
    // TODO: Option UI

    PlayUISound(ESoundID::UI_Open);
}

void UPlayerUISubsystem::HandlePauseMenuSaveAndExitRequested()
{
    ULocalPlayer* LocalPlayer = GetLocalPlayer();

    if (!IsValid(LocalPlayer)) { return; }

    UGameInstance* GameInstance = LocalPlayer->GetGameInstance();

    if (!IsValid(GameInstance)) { return; }

    UGameFlowSubsystem* GameFlowSubsystem = GameInstance->GetSubsystem<UGameFlowSubsystem>();

    if (!IsValid(GameFlowSubsystem)) { return; }

    GameFlowSubsystem->SaveAndQuitGame();

    UE_LOG(LogTemp, Error, TEXT("void UPlayerUISubsystem::HandlePauseMenuSaveAndExitRequested()"));
}

void UPlayerUISubsystem::HandlePlayRequested()
{
    //// <Old Version>
    //ULocalPlayer* LocalPlayer = GetLocalPlayer();
    //if (!IsValid(LocalPlayer)) { return; }
    //UGameInstance* GameInstance = LocalPlayer->GetGameInstance();
    //if (!IsValid(GameInstance)) { return; }
    //UGameFlowSubsystem* GameFlowSubsystem = GameInstance->GetSubsystem<UGameFlowSubsystem>();
    //if (!IsValid(GameFlowSubsystem)) { return; }
    //GameFlowSubsystem->StartNewGame();

    //-----------------------------------------------------------------------
    // <New Version>
    ULocalPlayer* LocalPlayer = GetLocalPlayer();
    if (!IsValid(LocalPlayer)) { return; }
    UGameInstance* GameInstance = LocalPlayer->GetGameInstance();
    if (!IsValid(GameInstance)) { return; }
    const UCustomGameInstance* CustomGameInstance = Cast<UCustomGameInstance>(GameInstance);
    if (!CustomGameInstance) { return; }

    UMapSelectMenuWidget* MapSelectMenuWidget = Cast<UMapSelectMenuWidget>(GetUIWidget(EUIType::MapSelectMenu));
    if (!IsValid(MapSelectMenuWidget)) { return; }

    MapSelectMenuWidget->RefreshPlayableMaps(CustomGameInstance->GetPlayableMaps());
    HideUI(EUIType::MainMenu);
    ShowUI(EUIType::MapSelectMenu);
    PlayUISound(ESoundID::UI_Click);
    SetUIOnlyInput(MapSelectMenuWidget);
}

void UPlayerUISubsystem::HandlePlayableMapSelected(FPlayableMapInfo MapInfo)
{
    if (!MapInfo.IsValid()) { return; }

    ULocalPlayer* LocalPlayer = GetLocalPlayer();
    if (!IsValid(LocalPlayer)) { return; }
    UGameInstance* GameInstance = LocalPlayer->GetGameInstance();
    if (!IsValid(GameInstance)) { return; }

    UGameFlowSubsystem* GameFlowSubsystem = GameInstance->GetSubsystem<UGameFlowSubsystem>();

    if (!IsValid(GameFlowSubsystem)) { return; }

    PlayUISound(ESoundID::UI_Click);

    GameFlowSubsystem->StartNewGame(MapInfo.LevelAsset);
}

void UPlayerUISubsystem::HandleMapSelectBackRequested()
{
    UBaseUIWidget* MainMenuWidget = GetUIWidget(EUIType::MainMenu);

    if (!IsValid(MainMenuWidget)) { return; }

    HideUI(EUIType::MapSelectMenu);
    ShowUI(EUIType::MainMenu);

    PlayUISound(ESoundID::UI_Click);

    SetUIOnlyInput(MainMenuWidget);
}

void UPlayerUISubsystem::HandleContinueRequested()
{
    //// <Old Version>

    //ULocalPlayer* LocalPlayer = GetLocalPlayer();
    //if (!IsValid(LocalPlayer)) { return; }
    //UGameInstance* GameInstance = LocalPlayer->GetGameInstance();
    //if (!IsValid(GameInstance)) { return; }

    //UGameFlowSubsystem* GameFlowSubsystem = GameInstance->GetSubsystem<UGameFlowSubsystem>();
    ////USaveGameSubsystem* SaveSubsystem = GameInstance->GetSubsystem<USaveGameSubsystem>();

    //if (!GameFlowSubsystem) { return; }

    ////SaveSubsystem->StartNewGame(true);
    //GameFlowSubsystem->RestartFromCheckPoint();

    //--------------------------------------------------------
    // <New Version>
    ULocalPlayer* LocalPlayer = GetLocalPlayer();
    if (!IsValid(LocalPlayer)) { return; }

    UGameInstance* GameInstance = LocalPlayer->GetGameInstance();
    if (!IsValid(GameInstance)) { return; }

    USaveGameSubsystem* SaveSubsystem = GameInstance->GetSubsystem<USaveGameSubsystem>();
    if (!IsValid(SaveSubsystem)) { return; }

    USaveFileSlotMenuWidget* SaveFileSlotMenuWidget = Cast<USaveFileSlotMenuWidget>(GetUIWidget(EUIType::SaveFileSlotMenu));
    if (!IsValid(SaveFileSlotMenuWidget)) { return; }

    SaveFileSlotMenuWidget->RefreshSaveSlots(SaveSubsystem->GetSaveSlots());

    HideUI(EUIType::MainMenu);
    ShowUI(EUIType::SaveFileSlotMenu);
    PlayUISound(ESoundID::UI_Click);

    SetUIOnlyInput(SaveFileSlotMenuWidget);
}

void UPlayerUISubsystem::HandleOptionRequested()
{
    PlayUISound(ESoundID::UI_Click);
}

void UPlayerUISubsystem::HandleExitRequested()
{
    ULocalPlayer* LocalPlayer = GetLocalPlayer();
    if (!IsValid(LocalPlayer)) { return; }
    UGameInstance* GameInstance = LocalPlayer->GetGameInstance();
    if (!IsValid(GameInstance)) { return; }
    UGameFlowSubsystem* GameFlowSubsystem = GameInstance->GetSubsystem<UGameFlowSubsystem>();
    if (!IsValid(GameFlowSubsystem)) { return; }
    GameFlowSubsystem->QuitGame();
}

//void UPlayerUISubsystem::HandleSaveFileSlotSelected(int32 SlotIndex)
//{
//    ULocalPlayer* LocalPlayer = GetLocalPlayer();
//    if (!IsValid(LocalPlayer)) { return; }
//    UGameInstance* GameInstance = LocalPlayer->GetGameInstance();
//    if (!IsValid(GameInstance)) { return; }
//    UGameFlowSubsystem* GameFlowSubsystem = GameInstance->GetSubsystem<UGameFlowSubsystem>();
//    if (!IsValid(GameFlowSubsystem)) { return; }
//
//    GameFlowSubsystem->LoadGameFromSlot(SlotIndex);
//}

void UPlayerUISubsystem::HandleSaveFileSlotSelected(FString SlotName)
{
    ULocalPlayer* LocalPlayer = GetLocalPlayer();
    if (!IsValid(LocalPlayer)) { return; }
    UGameInstance* GameInstance = LocalPlayer->GetGameInstance();
    if (!IsValid(GameInstance)) { return; }
    UGameFlowSubsystem* GameFlowSubsystem = GameInstance->GetSubsystem<UGameFlowSubsystem>();
    if (!IsValid(GameFlowSubsystem)) { return; }

    PlayUISound(ESoundID::UI_Click);

    GameFlowSubsystem->LoadGameFromSlot(SlotName);
}

void UPlayerUISubsystem::HandleSaveFileSlotBackRequested()
{
    UBaseUIWidget* MainMenuWidget = GetUIWidget(EUIType::MainMenu);
    if (!IsValid(MainMenuWidget))
    {
        UE_LOG(LogTemp, Warning, TEXT("MainMenuWidget is not initialized."));
        return;
    }
    HideUI(EUIType::SaveFileSlotMenu);
    ShowUI(EUIType::MainMenu);
    SetUIOnlyInput(MainMenuWidget);

    PlayUISound(ESoundID::UI_Click);
}