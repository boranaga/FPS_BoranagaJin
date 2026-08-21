#pragma once

#include "CoreMinimal.h"
#include "UI/BaseUIWidget.h"
#include "PauseMenuWidget.generated.h"

class UButton;

DECLARE_MULTICAST_DELEGATE(FOnPauseMenuPlayRequested);
DECLARE_MULTICAST_DELEGATE(FOnPauseMenuOptionRequested);
DECLARE_MULTICAST_DELEGATE(FOnPauseMenuSaveAndExitRequested);

UCLASS()
class FPS_BORANAGAJIN_API UPauseMenuWidget : public UBaseUIWidget
{
	GENERATED_BODY()

public:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	virtual EUIType GetUIType() const override { return EUIType::PauseMenu; }

public:
	FOnPauseMenuPlayRequested OnPlayRequested;
	FOnPauseMenuOptionRequested OnOptionRequested;
	FOnPauseMenuSaveAndExitRequested OnSaveAndExitRequested;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UButton> Button_Play;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UButton> Button_Option;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UButton> Button_SaveAndExit;

private:
	UFUNCTION()
	void HandlePlayButtonClicked();

	UFUNCTION()
	void HandleOptionButtonClicked();

	UFUNCTION()
	void HandleSaveAndExitButtonClicked();
};
