#pragma once

#include "CoreMinimal.h"
#include "UI/BaseUIWidget.h"
#include "MainMenuWidget.generated.h"


DECLARE_MULTICAST_DELEGATE(FOnPlayNewGameRequested);
DECLARE_MULTICAST_DELEGATE(FOnContinueRequested);
DECLARE_MULTICAST_DELEGATE(FOnOptionRequested);
DECLARE_MULTICAST_DELEGATE(FOnExitRequested);

class UButton;

UCLASS()
class FPS_BORANAGAJIN_API UMainMenuWidget : public UBaseUIWidget
{
	GENERATED_BODY()
public:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	virtual EUIType GetUIType() const { return EUIType::MainMenu; }
protected:
	UFUNCTION()
	void HandlePlayButtonClicked();
	UFUNCTION()
	void HandleContinueButtonClicked();
	UFUNCTION()
	void HandleOptionButtonClicked();
	UFUNCTION()
	void HandleExitButtonClicked();
public:
	FOnPlayNewGameRequested OnPlayNewGameRequested;
	FOnContinueRequested OnContinueRequested;
	FOnOptionRequested OnOptionRequested;
	FOnExitRequested OnExitRequested;
protected:
	UPROPERTY(EditAnyWhere, BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UButton> Button_PlayNewGame;
	UPROPERTY(EditAnyWhere, BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UButton> Button_Continue;
	UPROPERTY(EditAnyWhere, BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UButton> Button_Option;
	UPROPERTY(EditAnyWhere, BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UButton> Button_Exit;
};

