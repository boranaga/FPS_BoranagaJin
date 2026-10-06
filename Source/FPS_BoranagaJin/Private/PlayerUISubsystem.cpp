#include "PlayerUISubsystem.h"
#include "GameFlowSubsystem.h"

#include "UI/MainMenuWidget.h"
#include "UI/MapSelectMenuWidget.h"
#include "UI/SaveFileSlotMenuWidget.h"
#include "UI/PauseMenuWidget.h"
#include "UI/GameOverWidget.h"
#include "UI/HealthWidget.h"
#include "UI/StaminaWidget.h"
#include "UI/InventoryUIWidget.h"
#include "UI/PlayerDisplayWidget.h"
#include "UI/ThrowableWeaponInventoryWidget.h"
#include "UI/InteractionWidget.h"

#include "Instance/DefaultGameInstance.h"
#include "Characters/Player/FPSPlayerController.h"
#include "Characters/Player/CharacterPlayer.h"
#include "Characters/HealthComponent.h"
#include "SaveSystem/SaveGameSubsystem.h"
//#include "SaveSystem/SaveGameCustom.h"

#include "SoundSystem/GameAudioSubsystem.h"

#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

void UPlayerUISubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    UE_LOG(LogTemp, Error, TEXT("void UPlayerUISubsystem::Initialize(FSubsystemCollectionBase& Collection)"));
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

void UPlayerUISubsystem::ShowUI(UBaseUIWidget* UIPtr)
{
    if (IsValid(UIPtr))
    {
        UIPtr->SetVisibility(ESlateVisibility::Visible);
    }
}

