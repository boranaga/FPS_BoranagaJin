

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UIType.h"
#include "UIManagerComponent.generated.h"

class AFPSPlayerController;
class ACharacterPlayer;
class UBaseUIWidget;
class UStaminaWidget;
class UThrowableWeaponInventoryWidget;

class UPlayerDisplayWidget;
class UInteractionWidget;

class UInventorySystemComponent;

class UPlayerHUD;

class UInputAction;
class UInputMappingContext;
class UEnhancedInputComponent;

struct FInputActionValue;

//DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUIWidgetCreated, UBaseUIWidget*, NewUIWidgetPtr);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FPS_BORANAGAJIN_API UUIManagerComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UUIManagerComponent();
	// virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void InitUIManagerComponent();
protected:
	virtual void BeginPlay() override;
protected:
	UPROPERTY()
	TObjectPtr<ACharacterPlayer> CharacterPlayer = nullptr;
	UPROPERTY()
	TObjectPtr<AFPSPlayerController> PlayerController = nullptr;

public:
	//void OpenUI(EUIType UIType);
	//UBaseUIWidget* GetWidget(EUIType UIType);

	void InitializeWidgets();
	//void InitializeManagers(); //TODO: ???

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* UISystemMappingContext;
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* ESCAction = nullptr;
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* TabAction = nullptr;
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* VAction = nullptr;
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* RightMouseAction = nullptr;
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* LeftMouseAction = nullptr;


	UPROPERTY(EditDefaultsOnly, Category = "Data")
	UDataTable* DTUISetting = nullptr; //TODO: ???

	//UPROPERTY(EditAnywhere, Category = "UI")
	//TMap<EUIType, TSubclassOf<UBaseUIWidget>> UIWidgetClasses;

	void OnShowTabMenuStarted(const FInputActionValue& Value);
	void OnShowTabMenuCompleted(const FInputActionValue& Value);

//----------------------------
protected:
	TTuple<FVector2D, bool> GetScreenPositionOfWorldLocation(const FVector& SearchLocation) const;
	bool IsInViewport(FVector2D ActorScreenPosition, float ScreenRatio_Width = 0.0f, float ScreenRatio_Height = 0.0f) const;
#pragma region UIManagement
public:
	//FOnUIWidgetCreated OnUIWidgetCreatedDelegate;
protected:
	UPROPERTY()
	TMap<EUIType, int32> UILayers;
	UPROPERTY()
	TMap<EUIType, UBaseUIWidget*> UIWidgets;
protected:
	void InitUILayersMap();
public:
	void RegisterUIWidget(UBaseUIWidget* NewUIWidget);
	

#pragma endregion
#pragma region InventoryUI
protected:
	UPROPERTY(EditAnywhere, BlueprintreadWrite, Category = "PlayerDisplayWidget")
	TSubclassOf<UPlayerDisplayWidget> PlayerDisplayWidgetClass;
	UPROPERTY()
	UPlayerDisplayWidget* PlayerDisplayWidget = nullptr;
protected:
	void InitPlayerDisplayWidget();
	void OpenInventory();
	void CloseInventory();
	void OnTabToggled();
protected:
	bool bIsInventoryOpened = false;
public:
	void RequestSwapInventorySlots(FName InventoryName, int32 FromIndex, int32 ToIndex);
	void RequestDropInventorySlot(FName InventoryName, int32 SlotIndex);
#pragma endregion
#pragma region ThrowableWeaponInventory
protected:
	UPROPERTY(EditAnywhere, BlueprintreadWrite, Category = "PlayerDisplayWidget")
	TSubclassOf<UThrowableWeaponInventoryWidget> ThrowableWeaponInventoryWidgetClass;
	UPROPERTY()
	UThrowableWeaponInventoryWidget* ThrowableWeaponInventoryWidget = nullptr;
protected:
	void OpenThrowableWeaponInventory();
	void CloseThrowableWeaponInventory();
#pragma endregion
#pragma region StaminaBar
protected:
	UPROPERTY(EditAnywhere, BlueprintreadWrite, Category = "StaminaWidget")
	TSubclassOf<UStaminaWidget> StaminaWidgetClass;
	UPROPERTY()
	UStaminaWidget* StaminaWidget = nullptr;

	UFUNCTION()
	void InitStaminaBar(float maxstamina = 100.f);
	UFUNCTION()
	void SetStaminaBarPercent(float const Value);
	//void HideInGame(bool bHidden);
	//void PlayFadeAnimation();
#pragma endregion
#pragma region InteractionUI
protected:
	UPROPERTY(EditAnywhere, BlueprintreadWrite, Category = "InteractionWidget")
	TSubclassOf<UInteractionWidget> InteractionWidgetClass;
	UPROPERTY()
	UInteractionWidget* InteractionWidget;
protected:
	UFUNCTION()
	void PlayPopUpInteractionWidgetAnim();
	UFUNCTION()
	void UpdateInteractionUI(bool bFlag = false, FVector NewLocation = FVector::ZeroVector);
#pragma endregion
};
