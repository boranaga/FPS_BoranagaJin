#pragma once

#include "CoreMinimal.h"
#include "BaseUIWidget.h"
#include "Components/Button.h"
#include "ExitDoorWidget.generated.h"

class UBorder;
class UHorizontalBox;
class UImage;
class UTextBlock;
class UTexture2D;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnExitSlotClicked, int32 /*SlotIndex*/);

UENUM(BlueprintType)
enum class EExitSlotState : uint8
{
	Missing		UMETA(DisplayName = "Missing"),		// Not present in the player inventory
	Owned		UMETA(DisplayName = "Owned"),		// Owned and available for insertion
	Inserted	UMETA(DisplayName = "Inserted")		// Already inserted into the exit
};

/** View data for one required-item slot. */
USTRUCT(BlueprintType)
struct FPS_BORANAGAJIN_API FExitSlotViewData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exit")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exit")
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exit")
	EExitSlotState State = EExitSlotState::Missing;
};

/** Slot button that reports its own index. */
UCLASS()
class FPS_BORANAGAJIN_API UExitSlotButton : public UButton
{
	GENERATED_BODY()
public:
	void InitSlot(int32 InSlotIndex);

	FOnExitSlotClicked OnSlotButtonClicked;

private:
	UFUNCTION()
	void HandleClicked();

	int32 SlotIndex = INDEX_NONE;
};

/**
 * Required-item UI opened by interaction while the player is near an exit door.
 *
 * - Each slot shows inserted/required count (0/1 or 1/1).
 * - Only owned items can be clicked for insertion.
 * - The fallback WidgetTree is created in C++ if no WBP is assigned.
 * - WBP subclasses can provide TitleText, SlotBox, and HintText widgets.
 * - UPlayerUISubsystem owns one active instance shared across nearby exit doors.
 */
UCLASS()
class FPS_BORANAGAJIN_API UExitDoorWidget : public UBaseUIWidget
{
	GENERATED_BODY()
public:
	/** Classifies this widget for UPlayerUISubsystem registration and Z-order. */
	virtual EUIType GetUIType() const override { return EUIType::ExitDoor; }

	UFUNCTION(BlueprintCallable, Category = "Exit")
	void SetExitData(const FText& Title, const TArray<FExitSlotViewData>& Slots, const FText& Hint, bool bUnlocked);

	/** Optional Blueprint presentation event. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Exit")
	void OnExitDataChanged(const TArray<FExitSlotViewData>& Slots, bool bUnlocked);

	/** Raised when an item slot is clicked. */
	FOnExitSlotClicked OnSlotClicked;

protected:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void BuildDefaultTree();
	void RebuildSlotEntries(int32 SlotCount);
	void HandleSlotButtonClicked(int32 SlotIndex);
	FLinearColor GetStateColor(EExitSlotState State) const;

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> SlotBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HintText;

	/** Semi-transparent panel background. */
	UPROPERTY(EditDefaultsOnly, Category = "Exit|Style")
	FLinearColor BackgroundColor = FLinearColor(0.2f, 0.2f, 0.2f, 0.6f);

	UPROPERTY(EditDefaultsOnly, Category = "Exit|Style")
	FLinearColor MissingColor = FLinearColor(0.12f, 0.12f, 0.12f, 0.9f);

	/** Available item slot. */
	UPROPERTY(EditDefaultsOnly, Category = "Exit|Style")
	FLinearColor OwnedColor = FLinearColor(0.85f, 0.85f, 0.85f, 1.f);

	UPROPERTY(EditDefaultsOnly, Category = "Exit|Style")
	FLinearColor InsertedColor = FLinearColor(0.1f, 0.5f, 0.15f, 1.f);

	UPROPERTY(EditDefaultsOnly, Category = "Exit|Style")
	FText RequiredLabelText = FText::FromString(TEXT("필요한 아이템"));

	UPROPERTY(EditDefaultsOnly, Category = "Exit|Style")
	FVector2D SlotSize = FVector2D(130.f, 130.f);

	UPROPERTY(EditDefaultsOnly, Category = "Exit|Style")
	FVector2D IconSize = FVector2D(48.f, 48.f);

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UExitSlotButton>> SlotButtons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> SlotIcons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> SlotNameTexts;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> SlotCountTexts;
};
