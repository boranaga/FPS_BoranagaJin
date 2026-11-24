#pragma once

#include "CoreMinimal.h"
#include "Objects/DestructibleDoor.h"
#include "ExitDoor.generated.h"

class AExitDoor;
class ACharacterPlayer;
class AMapEscapeTrigger;
class APlayerController;
class UBoxComponent;
class UPrimitiveComponent;
class UExitDoorWidget;
class UPlayerUISubsystem;
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Exit")
	FName ItemID = NAME_None;

	/** Name displayed in the UI. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Exit")
	FText DisplayName;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, SaveGame, Category = "Exit")
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
 * - The interaction prompt appears only while the player is close AND targeting the door.
 * - Pressing Interact opens the shared ExitDoorWidget; clicking owned item slots inserts them.
 * - Leaving the proximity volume or pressing Escape closes the panel.
 * - Once all three items are inserted, the inherited physical door opens, any
 *   optional LinkedDoor opens, and LinkedEscapeTrigger is enabled.
 *
 * Inherits physical door movement, damage, and destruction from ADestructibleDoor.
 * Uses the existing inventory, door, and escape trigger APIs.
 * - Items are identified by ItemID (FName), not EItemName.
 * - While locked, DoorVolume intercepts interaction traces and the overridden
 *   SetDoorOpen prevents opening through the parent door API.
 */
UCLASS()
class FPS_BORANAGAJIN_API AExitDoor : public ADestructibleDoor
{
	GENERATED_BODY()
	friend class UPlayerUISubsystem;
public:
	AExitDoor();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:
	/** Enforces the exit puzzle when opening the physical door. */
	virtual void SetDoorOpen(bool bOpen) override;

	/** Resets the exit puzzle and locks the door after restoration. */
	virtual void RestoreDestructibleObject() override;

	/** Reapplies saved unlock state to the door and escape trigger. */
	virtual void OnAfterLoad() override;

	/** Repopulates RequiredSlots with the three default items corresponding to ExitNumber. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Exit")
	void ApplyExitPreset();

	UFUNCTION(BlueprintPure, Category = "Exit")
	bool IsUnlocked() const { return bUnlocked; }

	UFUNCTION(BlueprintPure, Category = "Exit")
	int32 GetInsertedCount() const;

	UFUNCTION(BlueprintPure, Category = "Exit")
	static FText GetDefaultItemDisplayName(FName ItemID);

	/** Used by the interaction trace and shared UI to validate proximity. */
	bool IsPlayerInExitRange(const ACharacterPlayer* Player) const;

	/** Anchor above the door bounds for the world-space interaction prompt. */
	FVector GetInteractionPromptLocation() const;

	/** Update the existing interaction HUD prompt while the door is focused. */
	void RefreshInteractionPrompt(ACharacterPlayer* Player);

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
	/** Finds the local player UI manager, not the global game instance. */
	UPlayerUISubsystem* ResolvePlayerUISubsystem() const;

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
	/**
	 * Legacy mesh retained for existing ExitDoor Blueprints.
	 * When IntactMesh is unset, copy this mesh into inherited IntactMesh.
	 * The legacy component itself does not render or collide.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
	TObjectPtr<UStaticMeshComponent> DoorMesh;

	/**
	 * Area occupied by the door. The UI appears within ShowDistance of this box surface.
	 * While locked, this component blocks interaction traces (Visibility) for this actor.
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Exit")
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
	 * Optional additional external door retained for older maps.
	 * The inherited physical door is always controlled, even when unassigned.
	 * - Timeline-driven Blueprint: disable detection boxes while locked, then play the timeline.
	 * - DestructibleDoor: open using OpenDoor().
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Exit|Link")
	TObjectPtr<AActor> LinkedDoor;

	/** Classes recognized as doors during automatic linking. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exit|Link", meta = (EditCondition = "bAutoLinkNearby"))
	TArray<TSubclassOf<AActor>> AutoLinkDoorClasses;

	/**
	 * true: Keep this door open and leave external Blueprint detection boxes disabled.
	 * false: Permit closing this door and restore external Blueprint trigger behavior.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exit|Link")
	bool bKeepDoorOpenAfterUnlock = true;

	/** Escape trigger to keep disabled until all three items have been inserted. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Exit|Link")
	TObjectPtr<AMapEscapeTrigger> LinkedEscapeTrigger;

	/**
	 * Automatically links the nearest escape trigger when unassigned.
	 * External door auto-linking is opt-in unless no physical door mesh is assigned.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exit|Link")
	bool bAutoLinkNearby = true;

	/** Preserve auto-link behavior for an additional external door if desired. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exit|Link", meta = (EditCondition = "bAutoLinkNearby"))
	bool bAutoLinkExternalDoor = false;

	/** When enabled, allow normal door interaction after unlocking if keep-open is false. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exit|Interaction")
	bool bAllowManualDoorInteractionAfterUnlock = false;

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

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, SaveGame, Category = "Exit")
	bool bUnlocked = false;

private:
	UPROPERTY(Transient)
	TObjectPtr<ACharacterPlayer> NearbyPlayer;

	/** The local player's UI subsystem manages the shared ExitDoorWidget lifetime. */
	TWeakObjectPtr<UPlayerUISubsystem> RegisteredUISubsystem;

	// True while this door is registered as nearby, not while its panel is open.
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
