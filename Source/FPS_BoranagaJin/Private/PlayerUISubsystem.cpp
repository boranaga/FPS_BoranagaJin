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
#include "UI/QuickSlotWidget.h"
#include "UI/ExitDoorWidget.h"
#include "Objects/ExitDoor.h"

#include "Instance/DefaultGameInstance.h"
#include "Characters/Player/FPSPlayerController.h"
#include "Characters/Player/CharacterPlayer.h"
#include "Characters/InteractionComponent.h"
#include "Characters/HealthComponent.h"
#include "SaveSystem/SaveGameSubsystem.h"
//#include "SaveSystem/SaveGameCustom.h"

#include "SoundSystem/GameAudioSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
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

    // Door actors may be ending play concurrently with the local-player subsystem.
    CloseExitDoorUI();
    ReleaseExitDoorCursor();
    ActiveExitDoor.Reset();
    NearbyExitDoors.Reset();
    ReleaseExitDoorWidget();

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

    WidgetArray.Widgets.AddUnique(NewWidget);

    if (!NewWidget->IsInViewport())
    {
        NewWidget->AddToPlayerScreen(Layer);
    }
}

void UPlayerUISubsystem::UnregisterUIWidget(UBaseUIWidget* Widget)
{
    if (!IsValid(Widget)) { return; }

    for (auto It = UIWidgets.CreateIterator(); It; ++It)
    {
        It.Value().Widgets.Remove(Widget);
        if (It.Value().Widgets.IsEmpty())
        {
            It.RemoveCurrent();
        }
    }
    Widget->RemoveFromParent();
}

void UPlayerUISubsystem::RegisterNearbyExitDoor(AExitDoor* Door)
{
    if (!IsValid(Door)) { return; }

    NearbyExitDoors.RemoveAll([Door](const TWeakObjectPtr<AExitDoor>& Entry)
        {
            return !Entry.IsValid() || Entry.Get() == Door;
        });
    NearbyExitDoors.Add(Door);

    // Do NOT show the puzzle panel just because the player entered the volume.
}

void UPlayerUISubsystem::UnregisterNearbyExitDoor(AExitDoor* Door)
{
    if (ActiveExitDoor.Get() == Door)
    {
        CloseExitDoorUI();
    }
    NearbyExitDoors.RemoveAll([Door](const TWeakObjectPtr<AExitDoor>& Entry)
        {
            return !Entry.IsValid() || Entry.Get() == Door;
        });
}

bool UPlayerUISubsystem::IsActiveExitDoor(const AExitDoor* Door) const
{
    return bExitDoorPanelOpen && IsValid(Door) && ActiveExitDoor.Get() == Door;
}

bool UPlayerUISubsystem::OpenExitDoorUI(AExitDoor* Door, ACharacterPlayer* InteractingPlayer)
{
    if (!IsValid(Door) || !IsValid(InteractingPlayer) ||
        !Door->IsPlayerInExitRange(InteractingPlayer) ||
        InteractingPlayer->GetController() != GetCustomPlayerController() ||
        IsOtherModalUIVisible())
    {
        return false;
    }

    // Verify that the interaction key was pressed while actually targeting this door.
    const UInteractionComponent* Interaction = InteractingPlayer->GetInteractionComponent();
    if (!IsValid(Interaction) || Interaction->GetCurrentInteractionTarget() != Door)
    {
        return false;
    }

    if (!NearbyExitDoors.ContainsByPredicate([Door](const TWeakObjectPtr<AExitDoor>& Entry)
        { return Entry.Get() == Door; }))
    {
        return false;
    }

    if (!EnsureExitDoorWidget(Door->WidgetClass)) { return false; }

    // A different door can be opened only by explicitly interacting with it.
    if (bExitDoorPanelOpen && ActiveExitDoor.Get() != Door)
    {
        CloseExitDoorUI();
    }

    ActiveExitDoor = Door;
    bExitDoorPanelOpen = true;
    UpdateInteractionUI(false, FVector::ZeroVector);
    ShowUI(EUIType::ExitDoor);
    if (UExitDoorWidget* Widget = Cast<UExitDoorWidget>(GetUIWidget(EUIType::ExitDoor)))
    {
        Widget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    }

    Door->RefreshWidget();
    SetExitDoorCursorWanted(Door, true);
    return true;
}

