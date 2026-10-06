#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FPSPlayerController.generated.h"

class UUIManagerComponent;

class UPauseMenuWidget;
class UGameOverWidget;
class UHealthWidget;
class UStaminaWidget;
class UPlayerDisplayWidget; //Inventory Widget
class UThrowableWeaponInventoryWidget;
class UInteractionWidget;

class UInputAction;
class UInputMappingContext;

UCLASS(abstract, config = "Game")
class FPS_BORANAGAJIN_API AFPSPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	AFPSPlayerController();
protected:
	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI", meta = (AllowPrivateAccess = "true"))
	//UUIManagerComponent* UIManagerComponent;

	UPROPERTY(EditAnywhere, Category = "Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* aPawn) override;
	virtual void SetupInputComponent() override;

//public:
	//UUIManagerComponent* GetUIManager() const { return UIManagerComponent; }

public:
	//void InitUIManager(); //MEMO: Legacy
	void InitPlayerUISubsystem();

private:
	void HandlePauseInput();
	void OpenInventory();
	void CloseInventory();
	void OnTabToggled();
	void OpenThrowableWeaponInventory();
	void CloseThrowableWeaponInventory();

private:
	bool bIsInventoryOpened = false;

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_Pause;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_Tab = nullptr;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_V = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UPauseMenuWidget> PauseMenuWidgetClass;
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UGameOverWidget> GameOverWidgetClass;

	//-----------------------------------------
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UHealthWidget> HealthBarWidgetClass;
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UStaminaWidget> StaminaWidgetClass;
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UInteractionWidget> InteractionWidgetClass;
	UPROPERTY(EditDefaultsOnly, Category = "UI") //MEMO: Inventory
	TSubclassOf<UPlayerDisplayWidget> PlayerDisplayWidgetClass;
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UThrowableWeaponInventoryWidget> ThrowableWeaponInventoryWidgetClass;
};

