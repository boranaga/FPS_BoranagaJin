#include "Objects/ExitDoor.h"

#include "Characters/Player/CharacterPlayer.h"
#include "Characters/InteractionComponent.h"
#include "Items/InventoryName.h"
#include "Items/InventorySlot.h"
#include "Items/InventorySystemComponent.h"
#include "Items/Item.h"
#include "Objects/DestructibleDoor.h"
#include "Objects/MapEscapeTrigger.h"
#include "UI/ExitDoorWidget.h"
#include "PlayerUISubsystem.h"
#include "Engine/LocalPlayer.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/ShapeComponent.h"
#include "Components/TimelineComponent.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

namespace
{
	// Each exit uses three consecutive items, shifted by one per exit. (Exit N starts at index N - 1.)
	// Names must match the ItemID values in the item Blueprints.
	const TCHAR* const GExitItemOrder[] =
	{
		TEXT("KeyCard"),
		TEXT("Battery"),
		TEXT("Fuse"),
		TEXT("Chip")
	};

	// Read and modify only the publicly exposed slot array; do not change the inventory class.
	int32 CountItems(const ACharacterPlayer* Player, FName ItemID)
	{
		const UInventorySystemComponent* Inventory = IsValid(Player) ? Player->GetInventorySystemComponent() : nullptr;
		const TArray<FInventorySlot>* Slots = Inventory ? Inventory->GetInventoryConst(EInventoryName::Item) : nullptr;
		if (!Slots || ItemID.IsNone()) { return 0; }

		int32 Count = 0;
		for (const FInventorySlot& Slot : *Slots)
		{
			if (!Slot.IsEmpty() && Slot.ItemID == ItemID)
			{
				Count += Slot.Num();
			}
		}
		return Count;
	}

	bool ConsumeOneItem(ACharacterPlayer* Player, FName ItemID)
	{
		UInventorySystemComponent* Inventory = IsValid(Player) ? Player->GetInventorySystemComponent() : nullptr;
		TArray<FInventorySlot>* Slots = Inventory ? Inventory->GetInventory(EInventoryName::Item) : nullptr;
		if (!Slots || ItemID.IsNone()) { return false; }

		for (FInventorySlot& Slot : *Slots)
		{
			if (Slot.IsEmpty() || Slot.ItemID != ItemID) { continue; }

			// INDEX_NONE removes the last item. FInventorySlot resets itself when the slot becomes empty.
			AItem* RemovedItem = Slot.RemoveItem(INDEX_NONE);
			if (IsValid(RemovedItem))
			{
				RemovedItem->DeactivateItem();
			}

			// Refresh the inventory UI.
			Player->OnInventoryUpdatedDelegate.Broadcast(*Slots);
			return true;
		}
		return false;
	}

	const int32 GExitRequiredItemCount = 3;

	template<typename TActor>
	TActor* FindNearestActor(const AActor* Origin, float Radius)
	{
		TActor* Nearest = nullptr;
		float NearestDistSq = FMath::Square(Radius);

		for (TActorIterator<TActor> It(Origin->GetWorld()); It; ++It)
		{
			const float DistSq = FVector::DistSquared(It->GetActorLocation(), Origin->GetActorLocation());
			if (DistSq <= NearestDistSq)
			{
				NearestDistSq = DistSq;
				Nearest = *It;
			}
		}
		return Nearest;
	}
}

