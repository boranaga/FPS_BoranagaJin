
#include "UI/ItemActionWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"

void UItemActionWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	SetUIType(EUIType::UIType_Inventory);


	TxtTextBlock->SetText(ActionText);

	ButtonItemAction->OnHovered.AddDynamic(this, &UItemActionWidget::OnButtonHovered);
}

void UItemActionWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UItemActionWidget::OnButtonHovered()
{
	
	//TxtTextBlock->SetColorAndOpacity();
}
