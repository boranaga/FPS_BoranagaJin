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
        case EUIType::UIType_Stamina: return 0;
        case EUIType::UIType_Health: return 0;
        case EUIType::UIType_Interaction: return 1;
        case EUIType::UIType_WeaponAim: return 2;
        case EUIType::UIType_AmmoCounter: return 2;
        case EUIType::UIType_MainMenu: return 3;
        case EUIType::UIType_Inventory: return 3;
        case EUIType::UIType_ThrowableWeaponInventory: return 3;
        case EUIType::UIType_Base: return 4;
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
    void HandleExitRequested();
private:
    //TODO: Weapon Aim UI는 같은 Type으로 여러개 존재하는데 이에 대한 처리 필요
    UPROPERTY()
    TMap<EUIType, TObjectPtr<UBaseUIWidget>> UIWidgets;
    //UPROPERTY()
    //TMap<EUIType, int32> UILayers;
    UPROPERTY()
    TObjectPtr<ACharacterPlayer> CharacterPlayer;
};