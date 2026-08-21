#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameFlowState.h"
#include "GameFlowSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnGameFlowStateChanged, EGameFlowState,EGameFlowState);

UCLASS()
class FPS_BORANAGAJIN_API UGameFlowSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
public:
    //TODO: Map Load와 관련해서 동기/비동기 설정하기. DataTable로 관리하기
    bool StartNewGame(const TSoftObjectPtr<UWorld>& LevelAsset);
    void ContinueGame();

    //bool RestartFromCheckPoint();
    bool LoadGameFromSlot(const FString& SlotName);

    void PauseGame();
    void ResumeGame();

    void HandlePlayerDeath();
    void HandleLevelCompleted();

    bool ReturnToMainMenu();

    void SaveAndQuitGame();

    void QuitGame();

    EGameFlowState GetCurrentState() const { return CurrentState; }
    bool IsPlaying() const
    {
        return CurrentState == EGameFlowState::Playing;
    }
public:
    FOnGameFlowStateChanged OnGameFlowStateChanged;
private:
    void ChangeState(EGameFlowState NewState);
    //void OpenLevel(FName LevelName);
    bool OpenLevel(const TSoftObjectPtr<UWorld>& LevelAsset);
private:
    EGameFlowState CurrentState = EGameFlowState::None;

    bool bLevelTransitionInProgress = false;

private:
    void HandlePostLoadMap(UWorld* LoadedWorld);

private:
    FDelegateHandle PostLoadMapDelegateHandle;
private:
    bool bPendingInitialSave = false;
};