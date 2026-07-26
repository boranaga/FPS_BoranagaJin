// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/BaseUIWidget.h"
#include "ItemToolWidget.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnToolWidgetEnter);
DECLARE_MULTICAST_DELEGATE(FOnToolWidgetLeave);

DECLARE_MULTICAST_DELEGATE(FOnUseItemRequested);
DECLARE_MULTICAST_DELEGATE(FOnDropItemRequested);

class UItemActionWidget;

class UBorder;

UCLASS()
class FPS_BORANAGAJIN_API UItemToolWidget : public UBaseUIWidget
{
	GENERATED_BODY()
public:
	FOnToolWidgetEnter OnToolWidgetEnter;
	FOnToolWidgetLeave OnToolWidgetLeave;

	FOnUseItemRequested OnUseItemRequested;
	FOnDropItemRequested OnDropItemRequested;

	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	//virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime);
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	//void SetInventoryIndex(int32 NewIndex);
protected:
	void OnUseClicked();
	void OnDropClicked();
public:
	UPROPERTY(meta = (BindWidget))
	UBorder* Border_ItemTool;
	UPROPERTY(meta = (BindWidget))
	UItemActionWidget* ItemAction_Use;
	UPROPERTY(meta = (BindWidget))
	UItemActionWidget* ItemAction_Drop;
protected:
	bool bMouseHovered = false;
public:
	bool IsHoveredToolWidget() const { return bMouseHovered; }

protected:
	FName ItemName;
	//int32 InventoryIndex = INDEX_NONE;
public:
	void SetItemToolPosition();
protected:
	void DisplayItemData();
};
