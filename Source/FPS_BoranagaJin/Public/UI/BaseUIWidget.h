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
	void SetUIType(EUIType NewUIType = EUIType::UIType_Base);
	virtual EUIType GetUIType() const { return EUIType::UIType_Base; }
protected:
	EUIType UIType = EUIType::UIType_Base;
public:
	void SetOwnerUIManager(UUIManagerComponent* InUIManager) { OwnerUIManager = InUIManager; }
	UUIManagerComponent* GetOwnerUIManager() const { return OwnerUIManager; }
protected:
	UUIManagerComponent* OwnerUIManager = nullptr;
};
