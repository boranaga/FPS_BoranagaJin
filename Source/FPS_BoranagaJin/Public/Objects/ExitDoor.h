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

/** One item slot required to unlock the exit. */
USTRUCT(BlueprintType)
struct FPS_BORANAGAJIN_API FExitRequirementSlot
{
	GENERATED_BODY()

	/** Must match the ItemID defined in the item Blueprint. (KeyCard / Battery / Fuse / Chip) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exit")
	FName ItemID = NAME_None;

	/** Name displayed in the UI. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exit")
	FText DisplayName;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Exit")
	bool bInserted = false;
};

/**
 * An exit that unlocks after three required items have been inserted.
 *
 * - Setting ExitNumber (1-4) automatically populates the three required item slots.
 *     Exit 1: KeyCard / Battery / Fuse   (no Chip)
 *     Exit 2: Battery / Fuse / Chip      (no KeyCard)
 *     Exit 3: Fuse / Chip / KeyCard      (no Battery)
 *     Exit 4: Chip / KeyCard / Battery   (no Fuse)
 * - The UI appears when the player comes within ShowDistance (default: 1 m) of DoorVolume.
 * - Players can click slots for items they own to insert them. (The mouse cursor appears only then.)
 * - Once all three items are inserted, LinkedDoor opens and LinkedEscapeTrigger is enabled.
 *
 * Uses only public functions without modifying existing classes (inventory, door, or escape trigger).
 * - Items are identified by ItemID (FName), not EItemName.
 * - While locked, DoorVolume intercepts interaction traces so the door cannot be opened directly.
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
	/** Repopulates RequiredSlots with the three default items corresponding to ExitNumber. */
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
	/** Blueprint event for additional effects when all three items have been inserted. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Exit")
	void OnExitUnlocked();

private:
	/** Shows or hides the UI through ProximityVolume overlap events without ticks or timers. */
	UFUNCTION()
	void HandleProximityBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void HandleProximityEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void SetNearbyPlayer(ACharacterPlayer* Player);
	void ClearNearbyPlayer();
	/** Refreshes the UI when the inventory changes. (Bound only while the player is nearby.) */
	void HandleInventoryUpdated(const TArray<FInventorySlot>& InventorySlots);

	void SetWidgetShown(bool bShown);
	void RefreshWidget();
	void UpdateCursorMode(bool bWantCursor);

	void SetLinkedActorsLocked(bool bLocked);
	/** Locks or unlocks Blueprint doors like ExitOpen that open via a timeline when a trigger box is entered. */
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

	/** Door mesh. Can be left empty if this actor is placed over a separate door in the level. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
	TObjectPtr<UStaticMeshComponent> DoorMesh;

	/**
	 * Area occupied by the door. The UI appears within ShowDistance of this box surface.
	 * While locked, this actor blocks interaction traces (Visibility) so it is selected instead of the door.
	 * Adjust Box Extent to cover the entire door with a small margin.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
	TObjectPtr<UBoxComponent> DoorVolume;

	/** DoorVolume expanded by ShowDistance. The UI appears when the player overlaps it. (Configured automatically in BeginPlay.) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
	TObjectPtr<UBoxComponent> ProximityVolume;

	/** Exit number (1-4). Changing it automatically repopulates RequiredSlots. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exit", meta = (ClampMin = "1", ClampMax = "4"))
	int32 ExitNumber = 1;

	/** If empty, displays a default title equivalent to "Exit N". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exit")
	FText ExitTitle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exit")
	TArray<FExitRequirementSlot> RequiredSlots;

	/** UI icons for each item. (Displays only the item name if no icon is assigned.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exit")
	TMap<FName, TObjectPtr<UTexture2D>> ItemIcons;

	/** If true, inserted items are removed from the inventory. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exit")
	bool bConsumeItems = true;

	/** Shows the UI when the gap between the player capsule surface and DoorVolume surface is within this distance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exit|UI", meta = (ClampMin = "0.0", Units = "cm"))
	float ShowDistance = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exit|UI")
	TSubclassOf<UExitDoorWidget> WidgetClass;

	/**
	 * Door to keep locked until all three items are inserted.
	 * - Blueprint doors like ExitOpen: disable their detection boxes while locked, then play their timelines on unlock.
	 * - DestructibleDoor: open using OpenDoor().
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Exit|Link")
	TObjectPtr<AActor> LinkedDoor;

	/** Classes recognized as doors during automatic linking. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exit|Link", meta = (EditCondition = "bAutoLinkNearby"))
	TArray<TSubclassOf<AActor>> AutoLinkDoorClasses;

	/**
	 * true: Once opened, the door stays open.
	 * false: After unlocking, the Blueprint door resumes its original behavior (opens nearby, closes when the player leaves).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exit|Link")
	bool bKeepDoorOpenAfterUnlock = true;

	/** Escape trigger to keep disabled until all three items have been inserted. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Exit|Link")
	TObjectPtr<AMapEscapeTrigger> LinkedEscapeTrigger;

	/**
	 * If LinkedDoor / LinkedEscapeTrigger are unassigned,
	 * automatically links the nearest door and escape trigger within AutoLinkRadius at startup.
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

	/** Controller that enabled the mouse cursor, retained so the cursor can be restored later. */
	TWeakObjectPtr<APlayerController> CursorController;

	bool bWidgetShown = false;

	/** Door trigger boxes disabled during locking and their original collision settings. */
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