AExitDoor::AExitDoor()
{
	// Parent ADestructibleDoor owns the root, pivot, and destructive mesh components.
	// Keep ticking available for the inherited open/close interpolation.

	DoorVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("DoorVolume"));
	DoorVolume->SetupAttachment(SceneRoot);
	DoorVolume->SetRelativeLocation(FVector(0.f, 0.f, 110.f));
	DoorVolume->SetBoxExtent(FVector(25.f, 100.f, 110.f));
	// While locked, block only interaction traces (Visibility), not movement or bullets.
	DoorVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DoorVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	DoorVolume->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	DoorVolume->SetGenerateOverlapEvents(false);

	// Player proximity detection box. Uses only overlap events, without ticks or timers.
	ProximityVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("ProximityVolume"));
	ProximityVolume->SetupAttachment(SceneRoot);
	// Must be movable because BeginPlay positions it at DoorVolume.
	ProximityVolume->SetMobility(EComponentMobility::Movable);
	ProximityVolume->SetCanEverAffectNavigation(false);

	// A puzzle exit always starts closed regardless of the base default.
	bStartsOpen = false;
	WidgetClass = UExitDoorWidget::StaticClass();
	AutoLinkDoorClasses.Add(ADestructibleDoor::StaticClass());

	ApplyExitPreset();
}

void AExitDoor::BeginPlay()
{
	// The exit puzzle always starts closed, regardless of Blueprint defaults.
	bStartsOpen = false;
	Super::BeginPlay();

	if (RequiredSlots.IsEmpty())
	{
		ApplyExitPreset();
	}

	for (FExitRequirementSlot& Slot : RequiredSlots)
	{
		Slot.bInserted = false;
	}
	bUnlocked = false;

	if (bAutoLinkNearby)
	{
		// IntactMesh is the primary physical door. Additional door linking is opt-in.
		if (bAutoLinkExternalDoor && !LinkedDoor) { LinkedDoor = FindNearestDoorActor(); }
		if (!LinkedEscapeTrigger) { LinkedEscapeTrigger = FindNearestActor<AMapEscapeTrigger>(this, AutoLinkRadius); }
	}

	SetLinkedActorsLocked(true);

	UE_LOG(LogTemp, Log, TEXT("ExitDoor %s: BeginPlay (Exit %d, LinkedDoor=%s, LinkedEscapeTrigger=%s, ShowDistance=%.0f)"),
		*GetName(), ExitNumber, *GetNameSafe(LinkedDoor), *GetNameSafe(LinkedEscapeTrigger), ShowDistance);

	// Reapply collision settings here to prevent Blueprint-saved values from overriding them.
	// The capsule (Pawn profile) blocks WorldDynamic, so overlap occurs if this box overlaps Pawn.
	ProximityVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ProximityVolume->SetCollisionObjectType(ECC_WorldDynamic);
	ProximityVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	ProximityVolume->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Overlap);
	ProximityVolume->SetGenerateOverlapEvents(true);

	// Match DoorVolume expanded by ShowDistance. (Account for scale in the box extent.)
	FTransform VolumeTransform = DoorVolume->GetComponentTransform();
	VolumeTransform.SetScale3D(FVector::OneVector);
	ProximityVolume->SetWorldTransform(VolumeTransform);
	ProximityVolume->SetBoxExtent(DoorVolume->GetScaledBoxExtent() + FVector(ShowDistance));

	ProximityVolume->OnComponentBeginOverlap.AddUniqueDynamic(this, &AExitDoor::HandleProximityBeginOverlap);
	ProximityVolume->OnComponentEndOverlap.AddUniqueDynamic(this, &AExitDoor::HandleProximityEndOverlap);
	ProximityVolume->UpdateOverlaps();

	UE_LOG(LogTemp, Log, TEXT("ExitDoor %s: ProximityVolume loc=%s extent=%s"),
		*GetName(), *ProximityVolume->GetComponentLocation().ToString(), *ProximityVolume->GetUnscaledBoxExtent().ToString());

	// Handle a player already inside the box at startup.
	TArray<AActor*> OverlappingActors;
	ProximityVolume->GetOverlappingActors(OverlappingActors, ACharacterPlayer::StaticClass());
	for (AActor* Actor : OverlappingActors)
	{
		ACharacterPlayer* Player = Cast<ACharacterPlayer>(Actor);
		if (IsValid(Player) && Player->IsLocallyControlled())
		{
			SetNearbyPlayer(Player);
			break;
		}
	}
}

