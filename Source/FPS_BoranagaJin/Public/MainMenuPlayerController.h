
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MainMenuPlayerController.generated.h"


class UMainMenuWidget;
class USaveFileSlotMenuWidget;
class UMapSelectMenuWidget;

class UInputMappingContext;

class UInputAction;
class UInputMappingContext;
class UEnhancedInputComponent;

struct FInputActionValue;

UCLASS(abstract, config = "Game")
class FPS_BORANAGAJIN_API AMainMenuPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	AMainMenuPlayerController();
protected:
	virtual void BeginPlay() override;
	//virtual void OnPossess(APawn* aPawn) override;
	virtual void SetupInputComponent() override;
protected:
	UPROPERTY(EditAnywhere, Category = "Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* ESCAction = nullptr;
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* TabAction = nullptr;
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* MouseRightClickAction = nullptr;
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* MouseLeftClickAction = nullptr;

#pragma region UI
	UPROPERTY(EditAnywhere, Category = "MainMenuWidget")
	TSubclassOf<UMainMenuWidget> MainMenuWidgetClass;
	UPROPERTY(EditAnywhere, Category = "MapSelectMenuWidget")
	TSubclassOf<UMapSelectMenuWidget> MapSelectMenuWidgetClass;
	UPROPERTY(EditAnywhere, Category = "SaveFileSlotWidget")
	TSubclassOf<USaveFileSlotMenuWidget> SaveFileSlotWidgetClass;
#pragma endregion
};
