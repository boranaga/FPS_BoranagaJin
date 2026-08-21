#include "UI/MapSelectEntryWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"

void UMapSelectEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(Button_Map))
	{
		Button_Map->OnClicked.RemoveDynamic(this, &UMapSelectEntryWidget::HandleMapButtonClicked);
		Button_Map->OnClicked.AddDynamic(this, &UMapSelectEntryWidget::HandleMapButtonClicked);
	}
}

void UMapSelectEntryWidget::NativeDestruct()
{
	if (IsValid(Button_Map))
	{
		Button_Map->OnClicked.RemoveDynamic(this, &UMapSelectEntryWidget::HandleMapButtonClicked);
	}

	Super::NativeDestruct();
}

void UMapSelectEntryWidget::InitializeMap(const FPlayableMapInfo& InMapInfo)
{
	MapInfo = InMapInfo;

	if (IsValid(Text_MapName))
	{
		Text_MapName->SetText(MapInfo.DisplayName);
	}
}

void UMapSelectEntryWidget::HandleMapButtonClicked()
{
	OnMapSelected.Broadcast(MapInfo);
}