void UPlayerUISubsystem::CloseExitDoorUI()
{
    if (!bExitDoorPanelOpen) { return; }

    AExitDoor* PreviouslyOpenDoor = ActiveExitDoor.Get();
    bExitDoorPanelOpen = false;
    ActiveExitDoor.Reset();
    LastExitDoorClosedFrame = GFrameCounter;

    HideUI(EUIType::ExitDoor);
    ReleaseExitDoorCursor();

    // If the player is still looking at the exit, show its interaction prompt again.
    if (IsValid(PreviouslyOpenDoor) && IsValid(PreviouslyOpenDoor->NearbyPlayer))
    {
        PreviouslyOpenDoor->RefreshInteractionPrompt(PreviouslyOpenDoor->NearbyPlayer);
    }
}

bool UPlayerUISubsystem::EnsureExitDoorWidget(TSubclassOf<UExitDoorWidget> WidgetClass)
{
    if (!WidgetClass) { WidgetClass = UExitDoorWidget::StaticClass(); }

    // Like the other UIs, the widget lives only in the UIWidgets registry.
    UExitDoorWidget* Widget = Cast<UExitDoorWidget>(GetUIWidget(EUIType::ExitDoor));

    // Different exits may use different WBP subclasses. Replace the registered widget.
    if (IsValid(Widget) && Widget->GetClass() != WidgetClass.Get())
    {
        ReleaseExitDoorWidget();
        Widget = nullptr;
    }

    if (!IsValid(Widget))
    {
        APlayerController* PlayerController = GetCustomPlayerController();
        if (!IsValid(PlayerController)) { return false; }

        Widget = CreateWidget<UExitDoorWidget>(PlayerController, WidgetClass);
        if (!IsValid(Widget))
        {
            UE_LOG(LogTemp, Warning, TEXT("PlayerUISubsystem: failed to create ExitDoorWidget."));
            return false;
        }

        // RegisterUIWidget stores it in UIWidgets[EUIType::ExitDoor] and adds it to the screen.
        RegisterUIWidget(Widget);
        if (GetUIWidget(EUIType::ExitDoor) != Widget)
        {
            UE_LOG(LogTemp, Error, TEXT("PlayerUISubsystem: failed to register ExitDoorWidget."));
            Widget->RemoveFromParent();
            return false;
        }

        Widget->OnSlotClicked.AddUObject(this, &UPlayerUISubsystem::HandleExitDoorSlotClicked);
        HideUI(EUIType::ExitDoor);
    }

    return true;
}

void UPlayerUISubsystem::ReleaseExitDoorWidget()
{
    UExitDoorWidget* Widget = Cast<UExitDoorWidget>(GetUIWidget(EUIType::ExitDoor));
    if (!IsValid(Widget)) { return; }

    Widget->OnSlotClicked.RemoveAll(this);
    UnregisterUIWidget(Widget);
}

void UPlayerUISubsystem::HandleExitDoorSlotClicked(int32 SlotIndex)
{
    AExitDoor* Door = ActiveExitDoor.Get();
    if (!IsValid(Door) || !IsValid(Door->NearbyPlayer)) { return; }
    Door->HandleSlotClicked(SlotIndex);
}

bool UPlayerUISubsystem::IsOtherModalUIVisible() const
{
    constexpr EUIType ModalTypes[] = {
        EUIType::PauseMenu, EUIType::GameOver, EUIType::Inventory,
        EUIType::ThrowableWeaponInventory, EUIType::MainMenu,
        EUIType::MapSelectMenu, EUIType::SaveFileSlotMenu
    };

    for (EUIType Type : ModalTypes)
    {
        const UBaseUIWidget* Widget = GetUIWidget(Type);
        if (IsValid(Widget) && Widget->IsInViewport() &&
            Widget->GetVisibility() != ESlateVisibility::Collapsed &&
            Widget->GetVisibility() != ESlateVisibility::Hidden)
        {
            return true;
        }
    }
    return false;
}