void AExitDoor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Unregister this door; the shared widget remains owned by PlayerUISubsystem.
	ClearNearbyPlayer();
	Super::EndPlay(EndPlayReason);
}

#if WITH_EDITOR
void AExitDoor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(AExitDoor, ExitNumber))
	{
		ApplyExitPreset();
	}
}
#endif

void AExitDoor::ApplyExitPreset()
{
	const int32 ItemKindCount = UE_ARRAY_COUNT(GExitItemOrder);
	const int32 StartIndex = FMath::Clamp(ExitNumber, 1, ItemKindCount) - 1;

	RequiredSlots.Reset();
	for (int32 Offset = 0; Offset < GExitRequiredItemCount; ++Offset)
	{
		FExitRequirementSlot Slot;
		Slot.ItemID = FName(GExitItemOrder[(StartIndex + Offset) % ItemKindCount]);
		Slot.DisplayName = GetDefaultItemDisplayName(Slot.ItemID);
		RequiredSlots.Add(Slot);
	}
}

int32 AExitDoor::GetInsertedCount() const
{
	int32 Count = 0;
	for (const FExitRequirementSlot& Slot : RequiredSlots)
	{
		if (Slot.bInserted) { ++Count; }
	}
	return Count;
}

FText AExitDoor::GetDefaultItemDisplayName(FName ItemID)
{
	if (ItemID == FName(TEXT("KeyCard"))) { return FText::FromString(TEXT("키카드")); }
	if (ItemID == FName(TEXT("Battery"))) { return FText::FromString(TEXT("배터리")); }
	if (ItemID == FName(TEXT("Fuse"))) { return FText::FromString(TEXT("퓨즈")); }
	if (ItemID == FName(TEXT("Chip"))) { return FText::FromString(TEXT("칩")); }
	return FText::FromName(ItemID);
}

void AExitDoor::SetDoorOpen(bool bOpen)
{
	// All inherited OpenDoor/ToggleDoor requests go through this virtual setter.
	if ((bOpen && !bUnlocked) || (!bOpen && bUnlocked && bKeepDoorOpenAfterUnlock))
	{
		return;
	}

	Super::SetDoorOpen(bOpen);
}

void AExitDoor::RestoreDestructibleObject()
{
	Super::RestoreDestructibleObject();

	// A restored exit is a fresh locked puzzle.
	bUnlocked = false;
	for (FExitRequirementSlot& Slot : RequiredSlots)
	{
		Slot.bInserted = false;
	}

	SetLinkedActorsLocked(true);
	if (bWidgetShown) { RefreshWidget(); }
}

void AExitDoor::OnAfterLoad()
{
	Super::OnAfterLoad();

	// The SaveGame properties restore both unlock state and inserted items.
	// Reapply it to the inherited door and to the optional linked actors.
	SetLinkedActorsLocked(!bUnlocked);
	if (bWidgetShown) { RefreshWidget(); }
}

void AExitDoor::SetLinkedActorsLocked(bool bLocked)
{
	// The query-only box intercepts interaction traces, never physical movement.
	DoorVolume->SetCollisionEnabled(bLocked ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);

	// This physical door is the inherited ADestructibleDoor, not LinkedDoor.
	if (!bDestroyed)
	{
		if (bLocked) { CloseDoor(); }
		else { OpenDoor(); }
	}

	if (IsValid(LinkedEscapeTrigger))
	{
		LinkedEscapeTrigger->SetActorEnableCollision(!bLocked);
	}

	// Legacy levels may still configure an additional external door.
	if (!IsValid(LinkedDoor) || LinkedDoor == this) { return; }

	if (ADestructibleDoor* DestructibleDoor = Cast<ADestructibleDoor>(LinkedDoor))
	{
		if (bLocked)
		{
			DestructibleDoor->CloseDoor();
		}
		else
		{
			DestructibleDoor->OpenDoor();
		}
	}
	else if (LinkedDoor)
	{
		SetTriggeredDoorLocked(LinkedDoor, bLocked);
	}
}

