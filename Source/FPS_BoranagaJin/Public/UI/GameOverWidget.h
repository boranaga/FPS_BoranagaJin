#pragma once

#include "CoreMinimal.h"
#include "UI/BaseUIWidget.h"
//#include "GameFlow/GameEndReason.h"
#include "GameOverWidget.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnMainMenuRequested);
DECLARE_MULTICAST_DELEGATE(FOnExitRequested);

class UButton;
class UTextBlock;

UCLASS()
class FPS_BORANAGAJIN_API UGameOverWidget : public UBaseUIWidget
{
	GENERATED_BODY()
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

public:
	virtual EUIType GetUIType() const override { return EUIType::GameOver; }
	//void SetGameEndReason(EGameEndReason EndReason); //TODO: GameFlowSystem에서 요청(처리)하도록 해야함

public:
	FOnMainMenuRequested OnMainMenuRequested;
	FOnExitRequested OnExitRequested;
private:
	void UpdateGameOverText();
	//UFUNCTION()
	//void HandleRetryClicked();
	UFUNCTION()
	void HandleMainMenuClicked();
	UFUNCTION()
	void HandleExitClicked();
private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Result;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Description;
	//UPROPERTY(meta = (BindWidget))
	//TObjectPtr<UButton> Button_Retry;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_MainMenu;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Exit;

private:
	//EGameEndReason CurrentEndReason = EGameEndReason::None;
};