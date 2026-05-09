

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UIManagerComponent.generated.h"

class UWeaponSystemComponent;

class UPlayerHUD;

class UInputAction;
class UEnhancedInputComponent;
//class UBaseUIWidget;

UENUM(BlueprintType)
enum class EUIType : uint8
{
	None UMETA(DisplayName = "None"),
	Inventory UMETA(DisplayName = "Inventory"),
	PlayerHUD UMETA(DisplayName = "PlayerHUD")
};

struct FInputActionValue;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FPS_BORANAGAJIN_API UUIManagerComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UUIManagerComponent();
	// virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	void SetupInput();

public:
	void OpenUI(EUIType UIType);
	//UBaseUIWidget* GetWidget(EUIType UIType);

	void InitializeWidgets();
	//void InitializeManagers(); //TODO: ???

private:
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* ESCAction = nullptr;
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* TabAction = nullptr;
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* RightMouseAction = nullptr;
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* LeftMouseAction = nullptr;


	UPROPERTY(EditDefaultsOnly, Category = "Data")
	UDataTable* DTUISetting = nullptr; //TODO: ???

	//UPROPERTY(EditAnywhere, Category = "UI")
	//TMap<EUIType, TSubclassOf<UBaseUIWidget>> UIWidgetClasses;


	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> TabMenuWidgetClass;

	UPROPERTY()
	UUserWidget* TabMenuWidgetInstance;


	void OnShowTabMenuStarted(const FInputActionValue& Value);
	void OnShowTabMenuCompleted(const FInputActionValue& Value);


	//UPROPERTY()
	//TMap<EUIType, UBaseUIWidget*> UIWidgets;
};