void AExitDoor::SetTriggeredDoorLocked(AActor* DoorActor, bool bLocked)
{
	if (bLocked)
	{
		// Disable the door Blueprint detection boxes so it cannot open automatically when approached.
		LockedTriggers.Reset();
		LockedTriggerCollision.Reset();

		TInlineComponentArray<UShapeComponent*> Shapes(DoorActor);
		for (UShapeComponent* Shape : Shapes)
		{
			if (Shape->GetCollisionEnabled() == ECollisionEnabled::NoCollision) { continue; }

			LockedTriggers.Add(Shape);
			LockedTriggerCollision.Add(Shape->GetCollisionEnabled());
			Shape->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}

		UE_LOG(LogTemp, Log, TEXT("ExitDoor %s: locked door %s (%d trigger(s) disabled)"), *GetName(), *DoorActor->GetName(), LockedTriggers.Num());
		return;
	}

	// Directly play the door Blueprint timelines (opening animations).
	TInlineComponentArray<UTimelineComponent*> Timelines(DoorActor);
	for (UTimelineComponent* Timeline : Timelines)
	{
		Timeline->Play();
	}

	// If the door should stay open, leave detection boxes disabled. (Re-enabling them could close it when the player leaves.)
	// If no timeline was found, restore detection boxes to allow the original behavior.
	if (!bKeepDoorOpenAfterUnlock || Timelines.IsEmpty())
	{
		for (int32 Index = 0; Index < LockedTriggers.Num(); ++Index)
		{
			if (UShapeComponent* Shape = LockedTriggers[Index].Get())
			{
				Shape->SetCollisionEnabled(LockedTriggerCollision[Index]);
			}
		}
	}
	LockedTriggers.Reset();
	LockedTriggerCollision.Reset();

	UE_LOG(LogTemp, Log, TEXT("ExitDoor %s: opened door %s (%d timeline(s) played)"), *GetName(), *DoorActor->GetName(), Timelines.Num());
}

AActor* AExitDoor::FindNearestDoorActor() const
{
	AActor* Nearest = nullptr;
	float NearestDistSq = FMath::Square(AutoLinkRadius);

	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Candidate = *It;
		if (Candidate == this || Candidate->IsA(AExitDoor::StaticClass())) { continue; }

		const bool bIsDoor = AutoLinkDoorClasses.ContainsByPredicate([Candidate](const TSubclassOf<AActor>& DoorClass)
			{
				return DoorClass && Candidate->IsA(DoorClass);
			});
		if (!bIsDoor) { continue; }

		const float DistSq = FVector::DistSquared(Candidate->GetActorLocation(), GetActorLocation());
		if (DistSq <= NearestDistSq)
		{
			NearestDistSq = DistSq;
			Nearest = Candidate;
		}
	}
	return Nearest;
}

void AExitDoor::HandleProximityBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UE_LOG(LogTemp, Error, TEXT("ExitDoor %s: proximity begin overlap with %s"), *GetName(), *GetNameSafe(OtherActor));

	ACharacterPlayer* Player = Cast<ACharacterPlayer>(OtherActor);
	if (IsValid(Player) && Player->IsLocallyControlled())
	{
		SetNearbyPlayer(Player);
	}
}

void AExitDoor::HandleProximityEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor && OtherActor == NearbyPlayer)
	{
		UE_LOG(LogTemp, Error, TEXT("ExitDoor %s: proximity end overlap"), *GetName());
		ClearNearbyPlayer();
	}
}

