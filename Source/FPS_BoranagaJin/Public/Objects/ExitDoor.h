#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/InteractableInterface.h"
#include "ExitDoor.generated.h"

class AExitDoor;
class ACharacterPlayer;
class ADestructibleDoor;
class AMapEscapeTrigger;
class APlayerController;
class UBoxComponent;
class UExitDoorWidget;
class UShapeComponent;
class UStaticMeshComponent;
class UTexture2D;
struct FInventorySlot;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnExitDoorUnlocked, AExitDoor*, ExitDoor);

/** 탈출구에 넣어야 하는 아이템 1칸 */
USTRUCT(BlueprintType)
struct FPS_BORANAGAJIN_API FExitRequirementSlot
{
	GENERATED_BODY()

	/** 아이템 BP의 ItemID 와 같아야 합니다. (KeyCard / Battery / Fuse / Chip) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exit")
	FName ItemID = NAME_None;

	/** UI에 표시할 이름 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exit")
	FText DisplayName;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Exit")
	bool bInserted = false;
};

/**
 * 아이템 3개를 넣어야 열리는 탈출구.
 *
 * - ExitNumber(1~4)를 정하면 필요한 아이템 3개가 자동으로 채워집니다.
 *     1번: 키카드 / 배터리 / 퓨즈   (칩 X)
 *     2번: 배터리 / 퓨즈 / 칩       (키카드 X)
 *     3번: 퓨즈 / 칩 / 키카드       (배터리 X)
 *     4번: 칩 / 키카드 / 배터리     (퓨즈 X)
 * - 플레이어가 DoorVolume에서 ShowDistance(기본 1m) 안에 들어오면 화면에 UI가 표시됩니다.
 * - 갖고 있는 아이템의 칸은 마우스로 클릭해서 넣을 수 있습니다. (그때만 마우스 커서가 나타납니다)
 * - 3개가 모두 들어가면 LinkedDoor가 열리고 LinkedEscapeTrigger가 켜집니다.
 *
 * 기존 클래스(인벤토리, 문, 탈출 트리거)는 수정하지 않고 공개된 함수만 사용합니다.
 * - 아이템은 EItemName이 아니라 ItemID(FName)로 구분합니다.
 * - 잠겨 있는 동안에는 DoorVolume이 상호작용 트레이스를 대신 받아서 문을 직접 열 수 없게 합니다.
 */
UCLASS()
class FPS_BORANAGAJIN_API AExitDoor : public AActor, public IInteractableInterface
{
	GENERATED_BODY()
public:
	AExitDoor();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:
	/** ExitNumber에 맞는 기본 아이템 3개로 RequiredSlots를 다시 채웁니다. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Exit")
	void ApplyExitPreset();

	UFUNCTION(BlueprintPure, Category = "Exit")
	bool IsUnlocked() const { return bUnlocked; }

	UFUNCTION(BlueprintPure, Category = "Exit")
	int32 GetInsertedCount() const;

	UFUNCTION(BlueprintPure, Category = "Exit")
	static FText GetDefaultItemDisplayName(FName ItemID);

	UPROPERTY(BlueprintAssignable, Category = "Exit")
	FOnExitDoorUnlocked OnExitDoorUnlocked;

protected:
	/** 3개가 모두 들어갔을 때 블루프린트에서 추가 연출을 하고 싶을 때 사용 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Exit")
	void OnExitUnlocked();

private:
	/** 틱/타이머 없이, 접근 감지 박스(ProximityVolume)의 오버랩 이벤트로 UI를 켜고 끈다. */
	UFUNCTION()
	void HandleProximityBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void HandleProximityEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void SetNearbyPlayer(ACharacterPlayer* Player);
	void ClearNearbyPlayer();
	/** 인벤토리가 바뀌면 UI를 갱신한다. (가까이 있는 동안만 연결) */
	void HandleInventoryUpdated(const TArray<FInventorySlot>& InventorySlots);

	void SetWidgetShown(bool bShown);
	void RefreshWidget();
	void UpdateCursorMode(bool bWantCursor);

	void SetLinkedActorsLocked(bool bLocked);
	/** ExitOpen처럼 "박스에 닿으면 타임라인으로 열리는" 블루프린트 문을 잠그거나 연다. */
	void SetTriggeredDoorLocked(AActor* DoorActor, bool bLocked);
	AActor* FindNearestDoorActor() const;

	void HandleSlotClicked(int32 SlotIndex);
	bool TryInsertSlot(int32 SlotIndex, ACharacterPlayer* Player);
	void CheckUnlock();
	void Unlock();
	bool HasInsertableItem(const ACharacterPlayer* Player) const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
	TObjectPtr<USceneComponent> SceneRoot;