void UPlayerUISubsystem::HideUI(UBaseUIWidget* UIPtr)
{
    if (IsValid(UIPtr))
    {
        UIPtr->SetVisibility(ESlateVisibility::Collapsed);
        //Widget->SetVisibility(ESlateVisibility::Hidden);
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

void UPlayerUISubsystem::Init_BeginPlay()
{
    
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

//void UPlayerUISubsystem::InitGameplayUI()
//{
//    SetGameOnlyInput();
//
//    HideUI(EUIType::MainMenu);
//    HideUI(EUIType::SaveFileSlotMenu);
//
//    // Health, Stamina, AmmoCounter 등의 Gameplay UI 초기화
//}

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

void UPlayerUISubsystem::InitGameOverUI(TSubclassOf<UGameOverWidget> WidgetClass)
{
    if (!WidgetClass) { return; }
    APlayerController* PlayerController = GetCustomPlayerController();
    if (!IsValid(PlayerController)) { return; }
    UGameOverWidget* GameOverWidget = CreateWidget<UGameOverWidget>(PlayerController, WidgetClass);
    if (!IsValid(GameOverWidget)) { return; }
    RegisterUIWidget(GameOverWidget);

    GameOverWidget->OnMainMenuRequested.AddUObject(this, &UPlayerUISubsystem::HandleBackToMainMenuRequested);
    //TODO: 현재 게임 slot에 대한 처리를 완료하고 게임을 종료해야함.
    GameOverWidget->OnExitRequested.AddUObject(this, &UPlayerUISubsystem::HandleExitRequested);

    HideUI(EUIType::GameOver);
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

void UPlayerUISubsystem::InitHealthBarUI(TSubclassOf<UHealthWidget> WidgetClass)
{
    if (!WidgetClass) { return; }
    APlayerController* PlayerController = GetCustomPlayerController();
    if (!IsValid(PlayerController)) { return; }
    UHealthWidget* HealthWidget = CreateWidget<UHealthWidget>(PlayerController, WidgetClass);
    if (!IsValid(HealthWidget)) { return; }
    RegisterUIWidget(HealthWidget);

    HealthWidget->SetVisibility(ESlateVisibility::HitTestInvisible);

    if (!CharacterPlayer) return;
    CharacterPlayer->GetHealthComponent()->OnHealthChanged.AddUObject(HealthWidget, &UHealthWidget::SetHealthBarPercent);
}

void UPlayerUISubsystem::InitStaminaUI(TSubclassOf<UStaminaWidget> WidgetClass)
{
    if (!WidgetClass) { return; }
    APlayerController* PlayerController = GetCustomPlayerController();
    if (!IsValid(PlayerController)) { return; }
    UStaminaWidget* StaminaWidget = CreateWidget<UStaminaWidget>(PlayerController, WidgetClass);
    if (!IsValid(StaminaWidget)) { return; }
    RegisterUIWidget(StaminaWidget);
    StaminaWidget->SetVisibility(ESlateVisibility::HitTestInvisible);

    if (!CharacterPlayer) return;
    CharacterPlayer->OnStaminaUpdated.AddUObject(StaminaWidget, &UStaminaWidget::UpdateStaminaBar);
}

void UPlayerUISubsystem::InitInteractionWidget(TSubclassOf<UInteractionWidget> WidgetClass)
{
    if (!WidgetClass) { return; }
    APlayerController* PlayerController = GetCustomPlayerController();
    if (!IsValid(PlayerController)) { return; }
    UInteractionWidget* InteractionWidget = CreateWidget<UInteractionWidget>(PlayerController, WidgetClass);
    if (!IsValid(InteractionWidget)) { return; }
    RegisterUIWidget(InteractionWidget);

    InteractionWidget->SetVisibility(ESlateVisibility::Hidden);

    CharacterPlayer->OnInteractionUIPopUpDelegate.AddUObject(this, &UPlayerUISubsystem::PlayPopUpInteractionWidgetAnim);
    CharacterPlayer->OnInteractionUIUpdatedDelegate.AddUObject(this, &UPlayerUISubsystem::UpdateInteractionUI);
}

void UPlayerUISubsystem::InitInventoryUI(TSubclassOf<UPlayerDisplayWidget> WidgetClass)
{
    if (!WidgetClass) { return; }
    APlayerController* PlayerController = GetCustomPlayerController();
    if (!IsValid(PlayerController)) { return; }
    UPlayerDisplayWidget* InventoryWidget = CreateWidget<UPlayerDisplayWidget>(PlayerController, WidgetClass);
    if (!IsValid(InventoryWidget)) { return; }
    RegisterUIWidget(InventoryWidget);
    HideUI(InventoryWidget);

    InventoryWidget->GetItemInventoryUIWidget()->OnDropInventorySlotRequestedDelegate.AddUObject(this, &UPlayerUISubsystem::RequestDropInventorySlot);
    InventoryWidget->GetItemInventoryUIWidget()->OnSwapInventorySlotsRequestedDelegate.AddUObject(this, &UPlayerUISubsystem::RequestSwapInventorySlots);
    InventoryWidget->GetItemInventoryUIWidget()->OnUseInventorySlotRequestedDelegate.AddUObject(this, &UPlayerUISubsystem::RequestUseInventorySlot);

    InventoryWidget->GetWeaponInventoryUIWidget()->OnDropInventorySlotRequestedDelegate.AddUObject(this, &UPlayerUISubsystem::RequestDropInventorySlot);
    InventoryWidget->GetWeaponInventoryUIWidget()->OnSwapInventorySlotsRequestedDelegate.AddUObject(this, &UPlayerUISubsystem::RequestSwapInventorySlots);
    InventoryWidget->GetWeaponInventoryUIWidget()->OnUseInventorySlotRequestedDelegate.AddUObject(this, &UPlayerUISubsystem::RequestUseInventorySlot);

    CharacterPlayer->OnInventoryCreatedDelegate.AddUObject(InventoryWidget, &UPlayerDisplayWidget::CreateItemInventorySlots);
    CharacterPlayer->OnInventoryUpdatedDelegate.AddUObject(InventoryWidget, &UPlayerDisplayWidget::UpdateItemInventorySlots);

    CharacterPlayer->OnWeaponInventoryCreatedDelegate.AddUObject(InventoryWidget, &UPlayerDisplayWidget::CreateWeaponInventorySlots);
    CharacterPlayer->OnWeaponInventoryUpdatedDelegate.AddUObject(InventoryWidget, &UPlayerDisplayWidget::UpdateWeaponInventorySlots);
}

void UPlayerUISubsystem::InitThrowableWeaponInventoryUI(TSubclassOf<UThrowableWeaponInventoryWidget> WidgetClass)
{
    if (!WidgetClass) { return; }
    APlayerController* PlayerController = GetCustomPlayerController();
    if (!IsValid(PlayerController)) { return; }
    UThrowableWeaponInventoryWidget* ThrowableWeaponInventoryWidget = CreateWidget<UThrowableWeaponInventoryWidget>(PlayerController, WidgetClass);
    if (!IsValid(ThrowableWeaponInventoryWidget)) { return; }
    RegisterUIWidget(ThrowableWeaponInventoryWidget);
    HideUI(EUIType::ThrowableWeaponInventory);

    ThrowableWeaponInventoryWidget->OnThrowableWeaponEquipRequested.AddUObject(this, &UPlayerUISubsystem::HandleThrowableWeaponEquipRequested);

    CharacterPlayer->OnThrowableWeaponInventoryCreatedDelegate.AddUObject(ThrowableWeaponInventoryWidget, &UThrowableWeaponInventoryWidget::CreateInventorySlots);
    CharacterPlayer->OnThrowableWeaponInventoryUpdatedDelegate.AddUObject(ThrowableWeaponInventoryWidget, &UThrowableWeaponInventoryWidget::UpdateInventorySlots);

}

//void UPlayerUISubsystem::InitGameplayUI(TSubclassOf<UBaseUIWidget> WidgetClass)
//{
//
//}

void UPlayerUISubsystem::OpenGameOverUI(EGameEndReason EndReason)
{
    UGameOverWidget* GameOverWidget = Cast<UGameOverWidget>(GetUIWidget(EUIType::GameOver));
    if (!IsValid(GameOverWidget)) { return; }
    APlayerController* PlayerController = GetCustomPlayerController();
    if (!IsValid(PlayerController)) { return; }

    ShowUI(EUIType::GameOver);
    //PlayUISound(ESoundID::);

    PlayerController->SetShowMouseCursor(true);

    //FInputModeGameAndUI InputMode;
    FInputModeUIOnly InputMode;
    InputMode.SetWidgetToFocus(GameOverWidget->TakeWidget());
    //InputMode.SetHideCursorDuringCapture(false);

    PlayerController->SetInputMode(InputMode);


    //FInputModeUIOnly InputMode;
    //if (IsValid(FocusWidget))
    //{
    //    InputMode.SetWidgetToFocus(FocusWidget->TakeWidget());
    //}
    //PlayerController->SetInputMode(InputMode);
}

void UPlayerUISubsystem::CloseGameOverUI()
{
    APlayerController* PlayerController = GetCustomPlayerController();
    if (!IsValid(PlayerController)) { return; }
    HideUI(EUIType::GameOver);
    //PlayUISound(ESoundID::UI_Close);

    PlayerController->SetShowMouseCursor(false);

    FInputModeGameOnly InputMode;
    PlayerController->SetInputMode(InputMode);
}

bool UPlayerUISubsystem::IsGameOverUIOpened() const
{
    const UBaseUIWidget* GameOverWidget = GetUIWidget(EUIType::GameOver);
    if (!IsValid(GameOverWidget))
    {
        return false;
    }
    return GameOverWidget->GetVisibility() != ESlateVisibility::Collapsed;
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

void UPlayerUISubsystem::HandleBackToMainMenuRequested()
{
    //UBaseUIWidget* MainMenuWidget = GetUIWidget(EUIType::MainMenu);

    //if (!IsValid(MainMenuWidget)) { return; }

    //HideUI(EUIType::MapSelectMenu);
    //ShowUI(EUIType::MainMenu);

    //PlayUISound(ESoundID::UI_Click);

    //SetUIOnlyInput(MainMenuWidget);

    //---------------------
    //TODO: Open MainMenu Level


    ULocalPlayer* LocalPlayer = GetLocalPlayer();
    if (!IsValid(LocalPlayer)) { return; }
    UGameInstance* GameInstance = LocalPlayer->GetGameInstance();
    if (!IsValid(GameInstance)) { return; }
    UGameFlowSubsystem* GameFlowSubsystem = GameInstance->GetSubsystem<UGameFlowSubsystem>();
    if (!IsValid(GameFlowSubsystem)) { return; }

    PlayUISound(ESoundID::UI_Click);

    GameFlowSubsystem->ReturnToMainMenu();
}

void UPlayerUISubsystem::UpdateStaminaBar(float maxstamina, float currstamina)
{
    UStaminaWidget* StaminaWidget = Cast<UStaminaWidget>(GetUIWidget(EUIType::Stamina));
    if (!IsValid(StaminaWidget)) { return; }
    StaminaWidget->UpdateStaminaBar(maxstamina, currstamina);
}

void UPlayerUISubsystem::OpenInventory()
{
    UPlayerDisplayWidget* InventoryWidget = Cast<UPlayerDisplayWidget>(GetUIWidget(EUIType::Inventory));
    if (!IsValid(InventoryWidget)) { return; }

    ShowUI(InventoryWidget);
    InventoryWidget->OpenInventory();
}
void UPlayerUISubsystem::CloseInventory()
{
    UPlayerDisplayWidget* InventoryWidget = Cast<UPlayerDisplayWidget>(GetUIWidget(EUIType::Inventory));
    if (!IsValid(InventoryWidget)) { return; }

    HideUI(InventoryWidget);
    InventoryWidget->CloseInventory();
}

void UPlayerUISubsystem::RequestDropInventorySlot(FName InventoryName, int32 SlotIndex)
{
    if (CharacterPlayer)
    {
        CharacterPlayer->OnInventorySlotDropRequestedDelegate.Broadcast(InventoryName, SlotIndex);
    }
}

void UPlayerUISubsystem::RequestUseInventorySlot(FName InventoryName, int32 SlotIndex)
{
    UE_LOG(LogTemp, Error, TEXT("void UUIManagerComponent::RequestUseInventorySlot(FName InventoryName, int32 SlotIndex)"));
    if (CharacterPlayer)
    {
        CharacterPlayer->OnInventorySlotUseRequestedDelegate.Broadcast(InventoryName, SlotIndex);
    }
}

void UPlayerUISubsystem::RequestSwapInventorySlots(FName InventoryName, int32 FromIndex, int32 ToIndex)
{
    if (CharacterPlayer)
    {
        CharacterPlayer->OnInventorySwapRequestedDelegate.Broadcast(InventoryName, FromIndex, ToIndex);
    }
}

void UPlayerUISubsystem::HandleThrowableWeaponEquipRequested(int32 SlotIndex)
{
    if (!IsValid(CharacterPlayer)) { return; }
    if (SlotIndex == INDEX_NONE) { return; }

    CharacterPlayer->OnThrowableWeaponEquipRequestedDelegate.Broadcast(SlotIndex);
}

void UPlayerUISubsystem::OpenThrowableWeaponInventory()
{
    UThrowableWeaponInventoryWidget* ThrowableWeaponInventoryWidget = Cast<UThrowableWeaponInventoryWidget>(GetUIWidget(EUIType::ThrowableWeaponInventory));
    if (!IsValid(ThrowableWeaponInventoryWidget)) { return; }
    APlayerController* PlayerController = GetCustomPlayerController();
    if (!IsValid(PlayerController)) { return; }

    ShowUI(EUIType::ThrowableWeaponInventory);
    //PlayUISound(ESoundID::);

    PlayerController->SetShowMouseCursor(true);

    FInputModeGameAndUI InputMode;
    //FInputModeUIOnly InputMode;
    InputMode.SetWidgetToFocus(ThrowableWeaponInventoryWidget->TakeWidget());
    //InputMode.SetHideCursorDuringCapture(false);

    PlayerController->SetInputMode(InputMode);

    //---------------
    //if (!ThrowableWeaponInventoryWidget) return;
    //ThrowableWeaponInventoryWidget->OpenUI();
}

void UPlayerUISubsystem::CloseThrowableWeaponInventory()
{
    APlayerController* PlayerController = GetCustomPlayerController();
    if (!IsValid(PlayerController)) { return; }
    HideUI(EUIType::ThrowableWeaponInventory);
    //PlayUISound(ESoundID::UI_Close);

    PlayerController->SetShowMouseCursor(false);

    FInputModeGameOnly InputMode;
    PlayerController->SetInputMode(InputMode);

    //-----------

    //if (!ThrowableWeaponInventoryWidget) return;
    //ThrowableWeaponInventoryWidget->CloseUI();
}

void UPlayerUISubsystem::PlayPopUpInteractionWidgetAnim()
{
    UInteractionWidget* InteractionWidget = Cast<UInteractionWidget>(GetUIWidget(EUIType::Interaction));
    if (!IsValid(InteractionWidget)) { return; }
    if (InteractionWidget) InteractionWidget->PlayPopUpAnim();

    UE_LOG(LogTemp, Error, TEXT("void UPlayerUISubsystem::PlayPopUpInteractionWidgetAnim()"));
}


void UPlayerUISubsystem::UpdateInteractionUI(bool bFlag, FVector NewLocation)
{
    UInteractionWidget* InteractionWidget = Cast<UInteractionWidget>(GetUIWidget(EUIType::Interaction));
    if (!IsValid(InteractionWidget)) { return; }

    FVector2D TargetScreenPosition = GetScreenPositionOfWorldLocation(NewLocation).Get<0>();

    if (bFlag)
    {
        if (IsInViewport(TargetScreenPosition, 1.f, 1.f))
        {
            InteractionWidget->SetPositionInViewport(TargetScreenPosition);

            if (InteractionWidget->Visibility == ESlateVisibility::Hidden)
            {
                InteractionWidget->SetVisibility(ESlateVisibility::Visible);
            }
        }
    }
    else
    {
        if (InteractionWidget->Visibility == ESlateVisibility::Visible)
        {
            InteractionWidget->SetVisibility(ESlateVisibility::Hidden);
        }
    }

    //UE_LOG(LogTemp, Error, TEXT("void UPlayerUISubsystem::UpdateInteractionUI(bool bFlag, FVector NewLocation)"));
}


TTuple<FVector2D, bool> UPlayerUISubsystem::GetScreenPositionOfWorldLocation(const FVector& SearchLocation) const
{
    FVector2D ScreenLocation = FVector2D::ZeroVector;
    bool bResult = UGameplayStatics::ProjectWorldToScreen(GetCustomPlayerController(), SearchLocation, ScreenLocation);

    return MakeTuple(ScreenLocation, bResult);
}

bool UPlayerUISubsystem::IsInViewport(FVector2D ActorScreenPosition, float ScreenRatio_Width, float ScreenRatio_Height) const
{
    FVector2D ViewportSize = GEngine->GameViewport->Viewport->GetSizeXY();

    bool bIsInWidth = true;
    bool bIsInHeight = true;

    // Check Width
    if (ScreenRatio_Width == 0.0f || UKismetMathLibrary::Abs(ScreenRatio_Width) > 1.0f || (ScreenRatio_Width == (1.0f - ScreenRatio_Width)))
    {
        if (ActorScreenPosition.X >= 0.0f && ActorScreenPosition.X <= ViewportSize.X)
        {
            bIsInWidth = true;
        }
        else
        {
            bIsInWidth = false;
        }
    }
    else
    {
        float LargeScreenRatio_Width;
        float SmallScreenRatio_Width;

        if (ScreenRatio_Width < (1.0f - ScreenRatio_Width))
        {
            LargeScreenRatio_Width = 1.0f - ScreenRatio_Width;
            SmallScreenRatio_Width = ScreenRatio_Width;
        }
        else
        {
            LargeScreenRatio_Width = ScreenRatio_Width;
            SmallScreenRatio_Width = 1.0f - ScreenRatio_Width;
        }

        if (ActorScreenPosition.X >= ViewportSize.X * SmallScreenRatio_Width && ActorScreenPosition.X <= ViewportSize.X * LargeScreenRatio_Width)
        {
            bIsInWidth = true;
        }
        else
        {
            bIsInWidth = false;
        }
    }

    // Check Height
    if (ScreenRatio_Height == 0.0f || UKismetMathLibrary::Abs(ScreenRatio_Height) > 1.0f || (ScreenRatio_Height == (1.0f - ScreenRatio_Height)))
    {
        if (ActorScreenPosition.Y >= 0.0f && ActorScreenPosition.Y <= ViewportSize.Y)
        {
            bIsInHeight = true;
        }
        else
        {
            bIsInHeight = false;
        }
    }
    else
    {
        float LargeScreenRatio_Height;
        float SmallScreenRatio_Height;

        if (ScreenRatio_Height < (1.0f - ScreenRatio_Height))
        {
            LargeScreenRatio_Height = 1.0f - ScreenRatio_Height;
            SmallScreenRatio_Height = ScreenRatio_Height;
        }
        else
        {
            LargeScreenRatio_Height = ScreenRatio_Height;
            SmallScreenRatio_Height = 1.0f - ScreenRatio_Height;
        }

        if (ActorScreenPosition.Y >= ViewportSize.Y * SmallScreenRatio_Height && ActorScreenPosition.Y <= ViewportSize.Y * LargeScreenRatio_Height)
        {
            bIsInHeight = true;
        }
        else
        {
            bIsInHeight = false;
        }
    }

    // Return
    if (bIsInWidth && bIsInHeight)
    {
        return true;
    }
    else
    {
        return false;
    }
}
