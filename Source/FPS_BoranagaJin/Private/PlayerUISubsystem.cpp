#include "PlayerUISubsystem.h"
#include "GameFlowSubsystem.h"

#include "UI/BaseUIWidget.h"
#include "UI/MainMenuWidget.h"
#include "Characters/Player/FPSPlayerController.h"
#include "Characters/Player/CharacterPlayer.h"

#include "Engine/GameInstance.h"

void UPlayerUISubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    //InitializeUILayers();
}

void UPlayerUISubsystem::Deinitialize()
{
    UnbindCharacterDelegates();

    for (const TPair<EUIType, TObjectPtr<UBaseUIWidget>> &Pair : UIWidgets)
    {
        if (IsValid(Pair.Value))
        {
            Pair.Value->RemoveFromParent();
        }
    }

    UIWidgets.Reset();
    //UILayers.Reset();
    CharacterPlayer = nullptr;

    Super::Deinitialize();
}

void UPlayerUISubsystem::RegisterUIWidget(UBaseUIWidget* NewWidget)
{
    if (!IsValid(NewWidget)) { return; }

    const EUIType UIType = NewWidget->GetUIType();
    //const int32* Layer = UILayers.Find(UIType);
    const int32 Layer = GetUIZOrder(UIType);

    if (!Layer)
    {
        UE_LOG(LogTemp, Error, TEXT("UI layer is missing: %s"), *UEnum::GetValueAsString(UIType));
        return;
    }

    if (TObjectPtr<UBaseUIWidget>* Existing = UIWidgets.Find(UIType))
    {
        if (IsValid(*Existing) && *Existing != NewWidget)
        {
            (*Existing)->RemoveFromParent();
        }
    }

    UIWidgets.Add(UIType, NewWidget);

    if (!NewWidget->IsInViewport())
    {
        NewWidget->AddToPlayerScreen(Layer);
    }

    //TODO:
    //NewWidget->SetOwnerUIManager(this);
}

void UPlayerUISubsystem::ShowUI(EUIType UIType)
{
}

void UPlayerUISubsystem::HideUI(EUIType UIType)
{
}

UBaseUIWidget* UPlayerUISubsystem::GetUIWidget(EUIType UIType) const
{
    return nullptr;
}

void UPlayerUISubsystem::SetControlledCharacter(ACharacterPlayer* NewCharacter)
{
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

    if (!LocalPlayer)
    {
        return nullptr;
    }

    return Cast<APlayerController>(LocalPlayer->GetPlayerController(GetWorld()));
}

//void UPlayerUISubsystem::InitializeUILayers()
//{
//}

void UPlayerUISubsystem::BindCharacterDelegates()
{
}

void UPlayerUISubsystem::UnbindCharacterDelegates()
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

    MainMenuWidget->OnPlayRequested.AddUObject(this, &UPlayerUISubsystem::HandlePlayRequested);
    MainMenuWidget->OnExitRequested.AddUObject(this, &UPlayerUISubsystem::HandleExitRequested);

    //TODO: 아래 부분 처리에 대한 고민 필요. 어디서 SetInputMode를 담당할지

    PlayerController->SetShowMouseCursor(true);

    FInputModeUIOnly InputMode; 
    InputMode.SetWidgetToFocus(MainMenuWidget->TakeWidget());

    PlayerController->SetInputMode(InputMode);
}

void UPlayerUISubsystem::InitGameplayUI()
{
}

void UPlayerUISubsystem::HandlePlayRequested()
{
    ULocalPlayer* LocalPlayer = GetLocalPlayer();

    if (!IsValid(LocalPlayer))
    {
        return;
    }

    UGameInstance* GameInstance = LocalPlayer->GetGameInstance();

    if (!IsValid(GameInstance))
    {
        return;
    }

    UGameFlowSubsystem* GameFlowSubsystem = GameInstance->GetSubsystem<UGameFlowSubsystem>();

    if (!IsValid(GameFlowSubsystem))
    {
        return;
    }

    const bool bStarted = GameFlowSubsystem->StartNewGame();
}

void UPlayerUISubsystem::HandleExitRequested()
{
    ULocalPlayer* LocalPlayer = GetLocalPlayer();

    if (!IsValid(LocalPlayer))
    {
        return;
    }

    UGameInstance* GameInstance = LocalPlayer->GetGameInstance();

    if (!IsValid(GameInstance))
    {
        return;
    }

    UGameFlowSubsystem* GameFlowSubsystem = GameInstance->GetSubsystem<UGameFlowSubsystem>();

    GameFlowSubsystem->QuitGame();
}
