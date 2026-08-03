#pragma once

#include "CoreMinimal.h"
#include "UI/BaseUIWidget.h"
#include "MainMenuWidget.generated.h"


DECLARE_MULTICAST_DELEGATE(FOnPlayRequested);
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

	virtual EUIType GetUIType() const { return EUIType::UIType_MainMenu; }
protected:
	UFUNCTION()
	void HandlePlayButtonClicked();
	UFUNCTION()
	void HandleExitButtonClicked();
public:
	FOnPlayRequested OnPlayRequested;
	FOnExitRequested OnExitRequested;
protected:
	UPROPERTY(EditAnyWhere, BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UButton> Button_Play;
	UPROPERTY(EditAnyWhere, BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UButton> Button_Option;
	UPROPERTY(EditAnyWhere, BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UButton> Button_Exit;
};