void UPlayerUISubsystem::ReleaseExitDoorCursor()
{
    if (!bExitDoorOwnsCursor) { return; }
    bExitDoorOwnsCursor = false;

    APlayerController* PlayerController = ExitDoorCursorController.Get();
    ExitDoorCursorController.Reset();
    if (!IsValid(PlayerController) || IsOtherModalUIVisible()) { return; }

    PlayerController->SetInputMode(FInputModeGameOnly());
    PlayerController->SetShowMouseCursor(false);
}

void UPlayerUISubsystem::SetExitDoorCursorWanted(const AExitDoor* Door, bool bWantCursor)
{
    if (!bExitDoorPanelOpen || !IsValid(Door) || ActiveExitDoor.Get() != Door) { return; }
    if (!bWantCursor)
    {
        ReleaseExitDoorCursor();
        return;
    }

    APlayerController* PlayerController = GetCustomPlayerController();
    if (!IsValid(PlayerController)) { return; }

    // Another menu may have restored GameOnly input after we originally owned it.
    if (bExitDoorOwnsCursor && !PlayerController->bShowMouseCursor)
    {
        bExitDoorOwnsCursor = false;
        ExitDoorCursorController.Reset();
    }

    // Never take over a modal UI or a cursor already owned by another widget.
    if (IsOtherModalUIVisible()) { return; }
    if (!bExitDoorOwnsCursor && !PlayerController->bShowMouseCursor)
    {
        FInputModeGameAndUI InputMode;
        InputMode.SetHideCursorDuringCapture(false);
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        if (UExitDoorWidget* Widget = Cast<UExitDoorWidget>(GetUIWidget(EUIType::ExitDoor)))
        {
            InputMode.SetWidgetToFocus(Widget->TakeWidget());
        }
        PlayerController->SetInputMode(InputMode);
        PlayerController->SetShowMouseCursor(true);
        ExitDoorCursorController = PlayerController;
        bExitDoorOwnsCursor = true;
    }
    else if (UExitDoorWidget* Widget = Cast<UExitDoorWidget>(GetUIWidget(EUIType::ExitDoor)))
    {
        Widget->SetUserFocus(PlayerController);
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

    // Gameplay UI Delegate ����
}

void UPlayerUISubsystem::UnbindCharacterDelegates()
{
    if (!IsValid(CharacterPlayer)) { return; }

    // Gameplay UI Delegate ����
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
//    // Health, Stamina, AmmoCounter ���� Gameplay UI �ʱ�ȭ
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
    //TODO: ���� ���� slot�� ���� ó���� �Ϸ��ϰ� ������ �����ؾ���.
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

    //TODO: nullptr ��� �ڵ�
    if (InventoryWidget->GetItemInventoryUIWidget())
    {
        InventoryWidget->GetItemInventoryUIWidget()->OnDropInventorySlotRequestedDelegate.AddUObject(this, &UPlayerUISubsystem::RequestDropInventorySlot);
        InventoryWidget->GetItemInventoryUIWidget()->OnSwapInventorySlotsRequestedDelegate.AddUObject(this, &UPlayerUISubsystem::RequestSwapInventorySlots);
        InventoryWidget->GetItemInventoryUIWidget()->OnUseInventorySlotRequestedDelegate.AddUObject(this, &UPlayerUISubsystem::RequestUseInventorySlot);
    }
    if (InventoryWidget->GetPrimaryWeaponInventoryUIWidget())
    {
        InventoryWidget->GetPrimaryWeaponInventoryUIWidget()->OnDropInventorySlotRequestedDelegate.AddUObject(this, &UPlayerUISubsystem::RequestDropInventorySlot);
        InventoryWidget->GetPrimaryWeaponInventoryUIWidget()->OnSwapInventorySlotsRequestedDelegate.AddUObject(this, &UPlayerUISubsystem::RequestSwapInventorySlots);
        InventoryWidget->GetPrimaryWeaponInventoryUIWidget()->OnUseInventorySlotRequestedDelegate.AddUObject(this, &UPlayerUISubsystem::RequestUseInventorySlot);
    }
    if (InventoryWidget->GetSecondaryWeaponInventoryUIWidget())
    {
        InventoryWidget->GetSecondaryWeaponInventoryUIWidget()->OnDropInventorySlotRequestedDelegate.AddUObject(this, &UPlayerUISubsystem::RequestDropInventorySlot);
        InventoryWidget->GetSecondaryWeaponInventoryUIWidget()->OnSwapInventorySlotsRequestedDelegate.AddUObject(this, &UPlayerUISubsystem::RequestSwapInventorySlots);
        InventoryWidget->GetSecondaryWeaponInventoryUIWidget()->OnUseInventorySlotRequestedDelegate.AddUObject(this, &UPlayerUISubsystem::RequestUseInventorySlot);
    }

    CharacterPlayer->OnInventoryCreatedDelegate.AddUObject(InventoryWidget, &UPlayerDisplayWidget::CreateItemInventorySlots);
    CharacterPlayer->OnInventoryUpdatedDelegate.AddUObject(InventoryWidget, &UPlayerDisplayWidget::UpdateItemInventorySlots);

    CharacterPlayer->OnPrimaryWeaponInventoryCreatedDelegate.AddUObject(InventoryWidget, &UPlayerDisplayWidget::CreatePrimaryWeaponInventorySlots);
    CharacterPlayer->OnPrimaryWeaponInventoryUpdated.AddUObject(InventoryWidget, &UPlayerDisplayWidget::UpdatePrimaryWeaponInventorySlots);

    CharacterPlayer->OnSecondaryWeaponInventoryCreatedDelegate.AddUObject(InventoryWidget, &UPlayerDisplayWidget::CreateSecondaryWeaponInventorySlots);
    CharacterPlayer->OnSecondaryWeaponInventoryUpdated.AddUObject(InventoryWidget, &UPlayerDisplayWidget::UpdateSecondaryWeaponInventorySlots);
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

void UPlayerUISubsystem::InitQuickSlotUI(TSubclassOf<UQuickSlotWidget> WidgetClass)
{
    if (!WidgetClass) { return; }
    APlayerController* PlayerController = GetCustomPlayerController();
    if (!IsValid(PlayerController)) { return; }
    UQuickSlotWidget* QuickSlotWidget = CreateWidget<UQuickSlotWidget>(PlayerController, WidgetClass);
    if (!IsValid(QuickSlotWidget)) { return; }
    RegisterUIWidget(QuickSlotWidget);
    ShowUI(EUIType::QuickSlot);

    CharacterPlayer->OnQuickSlotUpdatedDelegate.AddUObject(QuickSlotWidget, &UQuickSlotWidget::UpdateQuickSlots);

    CharacterPlayer->OnQuickSlotSelectionChanged.AddUObject(QuickSlotWidget, &UQuickSlotWidget::HandleQuickSlotSelectionChanged); //TODO: �ʿ��Ѱ�?
}

//void UPlayerUISubsystem::InitGameplayUI(TSubclassOf<UBaseUIWidget> WidgetClass)
//{
//
//}

void UPlayerUISubsystem::OpenGameOverUI(EGameEndReason EndReason)
{
    if (IsExitDoorUIOpen()) { CloseExitDoorUI(); }
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
    // Restore the proximity UI cursor if the exit is still active.
    if (AExitDoor* Door = ActiveExitDoor.Get())
    {
        Door->RefreshWidget();
    }
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
    if (IsExitDoorUIOpen())
    {
        CloseExitDoorUI();
        return;
    }
    if (LastExitDoorClosedFrame == GFrameCounter) { return; }

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
    if (IsExitDoorUIOpen()) { CloseExitDoorUI(); return; }
    if (LastExitDoorClosedFrame == GFrameCounter) { return; }

    UE_LOG(LogTemp, Error, TEXT("void UPlayerUISubsystem::OpenPauseMenu()"));

    UPauseMenuWidget* PauseMenuWidget = Cast<UPauseMenuWidget>(GetUIWidget(EUIType::PauseMenu));

    if (!IsValid(PauseMenuWidget)) { return; }

    APlayerController* PlayerController = GetCustomPlayerController();

    if (!IsValid(PlayerController)) { return; }

    //TODO: GamePause�� ���ϴ� ����� ���� ����
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

    //TODO: �Ͻ������� ���� ���� �ʿ�
    //UGameplayStatics::SetGamePaused(this, false);

    PlayerController->SetShowMouseCursor(false);

    //TODO: �ٵ� Inventory�� Ų ���¿��� esc�� ������ ��� ���۵Ǿ�� �ϴ��� ���ǰ� �ʿ���
    FInputModeGameOnly InputMode;
    PlayerController->SetInputMode(InputMode);
    // Restore the proximity UI cursor if the exit is still active.
    if (AExitDoor* Door = ActiveExitDoor.Get())
    {
        Door->RefreshWidget();
    }
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
    if (IsExitDoorUIOpen()) { CloseExitDoorUI(); }
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
    // Restore the proximity UI cursor if the exit is still active.
    if (AExitDoor* Door = ActiveExitDoor.Get())
    {
        Door->RefreshWidget();
    }
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
    if (IsExitDoorUIOpen()) { CloseExitDoorUI(); }
    UThrowableWeaponInventoryWidget* ThrowableWeaponInventoryWidget = Cast<UThrowableWeaponInventoryWidget>(GetUIWidget(EUIType::ThrowableWeaponInventory));
    if (!IsValid(ThrowableWeaponInventoryWidget)) { return; }
    APlayerController* PlayerController = GetCustomPlayerController();
    if (!IsValid(PlayerController)) { return; }

    CharacterPlayer->OnThrowableWeaponInventoryUpdateRequestedDelegate.Broadcast();

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
    // Restore the proximity UI cursor if the exit is still active.
    if (AExitDoor* Door = ActiveExitDoor.Get())
    {
        Door->RefreshWidget();
    }
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

    // The interaction button and the item panel should never overlap.
    if (!bFlag || IsExitDoorUIOpen() || IsOtherModalUIVisible())
    {
        InteractionWidget->SetVisibility(ESlateVisibility::Hidden);
        return;
    }

    const TTuple<FVector2D, bool> Projected = GetScreenPositionOfWorldLocation(NewLocation);
    if (!Projected.Get<1>() || !IsInViewport(Projected.Get<0>(), 1.f, 1.f))
    {
        InteractionWidget->SetVisibility(ESlateVisibility::Hidden);
        return;
    }

    InteractionWidget->SetPositionInViewport(Projected.Get<0>());
    InteractionWidget->SetVisibility(ESlateVisibility::Visible);
}


TTuple<FVector2D, bool> UPlayerUISubsystem::GetScreenPositionOfWorldLocation(const FVector& SearchLocation) const
{
    FVector2D ScreenLocation = FVector2D::ZeroVector;
    bool bResult = UGameplayStatics::ProjectWorldToScreen(GetCustomPlayerController(), SearchLocation, ScreenLocation);

    return MakeTuple(ScreenLocation, bResult);
}

bool UPlayerUISubsystem::IsInViewport(FVector2D ActorScreenPosition, float ScreenRatio_Width, float ScreenRatio_Height) const
{
    if (!GEngine || !GEngine->GameViewport || !GEngine->GameViewport->Viewport) { return false; }
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
