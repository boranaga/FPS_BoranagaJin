#include "GameFlowSubsystem.h"
#include "Instance/DefaultGameInstance.h"
#include "SaveSystem/SaveGameSubsystem.h"

#include "Engine/World.h"
#include "Engine/GameInstance.h"
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

//bool UGameFlowSubsystem::StartNewGame()
//{
//    ChangeState(EGameFlowState::Loading);
//
//    const UCustomGameInstance* FPSGameInstance = Cast<UCustomGameInstance>(GetGameInstance());
//
//    if (!IsValid(FPSGameInstance))
//    {
//        UE_LOG(LogGameFlowSubsystem, Error, TEXT("StartNewGame failed: ""GameInstance is not UFPSGameInstance."));
//
//        return false;
//    }
//
//     USaveGameSubsystem* SaveSubsystem = GetGameInstance()->GetSubsystem<USaveGameSubsystem>();
//     SaveSubsystem->StartNewGame();
//
//    return OpenLevel(FPSGameInstance->GetFirstGameLevel());
//}

bool UGameFlowSubsystem::StartNewGame(const TSoftObjectPtr<UWorld>& LevelAsset)
{
    if (LevelAsset.IsNull())
    {
        UE_LOG(LogGameFlowSubsystem, Error, TEXT("StartNewGame failed: LevelAsset is invalid."));
        return false;
    }

    UGameInstance* GameInstance = GetGameInstance();

    if (!IsValid(GameInstance)) { return false; }

    USaveGameSubsystem* SaveSubsystem = GameInstance->GetSubsystem<USaveGameSubsystem>();

    if (!IsValid(SaveSubsystem))
    {
        UE_LOG(LogGameFlowSubsystem, Error, TEXT("StartNewGame failed: SaveGameSubsystem is invalid."));
        return false;
    }

    if (!SaveSubsystem->StartNewGame())
    {
        UE_LOG(LogGameFlowSubsystem, Error, TEXT("StartNewGame failed: Could not create new save slot."));
        return false;
    }

    ChangeState(EGameFlowState::Loading);

    if (!OpenLevel(LevelAsset))
    {
        bPendingInitialSave = false;
        return false;
    }

    return true;
}


void UGameFlowSubsystem::ContinueGame()
{
    // SaveGameSubsystem에서 마지막 저장 데이터를 읽은 뒤
    // 저장된 레벨로 이동하도록 구현합니다.
}

//bool UGameFlowSubsystem::RestartFromCheckPoint()
//{
//    UGameInstance* GameInstance = GetGameInstance();
//    if (!IsValid(GameInstance)) { return false; }
//
//    USaveGameSubsystem* SaveSubsystem = GameInstance->GetSubsystem<USaveGameSubsystem>();
//    if (!IsValid(SaveSubsystem)) { return false; }
//
//    if (!SaveSubsystem->LoadGame()) { return false; }
//
//    return SaveSubsystem->OpenSavedLevel();
//}

//bool UGameFlowSubsystem::LoadGameFromSlot(int32 SlotIndex)
//{
//    if (SlotIndex <= 0) { return false; }
//    USaveGameSubsystem* SaveGameSubsystem = GetGameInstance()->GetSubsystem<USaveGameSubsystem>();
//    if (!IsValid(SaveGameSubsystem)) { return false; }
//    return SaveGameSubsystem->LoadGameFromSlot(SlotIndex);
//}

bool UGameFlowSubsystem::LoadGameFromSlot(const FString& SlotName)
{
    UGameInstance* GameInstance = GetGameInstance();
    if (!IsValid(GameInstance)) { return false; }

    USaveGameSubsystem* SaveSubsystem = GameInstance->GetSubsystem<USaveGameSubsystem>();
    if (!IsValid(SaveSubsystem)) { return false; }

    if (!SaveSubsystem->LoadGameFromSlot(SlotName))
    {
        return false;
    }

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

void UGameFlowSubsystem::SaveAndQuitGame()
{
    UGameInstance* GameInstance = GetGameInstance();
    if (!IsValid(GameInstance)) { return; }
    USaveGameSubsystem* SaveGameSubsystem = GameInstance->GetSubsystem<USaveGameSubsystem>();
    if (!IsValid(SaveGameSubsystem))
    {
        UE_LOG(LogTemp, Warning, TEXT("SaveAndQuitGame: Invalid SaveGameSubsystem"));
        QuitGame();
        return;
    }

    if (!SaveGameSubsystem->HasCurrentSaveSlot())
    {
        UE_LOG(LogTemp, Warning, TEXT("SaveAndQuitGame: Current save slot does not exist."));
        QuitGame();
        return;
    }

    const bool bSaved = SaveGameSubsystem->SaveGameSync();

    if (!bSaved)
    {
        UE_LOG(LogTemp, Error, TEXT("SaveAndQuitGame: Failed to save game."));
        return;
    }

    QuitGame();
}

void UGameFlowSubsystem::QuitGame()
{
    //// <Old Version>

    //UWorld* World = GetWorld();

    //if (!IsValid(World))
    //{
    //    UE_LOG(
    //        LogGameFlowSubsystem,
    //        Error,
    //        TEXT("QuitGame failed: World is invalid.")
    //    );

    //    return;
    //}

    //APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0);

    //UKismetSystemLibrary::QuitGame(
    //    World,
    //    PlayerController,
    //    EQuitPreference::Quit,
    //    false
    //);

    //----------------------------------------
    //// <New Version>

    //UGameInstance* GameInstance = GetGameInstance();

    //if (!IsValid(GameInstance)) { return; }

    //USaveGameSubsystem* SaveSubsystem = GameInstance->GetSubsystem<USaveGameSubsystem>();

    //if (IsValid(SaveSubsystem) && SaveSubsystem->HasCurrentSaveSlot())
    //{
    //    SaveSubsystem->SaveGameSync();
    //}

    //APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);

    //if (!IsValid(PlayerController)) { return; }

    //UKismetSystemLibrary::QuitGame(GetWorld(), PlayerController, EQuitPreference::Quit, false);

    //------------------------------------
    // <New New Version>

    APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);

    if (!IsValid(PlayerController)) { return; }

    UKismetSystemLibrary::QuitGame(GetWorld(), PlayerController, EQuitPreference::Quit, false);

    UE_LOG(LogTemp, Error, TEXT("void UGameFlowSubsystem::QuitGame()"));
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
        bPendingInitialSave = false;
        ChangeState(EGameFlowState::MainMenu);
        return;
    }

    ChangeState(EGameFlowState::Playing);

    if (bPendingInitialSave)
    {
        bPendingInitialSave = false;

        USaveGameSubsystem* SaveSubsystem = GetGameInstance()->GetSubsystem<USaveGameSubsystem>();

        if (IsValid(SaveSubsystem))
        {
            SaveSubsystem->SaveGameAsync();
        }
    }
}