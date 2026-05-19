

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FPSPlayerController.generated.h"

class UUIManagerComponent;

class UInputMappingContext;

UCLASS(abstract, config = "Game")
class FPS_BORANAGAJIN_API AFPSPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	AFPSPlayerController();
protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI", meta = (AllowPrivateAccess = "true"))
	UUIManagerComponent* UIManagerComponent;

	UPROPERTY(EditAnywhere, Category = "Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* aPawn) override;
	virtual void SetupInputComponent() override;

	UUIManagerComponent* GetUIManager() const { return UIManagerComponent; }

public:
	void InitUIManager();
};