	/** 문 메시. 비워 두고 레벨에 따로 배치한 문 위에 이 액터를 올려도 됩니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
	TObjectPtr<UStaticMeshComponent> DoorMesh;

	/**
	 * 문이 차지하는 영역. 이 박스 표면에서 ShowDistance 안에 들어오면 UI가 뜹니다.
	 * 잠겨 있는 동안에는 상호작용 트레이스(Visibility)를 막아서 문 대신 이 액터가 선택됩니다.
	 * 문 전체를 살짝 덮도록 Box Extent를 조절하세요.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
	TObjectPtr<UBoxComponent> DoorVolume;

	/** DoorVolume을 ShowDistance만큼 키운 영역. 플레이어가 겹치면 UI가 뜹니다. (BeginPlay에서 자동으로 맞춤) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
	TObjectPtr<UBoxComponent> ProximityVolume;

	/** 탈출구 번호 (1~4). 바꾸면 RequiredSlots가 자동으로 다시 채워집니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exit", meta = (ClampMin = "1", ClampMax = "4"))
	int32 ExitNumber = 1;

	/** 비워 두면 "N번 탈출구"로 표시됩니다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exit")
	FText ExitTitle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exit")
	TArray<FExitRequirementSlot> RequiredSlots;

	/** 아이템별 UI 아이콘 (없으면 이름만 표시) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exit")
	TMap<FName, TObjectPtr<UTexture2D>> ItemIcons;

	/** true면 넣은 아이템이 인벤토리에서 사라집니다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exit")
	bool bConsumeItems = true;

	/** 플레이어(캡슐 표면)와 문(DoorVolume 표면) 사이가 이 거리 이내면 UI를 표시합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exit|UI", meta = (ClampMin = "0.0", Units = "cm"))
	float ShowDistance = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exit|UI")
	TSubclassOf<UExitDoorWidget> WidgetClass;

	/**
	 * 잠가 두었다가 3개가 모두 들어가면 여는 문.
	 * - ExitOpen 같은 블루프린트 문: 잠긴 동안 문의 감지 박스를 꺼 두고, 열 때 문의 타임라인을 재생합니다.
	 * - DestructibleDoor: OpenDoor()로 엽니다.
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Exit|Link")
	TObjectPtr<AActor> LinkedDoor;

	/** 자동 연결할 때 문으로 인식할 클래스 목록 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exit|Link", meta = (EditCondition = "bAutoLinkNearby"))
	TArray<TSubclassOf<AActor>> AutoLinkDoorClasses;

	/**
	 * true: 한 번 열리면 계속 열려 있습니다.
	 * false: 열린 뒤에는 문 블루프린트의 원래 동작(가까이 가면 열리고 멀어지면 닫힘)으로 돌아갑니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exit|Link")
	bool bKeepDoorOpenAfterUnlock = true;

	/** 꺼 두었다가 3개가 모두 들어가면 켜는 탈출 트리거 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Exit|Link")
	TObjectPtr<AMapEscapeTrigger> LinkedEscapeTrigger;

	/**
	 * LinkedDoor / LinkedEscapeTrigger를 비워 두면
	 * 시작할 때 AutoLinkRadius 안에서 가장 가까운 문과 탈출 트리거를 자동으로 연결합니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exit|Link")
	bool bAutoLinkNearby = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exit|Link", meta = (EditCondition = "bAutoLinkNearby", ClampMin = "0.0", Units = "cm"))
	float AutoLinkRadius = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exit|Text")
	FText InsertHintText = FText::FromString(TEXT("갖고 있는 아이템을 클릭해서 넣으세요"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exit|Text")
	FText NoItemHintText = FText::FromString(TEXT("필요한 아이템이 없습니다"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exit|Text")
	FText UnlockedHintText = FText::FromString(TEXT("탈출구 개방"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exit|Text")
	FText InteractionText = FText::FromString(TEXT("아이템 넣기"));

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Exit")
	bool bUnlocked = false;

private:
	UPROPERTY(Transient)
	TObjectPtr<ACharacterPlayer> NearbyPlayer;

	UPROPERTY(Transient)
	TObjectPtr<UExitDoorWidget> ExitWidget;

	/** 마우스 커서를 켜 준 컨트롤러 (끌 때 되돌리기 위해 기억) */
	TWeakObjectPtr<APlayerController> CursorController;

	bool bWidgetShown = false;

	/** 잠그면서 꺼 둔 문의 감지 박스와 원래 충돌 설정 */
	TArray<TWeakObjectPtr<UShapeComponent>> LockedTriggers;
	TArray<TEnumAsByte<ECollisionEnabled::Type>> LockedTriggerCollision;

	FDelegateHandle InventoryUpdatedHandle;

#pragma region InteractableInterface
public:
	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual FText GetInteractionText_Implementation(AActor* Interactor) const override;
	virtual void BeginFocus_Implementation(AActor* Interactor) override;
	virtual void EndFocus_Implementation(AActor* Interactor) override;
#pragma endregion
};
