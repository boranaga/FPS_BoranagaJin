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
	Missing		UMETA(DisplayName = "Missing"),		// 플레이어가 갖고 있지 않음
	Owned		UMETA(DisplayName = "Owned"),		// 인벤토리에 있음 (클릭해서 넣을 수 있음)
	Inserted	UMETA(DisplayName = "Inserted")		// 탈출구에 넣음
};

/** 위젯에 표시할 슬롯 1칸의 정보 */
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

/** 자기 번호를 알려 주는 슬롯 버튼 */
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
 * 탈출구 가까이 가면 화면 가운데에 표시되는 "필요한 아이템" UI.
 *
 * - 회색 반투명 배경 위에 아이템 칸이 나오고, 칸마다 0/1 (넣은 개수/필요 개수)이 표시됩니다.
 * - 플레이어가 갖고 있는 아이템의 칸만 마우스로 클릭할 수 있습니다.
 *
 * WBP 없이 이 클래스만으로 동작합니다. (위젯 트리를 C++에서 직접 만듭니다)
 * 모양을 바꾸고 싶으면 이 클래스를 부모로 WBP를 만들고,
 * TitleText / SlotBox / HintText 이름의 위젯을 배치하면 그 위젯을 대신 사용합니다.
 */
UCLASS()
class FPS_BORANAGAJIN_API UExitDoorWidget : public UBaseUIWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Exit")
	void SetExitData(const FText& Title, const TArray<FExitSlotViewData>& Slots, const FText& Hint, bool bUnlocked);

	/** WBP에서 추가 연출(애니메이션 등)을 하고 싶을 때 사용 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Exit")
	void OnExitDataChanged(const TArray<FExitSlotViewData>& Slots, bool bUnlocked);

	/** 슬롯을 마우스로 클릭했을 때 */
	FOnExitSlotClicked OnSlotClicked;

protected:
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

	/** 회색 반투명 배경 */
	UPROPERTY(EditDefaultsOnly, Category = "Exit|Style")
	FLinearColor BackgroundColor = FLinearColor(0.2f, 0.2f, 0.2f, 0.6f);

	UPROPERTY(EditDefaultsOnly, Category = "Exit|Style")
	FLinearColor MissingColor = FLinearColor(0.12f, 0.12f, 0.12f, 0.9f);

	/** 갖고 있어서 클릭할 수 있는 칸 */
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
