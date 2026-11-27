#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FPSPlayerController.generated.h"

class UPauseMenuWidget;
class UGameOverWidget;
class UHealthWidget;
class UStaminaWidget;
class UPlayerDisplayWidget; //Inventory Widget
class UThrowableWeaponInventoryWidget;
class UQuickSlotWidget;
class UInteractionWidget;
class ACharacterPlayer;

class UInputAction;
class UInputMappingContext;

UCLASS(abstract, config = "Game")
class FPS_BORANAGAJIN_API AFPSPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	AFPSPlayerController();
protected:
	UPROPERTY(EditAnywhere, Category = "Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* aPawn) override;
	virtual void OnUnPossess() override;
	virtual void OnRep_Pawn() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupInputComponent() override;
public:
	//void InitUIManager(); //MEMO: Legacy
	void InitPlayerUISubsystem();
	void ReleasePlayerUISubsystem(const ACharacterPlayer* ExpectedCharacter);

private:
	void HandlePauseInput();
	void OpenInventory();
	void CloseInventory();
	void OnTabToggled();
	void OpenThrowableWeaponInventory();
	void CloseThrowableWeaponInventory();

private:
	bool bIsInventoryOpened = false;
	TWeakObjectPtr<ACharacterPlayer> InitializedUICharacter;

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
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UQuickSlotWidget> QuickSlotWidgetClass;
};

