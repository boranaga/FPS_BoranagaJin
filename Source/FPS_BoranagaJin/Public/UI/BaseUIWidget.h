#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UIType.h"
#include "BaseUIWidget.generated.h"

class UUIManagerComponent;

UCLASS()
class FPS_BORANAGAJIN_API UBaseUIWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void SetUIType(EUIType NewUIType = EUIType::Base);
	virtual EUIType GetUIType() const { return EUIType::Base; }
protected:
	EUIType UIType = EUIType::Base;
public:
	void SetOwnerUIManager(UUIManagerComponent* InUIManager) { OwnerUIManager = InUIManager; }
	UUIManagerComponent* GetOwnerUIManager() const { return OwnerUIManager; }
protected:
	UUIManagerComponent* OwnerUIManager = nullptr;
};
