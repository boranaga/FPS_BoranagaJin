#include "GameFlowSubsystem.h"
#include "Instance/DefaultGameInstance.h"
#include "SaveSystem/SaveGameSubsystem.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogGameFlowSubsystem, Log, All);

void UGameFlowSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    CurrentState = EGameFlowState::None;
    ChangeState(EGameFlowState::Boot);

    bLevelTransitionInProgress = false;

    PostLoadMapDelegateHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UGameFlowSubsystem::HandlePostLoadMap);

    UE_LOG(LogGameFlowSubsystem, Log, TEXT("GameFlowSubsystem initialized."));
}

void UGameFlowSubsystem::Deinitialize()
{
    if (PostLoadMapDelegateHandle.IsValid())
    {
        FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapDelegateHandle);
        PostLoadMapDelegateHandle.Reset();
    }

    OnGameFlowStateChanged.Clear();
    Super::Deinitialize();
}

bool UGameFlowSubsystem::StartNewGame()
{
    ChangeState(EGameFlowState::Loading);

    const UCustomGameInstance* FPSGameInstance = Cast<UCustomGameInstance>(GetGameInstance());

    if (!IsValid(FPSGameInstance))
    {
        UE_LOG(LogGameFlowSubsystem, Error, TEXT("StartNewGame failed: ""GameInstance is not UFPSGameInstance."));

        return false;
    }

    /*
     * 세이브 시스템을 도입한 경우 여기에서 새 게임 데이터를
     * 초기화하면 됩니다.
     *
     * USaveGameSubsystem* SaveSubsystem = GetGameInstance()->GetSubsystem<USaveGameSubsystem>();
     *
     * SaveSubsystem->StartNewGame(...);
     */

    return OpenLevel(FPSGameInstance->GetFirstGameLevel());
}

void UGameFlowSubsystem::ContinueGame()
{
    // SaveGameSubsystem에서 마지막 저장 데이터를 읽은 뒤
    // 저장된 레벨로 이동하도록 구현합니다.
}

bool UGameFlowSubsystem::RestartFromCheckPoint()
{
    UGameInstance* GameInstance = GetGameInstance();
    if (!IsValid(GameInstance)) { return false; }

    USaveGameSubsystem* SaveSubsystem = GameInstance->GetSubsystem<USaveGameSubsystem>();
    if (!IsValid(SaveSubsystem)) { return false; }

    if (!SaveSubsystem->LoadGame()) { return false; }

    return SaveSubsystem->OpenSavedLevel();
}

void UGameFlowSubsystem::PauseGame()
{
    if (CurrentState != EGameFlowState::Playing)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0);

    if (!PlayerController)
    {
        return;
    }

    PlayerController->SetPause(true);

    FInputModeUIOnly InputMode;
    PlayerController->SetInputMode(InputMode);
    PlayerController->SetShowMouseCursor(true);

    ChangeState(EGameFlowState::Paused);
}

void UGameFlowSubsystem::ResumeGame()
{
    if (CurrentState != EGameFlowState::Paused)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0);

    if (!PlayerController)
    {
        return;
    }

    PlayerController->SetPause(false);

    FInputModeGameOnly InputMode;
    PlayerController->SetInputMode(InputMode);
    PlayerController->SetShowMouseCursor(false);

    ChangeState(EGameFlowState::Playing);
}

void UGameFlowSubsystem::HandlePlayerDeath()
{
    if (CurrentState != EGameFlowState::Playing)
    {
        return;
    }

    ChangeState(EGameFlowState::GameOver);
}

void UGameFlowSubsystem::HandleLevelCompleted()
{
    if (CurrentState != EGameFlowState::Playing)
    {
        return;
    }

    ChangeState(EGameFlowState::LevelCompleted);
}

bool UGameFlowSubsystem::ReturnToMainMenu()
{
    ChangeState(EGameFlowState::Loading);

    const UCustomGameInstance* FPSGameInstance = Cast<UCustomGameInstance>(GetGameInstance());

    if (!IsValid(FPSGameInstance))
    {
        UE_LOG(LogGameFlowSubsystem, Error, TEXT("ReturnToMainMenu failed: ""GameInstance is not UFPSGameInstance."));

        return false;
    }

    return OpenLevel(FPSGameInstance->GetMainMenuLevel());
}

void UGameFlowSubsystem::QuitGame()
{
    UWorld* World = GetWorld();

    if (!IsValid(World))
    {
        UE_LOG(
            LogGameFlowSubsystem,
            Error,
            TEXT("QuitGame failed: World is invalid.")
        );

        return;
    }

    APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0);

    UKismetSystemLibrary::QuitGame(
        World,
        PlayerController,
        EQuitPreference::Quit,
        false
    );
}

void UGameFlowSubsystem::ChangeState(EGameFlowState NewState)
{
    if (CurrentState == NewState)
    {
        return;
    }

    const EGameFlowState PreviousState = CurrentState;
    CurrentState = NewState;

    OnGameFlowStateChanged.Broadcast(PreviousState, CurrentState);
}

//void UGameFlowSubsystem::OpenLevel(FName LevelName)
//{
//    if (LevelName.IsNone())
//    {
//        return;
//    }
//
//    UGameplayStatics::OpenLevel(GetWorld(), LevelName);
//}

bool UGameFlowSubsystem::OpenLevel(const TSoftObjectPtr<UWorld>& LevelAsset)
{
    if (bLevelTransitionInProgress)
    {
        UE_LOG(
            LogGameFlowSubsystem,
            Warning,
            TEXT(
                "Level transition request ignored: "
                "another transition is already in progress."
            )
        );

        return false;
    }

    if (LevelAsset.IsNull())
    {
        UE_LOG(
            LogGameFlowSubsystem,
            Error,
            TEXT("OpenLevel failed: Level asset is not configured.")
        );

        return false;
    }

    const FSoftObjectPath LevelPath = LevelAsset.ToSoftObjectPath();

    /*
     * /Game/Maps/Stage01.Stage01 같은 경로에서
     * Stage01을 가져옵니다.
     */
    const FName LevelName = LevelPath.GetAssetFName();

    if (LevelName.IsNone())
    {
        UE_LOG(
            LogGameFlowSubsystem,
            Error,
            TEXT(
                "OpenLevel failed: "
                "Could not resolve level name from '%s'."
            ),
            *LevelPath.ToString()
        );

        return false;
    }

    UWorld* World = GetWorld();

    if (!IsValid(World))
    {
        return false;
    }

    bLevelTransitionInProgress = true;
    ChangeState(EGameFlowState::Loading);

    UE_LOG(
        LogGameFlowSubsystem,
        Log,
        TEXT("Opening level: %s"),
        *LevelName.ToString()
    );

    UGameplayStatics::OpenLevel(World, LevelName);

    return true;
}

void UGameFlowSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
    if (!IsValid(LoadedWorld)) { return; }

    bLevelTransitionInProgress = false;

    const UCustomGameInstance* FPSGameInstance = Cast<UCustomGameInstance>(GetGameInstance());

    if (!IsValid(FPSGameInstance)) { return; }

    const FName LoadedLevelName = FName(*UGameplayStatics::GetCurrentLevelName(LoadedWorld, true));

    const FName MainMenuLevelName = FPSGameInstance->GetMainMenuLevel().ToSoftObjectPath().GetAssetFName();

    if (LoadedLevelName == MainMenuLevelName)
    {
        ChangeState(EGameFlowState::MainMenu);
    }
    else
    {
        ChangeState(EGameFlowState::Playing);
    }
}