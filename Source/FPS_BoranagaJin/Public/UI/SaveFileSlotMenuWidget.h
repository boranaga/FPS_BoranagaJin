#pragma once

#include "CoreMinimal.h"
#include "UI/BaseUIWidget.h"
#include "SaveSystem/SaveSlotInfo.h"
#include "SaveFileSlotMenuWidget.generated.h"

class UButton;
class UVerticalBox;
class USaveFileSlotEntryWidget;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnSaveFileSlotSelected, FString);
DECLARE_MULTICAST_DELEGATE(FOnSaveFileSlotBackRequested);

UCLASS()
class FPS_BORANAGAJIN_API USaveFileSlotMenuWidget : public UBaseUIWidget
{
	GENERATED_BODY()
public:
	void RefreshSaveSlots(const TArray<FSaveSlotInfo>& SaveSlots);

	FOnSaveFileSlotSelected OnSaveFileSlotSelected;
	FOnSaveFileSlotBackRequested OnBackRequested;

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;


public:
	virtual EUIType GetUIType() const override { return EUIType::SaveFileSlotMenu; }

private:
#pragma region Button
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> SaveSlotContainer;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Back;

	UPROPERTY(EditDefaultsOnly, Category = "SaveFile")
	TSubclassOf<USaveFileSlotEntryWidget> SaveSlotEntryWidgetClass;
#pragma endregion

#pragma region ButtonHandler
	void HandleSaveSlotClicked(FString SlotName);

	UFUNCTION()
	void HandleBackButtonClicked();
#pragma endregion
};