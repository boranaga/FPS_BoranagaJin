#pragma once

#include "CoreMinimal.h"
#include "UI/BaseUIWidget.h"
#include "GameResultLogWidget.generated.h"

class UScrollBox;
class UButton;
class UGameResultEntryWidget;

UCLASS()
class FPS_BORANAGAJIN_API UGameResultLogWidget : public UBaseUIWidget
{
	GENERATED_BODY()
protected:
	virtual void NativeConstruct() override;
public:
	virtual EUIType GetUIType() const override { return EUIType::GameResultLog; }
private:
	void RefreshGameResults();

	UFUNCTION()
	void HandleBackClicked();

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> ScrollBox_GameResults;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Back;

	UPROPERTY(EditDefaultsOnly, Category = "GameResult")
	TSubclassOf<UGameResultEntryWidget> GameResultEntryWidgetClass;
};