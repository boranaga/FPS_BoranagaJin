
#include "UI/ItemActionWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"

void UItemActionWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	SetUIType(EUIType::Inventory);


	TxtTextBlock->SetText(ActionText);

	if (ButtonItemAction)
	{
		ButtonItemAction->OnHovered.AddDynamic(this, &UItemActionWidget::OnButtonHovered);
		ButtonItemAction->OnClicked.AddDynamic(this, &UItemActionWidget::OnButtonClicked);
	}
}

void UItemActionWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UItemActionWidget::OnButtonHovered()
{
	
	//TxtTextBlock->SetColorAndOpacity();
}

void UItemActionWidget::OnButtonClicked()
{
	UE_LOG(LogTemp, Error, TEXT("void UItemActionWidget::OnButtonClicked()"));

	OnItemActionClicked.Broadcast();
}