void AExitDoor::SetNearbyPlayer(ACharacterPlayer* Player)
{
	if (NearbyPlayer == Player) { return; }

	ClearNearbyPlayer();
	NearbyPlayer = Player;

	// Reassert the closed state if anything outside the puzzle tried to open it.
	if (!bUnlocked)
	{
		if (!bDestroyed && IsDoorOpen()) { CloseDoor(); }

		if (ADestructibleDoor* ExternalDoor = Cast<ADestructibleDoor>(LinkedDoor))
		{
			if (ExternalDoor != this && ExternalDoor->IsDoorOpen())
			{
				ExternalDoor->CloseDoor();
			}
		}
	}

	// Reflect inventory changes in the UI while the player is nearby.
	InventoryUpdatedHandle = NearbyPlayer->OnInventoryUpdatedDelegate.AddUObject(this, &AExitDoor::HandleInventoryUpdated);

	// Proximity only registers the door. The panel is opened via Interact.
	SetWidgetShown(true);
	if (UInteractionComponent* Interaction = Player->GetInteractionComponent())
	{
		Interaction->UpdateInteractionTarget();
	}
}

void AExitDoor::ClearNearbyPlayer()
{
	ACharacterPlayer* Player = NearbyPlayer.Get();
	if (IsValid(Player) && InventoryUpdatedHandle.IsValid())
	{
		Player->OnInventoryUpdatedDelegate.Remove(InventoryUpdatedHandle);
	}
	InventoryUpdatedHandle.Reset();

	const bool bWasRegistered = bWidgetShown;
	NearbyPlayer = nullptr;
	if (bWasRegistered)
	{
		SetWidgetShown(false); // Also closes the panel if this door owned it.
	}

	// Remove the interaction prompt immediately, without waiting for the next trace.
	if (IsValid(Player))
	{
		if (UInteractionComponent* Interaction = Player->GetInteractionComponent())
		{
			if (Interaction->GetCurrentInteractionTarget() == this)
			{
				Player->OnInteractionUIUpdatedDelegate.Broadcast(false, FVector::ZeroVector);
				Interaction->UpdateInteractionTarget();
			}
		}
	}
}

void AExitDoor::HandleInventoryUpdated(const TArray<FInventorySlot>& InventorySlots)
{
	if (bWidgetShown)
	{
		RefreshWidget();
	}
}

bool AExitDoor::IsPlayerInExitRange(const ACharacterPlayer* Player) const
{
	return IsValid(Player) && IsValid(NearbyPlayer) && NearbyPlayer.Get() == Player && bWidgetShown;
}

FVector AExitDoor::GetInteractionPromptLocation() const
{
	if (IsValid(DoorVolume))
	{
		const FBoxSphereBounds& Bounds = DoorVolume->Bounds;
		return Bounds.Origin + FVector(0.f, 0.f, Bounds.BoxExtent.Z + 35.f);
	}
	return GetActorLocation() + FVector(0.f, 0.f, 150.f);
}

void AExitDoor::RefreshInteractionPrompt(ACharacterPlayer* Player)
{
	if (!IsPlayerInExitRange(Player)) { return; }
	const UInteractionComponent* Interaction = Player->GetInteractionComponent();
	if (!IsValid(Interaction) || Interaction->GetCurrentInteractionTarget() != this) { return; }

	UPlayerUISubsystem* UISubsystem = RegisteredUISubsystem.Get();
	if (IsValid(UISubsystem) && !UISubsystem->IsExitDoorUIOpen())
	{
		// Use the same multi-point selection as all other interactables.
		// The legacy door-bounds anchor remains the fallback when no points
		// are enabled on this actor.
		if (UInteractionComponent* MutableInteraction = Player->GetInteractionComponent())
		{
			MutableInteraction->RefreshInteractionPrompt();
		}
	}
}

UPlayerUISubsystem* AExitDoor::ResolvePlayerUISubsystem() const
{
	if (!IsValid(NearbyPlayer)) { return nullptr; }

	const APlayerController* PlayerController = Cast<APlayerController>(NearbyPlayer->GetController());
	ULocalPlayer* LocalPlayer = IsValid(PlayerController) ? PlayerController->GetLocalPlayer() : nullptr;
	return IsValid(LocalPlayer) ? LocalPlayer->GetSubsystem<UPlayerUISubsystem>() : nullptr;
}

