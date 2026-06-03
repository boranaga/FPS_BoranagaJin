#pragma once

#include "CoreMinimal.h"
#include "BaseUIWidget.h"
#include "PlayerDisplayWidget.generated.h"

UCLASS()
class FPS_BORANAGAJIN_API UPlayerDisplayWidget : public UBaseUIWidget
{
	GENERATED_BODY()
public:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual EUIType GetUIType() const { return EUIType::UIType_Inventory; }
public:
	void OpenInventory();
	void CloseInventory();
protected:
	void ShowMouseCursor();
	void HideMouseCursor();
	void DisplayInventory();
};
