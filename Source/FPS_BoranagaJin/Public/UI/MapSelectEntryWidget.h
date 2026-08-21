#pragma once

#include "CoreMinimal.h"
//#include "Blueprint/UserWidget.h"
#include "UI/BaseUIWidget.h"
#include "PlayableMapInfo.h"
#include "MapSelectEntryWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnMapSelected, FPlayableMapInfo);

UCLASS()
class FPS_BORANAGAJIN_API UMapSelectEntryWidget : public UBaseUIWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	void InitializeMap(const FPlayableMapInfo& InMapInfo);

	FOnMapSelected OnMapSelected;
public:
	virtual EUIType GetUIType() const override { return EUIType::MapSelectEntry; }
private:
	UFUNCTION()
	void HandleMapButtonClicked();

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Map;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_MapName;

	FPlayableMapInfo MapInfo;
};