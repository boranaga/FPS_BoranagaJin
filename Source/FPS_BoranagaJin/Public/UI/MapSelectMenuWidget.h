#pragma once

#include "CoreMinimal.h"
#include "UI/BaseUIWidget.h"
#include "PlayableMapInfo.h"
#include "MapSelectMenuWidget.generated.h"

class UButton;
class UVerticalBox;
class UMapSelectEntryWidget;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnPlayableMapSelected, FPlayableMapInfo);
DECLARE_MULTICAST_DELEGATE(FOnMapSelectBackRequested);

UCLASS()
class FPS_BORANAGAJIN_API UMapSelectMenuWidget : public UBaseUIWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	virtual EUIType GetUIType() const override { return EUIType::MapSelectMenu; }

	void RefreshPlayableMaps(const TArray<FPlayableMapInfo>& PlayableMaps);

	FOnPlayableMapSelected OnPlayableMapSelected;
	FOnMapSelectBackRequested OnBackRequested;

private:
	void HandleMapSelected(FPlayableMapInfo MapInfo);

	UFUNCTION()
	void HandleBackButtonClicked();

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> MapContainer;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Back;

	UPROPERTY(EditDefaultsOnly, Category = "Map")
	TSubclassOf<UMapSelectEntryWidget> MapEntryWidgetClass;
};