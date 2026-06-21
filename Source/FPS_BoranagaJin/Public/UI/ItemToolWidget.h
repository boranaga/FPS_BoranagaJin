// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/BaseUIWidget.h"
#include "ItemToolWidget.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnToolWidgetEnter);
DECLARE_MULTICAST_DELEGATE(FOnToolWidgetLeave);

class UBorder;

UCLASS()
class FPS_BORANAGAJIN_API UItemToolWidget : public UBaseUIWidget
{
	GENERATED_BODY()
public:
	FOnToolWidgetEnter OnToolWidgetEnter;
	FOnToolWidgetLeave OnToolWidgetLeave;

	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	//virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime);
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
public:
	UPROPERTY(meta = (BindWidget))
	UBorder* Border_ItemTool;
protected:
	bool bMouseHovered = false;
public:
	bool IsHoveredToolWidget() const { return bMouseHovered; }

protected:
	FName ItemName;
public:
	void SetItemToolPosition();
protected:
	void DisplayItemData();
};