void AExitDoor::SetWidgetShown(bool bShown)
{
	if (bShown)
	{
		if (!IsValid(NearbyPlayer)) { return; }
		UPlayerUISubsystem* UISubsystem = ResolvePlayerUISubsystem();
		if (!IsValid(UISubsystem))
		{
			UE_LOG(LogTemp, Warning, TEXT("ExitDoor %s: no local PlayerUISubsystem."), *GetName());
			return;
		}

		// Registration selects this door as the currently displayed exit.
		bWidgetShown = true;
		RegisteredUISubsystem = UISubsystem;
		UISubsystem->RegisterNearbyExitDoor(this);
		return;
	}

	bWidgetShown = false;
	if (UPlayerUISubsystem* UISubsystem = RegisteredUISubsystem.Get())
	{
		UISubsystem->UnregisterNearbyExitDoor(this);
	}
	RegisteredUISubsystem.Reset();
}

void AExitDoor::RefreshWidget()
{
	// Multiple nearby exits share the UI registered under EUIType::ExitDoor.
	// Only the active door is allowed to write its contents.
	UPlayerUISubsystem* UISubsystem = RegisteredUISubsystem.Get();
	if (!bWidgetShown || !IsValid(UISubsystem) || !UISubsystem->IsActiveExitDoor(this)) { return; }
	UExitDoorWidget* Widget = Cast<UExitDoorWidget>(UISubsystem->GetUIWidget(EUIType::ExitDoor));
	if (!IsValid(Widget)) { return; }

	bool bHasInsertable = false;
	TArray<FExitSlotViewData> ViewSlots;
	ViewSlots.Reserve(RequiredSlots.Num());

	for (const FExitRequirementSlot& Slot : RequiredSlots)
	{
		FExitSlotViewData& View = ViewSlots.AddDefaulted_GetRef();
		View.DisplayName = Slot.DisplayName.IsEmpty() ? GetDefaultItemDisplayName(Slot.ItemID) : Slot.DisplayName;
		if (const TObjectPtr<UTexture2D>* Icon = ItemIcons.Find(Slot.ItemID))
		{
			View.Icon = *Icon;
		}

		if (Slot.bInserted)
		{
			View.State = EExitSlotState::Inserted;
		}
		else if (CountItems(NearbyPlayer, Slot.ItemID) > 0)
		{
			View.State = EExitSlotState::Owned;
			bHasInsertable = true;
		}
		else
		{
			View.State = EExitSlotState::Missing;
		}
	}

	const FText Title = ExitTitle.IsEmpty()
		? FText::Format(FText::FromString(TEXT("{0}번 탈출구")), FText::AsNumber(ExitNumber))
		: ExitTitle;

	const FText& Hint = bUnlocked ? UnlockedHintText : (bHasInsertable ? InsertHintText : NoItemHintText);
	Widget->SetExitData(Title, ViewSlots, Hint, bUnlocked);

	// Cursor and input mode are owned by the same subsystem as the widget.
	// Keep Escape and the slot buttons usable even when no insertable items remain.
	UISubsystem->SetExitDoorCursorWanted(this, true);
}

bool AExitDoor::HasInsertableItem(const ACharacterPlayer* Player) const
{
	for (const FExitRequirementSlot& Slot : RequiredSlots)
	{
		if (!Slot.bInserted && CountItems(Player, Slot.ItemID) > 0)
		{
			return true;
		}
	}
	return false;
}

void AExitDoor::HandleSlotClicked(int32 SlotIndex)
{
	UPlayerUISubsystem* UISubsystem = RegisteredUISubsystem.Get();
	if (!IsValid(UISubsystem) || !UISubsystem->IsActiveExitDoor(this) ||
		!IsPlayerInExitRange(NearbyPlayer))
	{
		return;
	}

	if (TryInsertSlot(SlotIndex, NearbyPlayer))
	{
		CheckUnlock();
	}
	RefreshWidget();
}

