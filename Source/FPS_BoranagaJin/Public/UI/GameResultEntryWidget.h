#pragma once

#include "CoreMinimal.h"
#include "UI/BaseUIWidget.h"
#include "SaveSystem/GameResultSaveData.h"
#include "GameResultEntryWidget.generated.h"

class UTextBlock;

UCLASS()
class FPS_BORANAGAJIN_API UGameResultEntryWidget : public UBaseUIWidget
{
	GENERATED_BODY()
public:
	virtual EUIType GetUIType() const override { return EUIType::GameResultEntry; }
public:
	void SetGameResult(const FGameResultSaveData& ResultData);
private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Result;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Level;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_PlayTime;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_CompletedAt;
};