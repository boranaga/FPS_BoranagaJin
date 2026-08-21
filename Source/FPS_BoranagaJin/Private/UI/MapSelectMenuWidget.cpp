#include "UI/MapSelectMenuWidget.h"
#include "UI/MapSelectEntryWidget.h"

#include "Components/Button.h"
#include "Components/VerticalBox.h"

void UMapSelectMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(Button_Back))
	{
		Button_Back->OnClicked.RemoveDynamic(this, &UMapSelectMenuWidget::HandleBackButtonClicked);
		Button_Back->OnClicked.AddDynamic(this, &UMapSelectMenuWidget::HandleBackButtonClicked);
	}
}

void UMapSelectMenuWidget::NativeDestruct()
{
	if (IsValid(Button_Back))
	{
		Button_Back->OnClicked.RemoveDynamic(this, &UMapSelectMenuWidget::HandleBackButtonClicked);
	}

	Super::NativeDestruct();
}

void UMapSelectMenuWidget::RefreshPlayableMaps(const TArray<FPlayableMapInfo>& PlayableMaps)
{
	if (!IsValid(MapContainer)) { return; }
	if (!MapEntryWidgetClass) { return; }

	MapContainer->ClearChildren();

	for (const FPlayableMapInfo& MapInfo : PlayableMaps)
	{
		if (!MapInfo.IsValid()) { continue; }

		UMapSelectEntryWidget* EntryWidget = CreateWidget<UMapSelectEntryWidget>(GetOwningPlayer(), MapEntryWidgetClass);

		if (!IsValid(EntryWidget)) { continue; }

		EntryWidget->InitializeMap(MapInfo);
		EntryWidget->OnMapSelected.AddUObject(this, &UMapSelectMenuWidget::HandleMapSelected);

		MapContainer->AddChild(EntryWidget);
	}
}

void UMapSelectMenuWidget::HandleMapSelected(FPlayableMapInfo MapInfo)
{
	OnPlayableMapSelected.Broadcast(MapInfo);
}

void UMapSelectMenuWidget::HandleBackButtonClicked()
{
	OnBackRequested.Broadcast();
}