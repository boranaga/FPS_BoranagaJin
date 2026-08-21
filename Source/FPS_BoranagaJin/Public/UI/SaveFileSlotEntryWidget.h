#pragma once

#include "CoreMinimal.h"
//#include "Blueprint/UserWidget.h"
#include "UI/BaseUIWidget.h"
#include "SaveSystem/SaveSlotInfo.h"
#include "SaveFileSlotEntryWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnSaveSlotClicked, FString);

UCLASS()
class FPS_BORANAGAJIN_API USaveFileSlotEntryWidget : public UBaseUIWidget
{
	GENERATED_BODY()
public:
	void InitializeSlot(const FSaveSlotInfo& InSlotInfo);
	FOnSaveSlotClicked OnSaveSlotClicked;
public:
	virtual EUIType GetUIType() const override { return EUIType::SaveFileSlotEntry; }
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
private:
	UFUNCTION()
	void HandleLoadButtonClicked();
private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Load;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_LevelName;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_SavedAt;

	FSaveSlotInfo SlotInfo;
};