bool AExitDoor::TryInsertSlot(int32 SlotIndex, ACharacterPlayer* Player)
{
	if (bUnlocked || !RequiredSlots.IsValidIndex(SlotIndex)) { return false; }

	FExitRequirementSlot& Slot = RequiredSlots[SlotIndex];
	if (Slot.bInserted || CountItems(Player, Slot.ItemID) <= 0) { return false; }

	if (bConsumeItems && !ConsumeOneItem(Player, Slot.ItemID)) { return false; }

	Slot.bInserted = true;
	UE_LOG(LogTemp, Log, TEXT("ExitDoor %s: slot %d inserted (%d/%d)"), *GetName(), SlotIndex, GetInsertedCount(), RequiredSlots.Num());
	return true;
}

void AExitDoor::CheckUnlock()
{
	if (!RequiredSlots.IsEmpty() && GetInsertedCount() == RequiredSlots.Num())
	{
		Unlock();
	}
}

void AExitDoor::Unlock()
{
	if (bUnlocked) { return; }
	bUnlocked = true;

	SetLinkedActorsLocked(false);

	OnExitUnlocked();
	OnExitDoorUnlocked.Broadcast(this);
}

// The interaction key opens the puzzle UI. Insertion is handled by slot buttons.
void AExitDoor::Interact_Implementation(AActor* Interactor)
{
	ACharacterPlayer* Player = Cast<ACharacterPlayer>(Interactor);
	if (!IsPlayerInExitRange(Player)) { return; }

	if (bUnlocked)
	{
		if (bAllowManualDoorInteractionAfterUnlock && !bKeepDoorOpenAfterUnlock &&
			Super::CanInteract_Implementation(Interactor))
		{
			ToggleDoor();
		}
		return;
	}

	// The InteractionComponent already checked both its camera trace and reach.
	const UInteractionComponent* Interaction = Player->GetInteractionComponent();
	if (!IsValid(Interaction) || Interaction->GetCurrentInteractionTarget() != this) { return; }

	if (UPlayerUISubsystem* UISubsystem = RegisteredUISubsystem.Get())
	{
		UISubsystem->OpenExitDoorUI(this, Player);
	}
}

bool AExitDoor::CanInteract_Implementation(AActor* Interactor) const
{
	const ACharacterPlayer* Player = Cast<ACharacterPlayer>(Interactor);
	if (!IsPlayerInExitRange(Player)) { return false; }

	if (!bUnlocked)
	{
		// The puzzle remains interactive even when the physical door has broken.
		return true;
	}

	return bAllowManualDoorInteractionAfterUnlock && !bKeepDoorOpenAfterUnlock &&
		Super::CanInteract_Implementation(Interactor);
}

FText AExitDoor::GetInteractionText_Implementation(AActor* Interactor) const
{
	if (bUnlocked)
	{
		return bAllowManualDoorInteractionAfterUnlock && !bKeepDoorOpenAfterUnlock
			? Super::GetInteractionText_Implementation(Interactor)
			: UnlockedHintText;
	}

	// Allow opening the requirements panel even if no required item is owned.
	return InteractionText;
}

void AExitDoor::BeginFocus_Implementation(AActor* Interactor)
{
	Super::BeginFocus_Implementation(Interactor);
	RefreshInteractionPrompt(Cast<ACharacterPlayer>(Interactor));
}

void AExitDoor::EndFocus_Implementation(AActor* Interactor)
{
	Super::EndFocus_Implementation(Interactor);
	ACharacterPlayer* Player = Cast<ACharacterPlayer>(Interactor);
	if (IsValid(Player) && IsPlayerInExitRange(Player))
	{
		Player->OnInteractionUIUpdatedDelegate.Broadcast(false, FVector::ZeroVector);
	}
}
