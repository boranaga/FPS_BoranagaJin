#pragma once

#include "CoreMinimal.h"
//#include "Subsystems/GameInstanceSubsystem.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "UI/UIType.h"
#include "PlayerUISubsystem.generated.h"

class UBaseUIWidget;
class UMainMenuWidget;
class AFPSPlayerController;
class ACharacterPlayer;

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
        case EUIType::Inventory: return 3;
        case EUIType::ThrowableWeaponInventory: return 3;
        case EUIType::Base: return 4;
        default: return 0;
        }
    }

    //void InitializeUILayers();

    void BindCharacterDelegates();
    void UnbindCharacterDelegates();

public:
    void InitMainMenuUI(TSubclassOf<UMainMenuWidget> WidgetClass);
    void InitGameplayUI();
private:
    void HandlePlayRequested();
    void HandleContinueRequested();
    void HandleOptionRequested();
    void HandleExitRequested();
private:
    //TODO: Weapon Aim UI는 같은 Type으로 여러개 존재하는데 이에 대한 처리 필요
    //UPROPERTY()
    //TMap<EUIType, TObjectPtr<UBaseUIWidget>> UIWidgets_Legacy;

    UPROPERTY()
    TMap<EUIType, FUIWidgetArray> UIWidgets;


    //UPROPERTY()
    //TMap<EUIType, int32> UILayers;
    UPROPERTY()
    TObjectPtr<ACharacterPlayer> CharacterPlayer;
};