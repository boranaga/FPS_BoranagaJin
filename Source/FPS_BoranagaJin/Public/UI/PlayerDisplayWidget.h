#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerDisplayWidget.generated.h"

UCLASS()
class FPS_BORANAGAJIN_API UPlayerDisplayWidget : public UUserWidget
{
	GENERATED_BODY()
public:

public:
	void OpenInventory();
	void CloseInventory();
protected:
	void ShowMouseCursor();
	void HideMouseCursor();
	void DisplayInventory();
};
