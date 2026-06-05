


#include "UI/UIManagerComponent.h"
#include "UI/StaminaWidget.h"
#include "UI/PlayerDisplayWidget.h"
#include "UI/InteractionWidget.h"
#include "Characters/Player/CharacterPlayer.h"
#include "Characters/Player/FPSPlayerController.h"

#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"


UUIManagerComponent::UUIManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

}

void UUIManagerComponent::InitUIManagerComponent()
{
	if (GetOwner())
	{
		PlayerController = Cast<AFPSPlayerController>(GetOwner());
		if (PlayerController)
		{
			if (PlayerController->GetPawn())
			{
				CharacterPlayer = Cast<ACharacterPlayer>(PlayerController->GetPawn());
			}

			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
			{
				// Set the priority of the mapping to 1, so that it overrides the Jump action with the Fire action when using touch input
				Subsystem->AddMappingContext(UISystemMappingContext, 1);
			}

			if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent))
			{
				EnhancedInputComponent->BindAction(TabAction, ETriggerEvent::Started, this, &UUIManagerComponent::OnTabToggled);
			}

		}
	}

	if (CharacterPlayer)
	{
		CharacterPlayer->OnUIWidgetCreatedDelegate.AddDynamic(this, &UUIManagerComponent::RegisterUIWidget);
		CharacterPlayer->OnStaminaInit.AddDynamic(this, &UUIManagerComponent::InitStaminaBar);
		CharacterPlayer->OnStaminaUpdated.AddDynamic(this, &UUIManagerComponent::SetStaminaBarPercent);
		CharacterPlayer->OnInteractionUIPopUpDelegate.AddDynamic(this, &UUIManagerComponent::PlayPopUpInteractionWidgetAnim);
		CharacterPlayer->OnInteractionUIUpdatedDelegate.AddDynamic(this, &UUIManagerComponent::UpdateInteractionUI);
	}
}

void UUIManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	//InitializeWidgets();
	//SetupInput();

	//ACharacterPlayer* CharacterPlayer = Cast<ACharacterPlayer>(GetOwner());
	//if (CharacterPlayer)
	//{
	//	WeaponSystemComponent = SuraPawnPlayer->GetWeaponSystemComponent();
	//}

	//-------------------------

	InitUILayersMap();

	if (StaminaWidgetClass)
	{
		StaminaWidget = CreateWidget<UStaminaWidget>(GetWorld(), StaminaWidgetClass);
		if (StaminaWidget)
		{
			UE_LOG(LogTemp, Error, TEXT("UIWidgetType: %d"), StaminaWidget->GetUIType());
			RegisterUIWidget(StaminaWidget);
			StaminaWidget->SetVisibility(ESlateVisibility::Visible);
		}
	}

	if (PlayerDisplayWidgetClass)
	{
		PlayerDisplayWidget = CreateWidget<UPlayerDisplayWidget>(GetWorld(), PlayerDisplayWidgetClass);
		if (PlayerDisplayWidget)
		{
			UE_LOG(LogTemp, Error, TEXT("UIWidgetType: %d"), PlayerDisplayWidget->GetUIType());
			RegisterUIWidget(PlayerDisplayWidget);
			PlayerDisplayWidget->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	if (InteractionWidgetClass)
	{
		InteractionWidget = CreateWidget<UInteractionWidget>(GetWorld(), InteractionWidgetClass);
		if (InteractionWidget)
		{
			UE_LOG(LogTemp, Error, TEXT("UIWidgetType: %d"), InteractionWidget->GetUIType());
			RegisterUIWidget(InteractionWidget);
			InteractionWidget->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

TTuple<FVector2D, bool> UUIManagerComponent::GetScreenPositionOfWorldLocation(const FVector& SearchLocation) const
{
	FVector2D ScreenLocation = FVector2D::ZeroVector;
	bool bResult = UGameplayStatics::ProjectWorldToScreen(PlayerController, SearchLocation, ScreenLocation);

	return MakeTuple(ScreenLocation, bResult);
}

bool UUIManagerComponent::IsInViewport(FVector2D ActorScreenPosition, float ScreenRatio_Width, float ScreenRatio_Height) const
{
	FVector2D ViewportSize = GEngine->GameViewport->Viewport->GetSizeXY();

	bool bIsInWidth = true;
	bool bIsInHeight = true;

	// Check Width
	if (ScreenRatio_Width == 0.0f || UKismetMathLibrary::Abs(ScreenRatio_Width) > 1.0f || (ScreenRatio_Width == (1.0f - ScreenRatio_Width)))
	{
		if (ActorScreenPosition.X >= 0.0f && ActorScreenPosition.X <= ViewportSize.X)
		{
			bIsInWidth = true;
		}
		else
		{
			bIsInWidth = false;
		}
	}
	else
	{
		float LargeScreenRatio_Width;
		float SmallScreenRatio_Width;

		if (ScreenRatio_Width < (1.0f - ScreenRatio_Width))
		{
			LargeScreenRatio_Width = 1.0f - ScreenRatio_Width;
			SmallScreenRatio_Width = ScreenRatio_Width;
		}
		else
		{
			LargeScreenRatio_Width = ScreenRatio_Width;
			SmallScreenRatio_Width = 1.0f - ScreenRatio_Width;
		}

		if (ActorScreenPosition.X >= ViewportSize.X * SmallScreenRatio_Width && ActorScreenPosition.X <= ViewportSize.X * LargeScreenRatio_Width)
		{
			bIsInWidth = true;
		}
		else
		{
			bIsInWidth = false;
		}
	}

	// Check Height
	if (ScreenRatio_Height == 0.0f || UKismetMathLibrary::Abs(ScreenRatio_Height) > 1.0f || (ScreenRatio_Height == (1.0f - ScreenRatio_Height)))
	{
		if (ActorScreenPosition.Y >= 0.0f && ActorScreenPosition.Y <= ViewportSize.Y)
		{
			bIsInHeight = true;
		}
		else
		{
			bIsInHeight = false;
		}
	}
	else
	{
		float LargeScreenRatio_Height;
		float SmallScreenRatio_Height;

		if (ScreenRatio_Height < (1.0f - ScreenRatio_Height))
		{
			LargeScreenRatio_Height = 1.0f - ScreenRatio_Height;
			SmallScreenRatio_Height = ScreenRatio_Height;
		}
		else
		{
			LargeScreenRatio_Height = ScreenRatio_Height;
			SmallScreenRatio_Height = 1.0f - ScreenRatio_Height;
		}

		if (ActorScreenPosition.Y >= ViewportSize.Y * SmallScreenRatio_Height && ActorScreenPosition.Y <= ViewportSize.Y * LargeScreenRatio_Height)
		{
			bIsInHeight = true;
		}
		else
		{
			bIsInHeight = false;
		}
	}

	// Return
	if (bIsInWidth && bIsInHeight)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void UUIManagerComponent::OpenInventory()
{
	if (!PlayerDisplayWidget) return;
	PlayerDisplayWidget->SetVisibility(ESlateVisibility::Visible);
	PlayerDisplayWidget->OpenInventory();
	bIsInventoryOpened = true;
}

void UUIManagerComponent::CloseInventory()
{
	if (!PlayerDisplayWidget) return;
	PlayerDisplayWidget->SetVisibility(ESlateVisibility::Hidden);
	PlayerDisplayWidget->CloseInventory();
	bIsInventoryOpened = false;
}

void UUIManagerComponent::OnTabToggled()
{
	bIsInventoryOpened ? CloseInventory() : OpenInventory();
}

void UUIManagerComponent::InitStaminaBar(float maxstamina)
{
	if (StaminaWidget)
	{
		StaminaWidget->InitStaminaBar(maxstamina);
		UE_LOG(LogTemp, Error, TEXT("UUIManagerComponent::InitStaminaBar(float maxstamina)!!!"));
	}
}
void UUIManagerComponent::SetStaminaBarPercent(float const Value)
{
	if (StaminaWidget) StaminaWidget->SetStaminaBarPercent(Value);
}

void UUIManagerComponent::PlayPopUpInteractionWidgetAnim()
{
	if (InteractionWidget) InteractionWidget->PlayPopUpAnim();

	//UE_LOG(LogTemp, Error, TEXT("UUIManagerComponent::PlayPopUpInteractionWidgetAnim"));
}

void UUIManagerComponent::UpdateInteractionUI(bool bFlag, FVector NewLocation)
{
	//UE_LOG(LogTemp, Error, TEXT("UUIManagerComponent::UpdateInteractionUI"));

	FVector2D TargetScreenPosition = GetScreenPositionOfWorldLocation(NewLocation).Get<0>();

	if (bFlag)
	{
		if (IsInViewport(TargetScreenPosition, 1.f, 1.f))
		{
			InteractionWidget->SetPositionInViewport(TargetScreenPosition);

			if (InteractionWidget->Visibility == ESlateVisibility::Hidden)
			{
				InteractionWidget->SetVisibility(ESlateVisibility::Visible);
			}
		}
		//else
		//{
		//	if (InteractionWidget->Visibility == ESlateVisibility::Visible)
		//	{
		//		InteractionWidget->SetVisibility(ESlateVisibility::Hidden);
		//	}
		//}
	}
	else
	{
		if (InteractionWidget->Visibility == ESlateVisibility::Visible)
		{
			InteractionWidget->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}



//void UUIManagerComponent::OpenUI(EUIType UIType)
//{
//	//UBaseUIWidget* TargetWidget = GetWidget(UIType);
//	//if (!TargetWidget) return; // 위젯 생성에 실패한 경우 처리
//
//	//if (!TargetWidget->IsInViewport())
//	//{
//	//	TargetWidget->OpenUI();
//	//}
//}

//UBaseUIWidget* UUIManagerComponent::GetWidget(EUIType UIType)
//{
//	if (UIWidgets.Contains(UIType))
//	{
//		return UIWidgets[UIType];
//	}
//
//	UE_LOG(LogTemp, Warning, TEXT("Widget not initialized!"));
//	return nullptr;
//}

void UUIManagerComponent::InitializeWidgets()
{
	//APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	//if (!PC)
	//{
	//	UE_LOG(LogTemp, Error, TEXT("PlayerController not valid in InitializeWidgets."));
	//	return;
	//}

	//for (const auto& Elem : UIWidgetClasses)
	//{
	//	EUIType UIType = Elem.Key;
	//	TSubclassOf<UBaseUIWidget> WidgetClass = Elem.Value;

	//	if (!WidgetClass) continue;

	//	UBaseUIWidget* NewWidget = CreateWidget<UBaseUIWidget>(GetWorld(), WidgetClass);
	//	if (!NewWidget) continue;
	//	if (!IsValid(NewWidget)) //  생성 실패 여부 확인
	//	{
	//		UE_LOG(LogTemp, Error, TEXT("InitializeWidgets: NewWidget 생성 실패! UIType: %s"), *UEnum::GetValueAsString(UIType));
	//		continue;
	//	}


	//	UIWidgets.Add(UIType, NewWidget);

	//	// 위젯 타입별로 매니저 연결
	//	switch (UIType)
	//	{
	//		// case EUIType::Inventory:
	//		// 	{
	//		// 		if (UInventoryWidget* IW = Cast<UInventoryWidget>(NewWidget))
	//		// 		{
	//		// 			IW->SetInventoryManager(InventoryManager); // InventoryWidget에 InventoryManager를 할당
	//		// 			InventoryManager->SetInventoryWidget(IW);  // InventoryManager에 InventoryWidget을 할당
	//		// 		}
	//		// 		break;
	//		// 	}

	//	case EUIType::KillLog:
	//	{
	//		if (UKillLogWidget* KLW = Cast<UKillLogWidget>(NewWidget))
	//		{
	//			KillLogManager->SetKillLogWidget(KLW);
	//			KLW->SetKillLogManager(KillLogManager);
	//			KLW->AddToViewport();

	//			//UE_LOG(LogTemp, Warning, TEXT("✔ KillLogWidget Viewport에 추가됨"));
	//		}
	//		break;
	//	}

	//	case EUIType::PlayerHUD:
	//	{
	//		if (UPlayerHUD* PW = Cast<UPlayerHUD>(NewWidget))
	//		{
	//			PlayerHUDManager->SetPlayerHUDWidget(PW);
	//			PW->SetPlayerHUDManager(PlayerHUDManager);
	//			PW->AddToViewport(-1);

	//			//UE_LOG(LogTemp, Warning, TEXT("✔ KillLogWidget Viewport에 추가됨"));
	//		}
	//		break;
	//	}

	//	case EUIType::Skill:
	//	{
	//		if (URocketLauncherSkillWidget* RLW = Cast<URocketLauncherSkillWidget>(NewWidget))
	//		{
	//			SkillManager->SetRocketLauncherSkillWidget(RLW);
	//			RLW->SetSKillManager(SkillManager);
	//			RLW->InitUIDataTable(DTUISetting);
	//			//RLW->AddToViewport(); //스킬 사용 시점에 AddToViewport해야함;

	//			//UE_LOG(LogTemp, Warning, TEXT("✔ RocketLauncherWidget Viewport에 추가됨"));
	//		}
	//		break;
	//	}

	//	default:
	//		break;
	//	}
	//}
}



void UUIManagerComponent::OnShowTabMenuStarted(const FInputActionValue& Value)
{

}

void UUIManagerComponent::OnShowTabMenuCompleted(const FInputActionValue& Value)
{

}

void UUIManagerComponent::InitUILayersMap()
{
	UILayers.Add(EUIType::UIType_Stamina, 0);
	UILayers.Add(EUIType::UIType_Interaction, 1);
	UILayers.Add(EUIType::UIType_Inventory, 3);
	UILayers.Add(EUIType::UIType_WeaponAim, 2);
}

void UUIManagerComponent::RegisterUIWidget(UBaseUIWidget* NewUIWidget)
{
	UIWidgets.Add(NewUIWidget->GetUIType(), NewUIWidget);
	NewUIWidget->AddToViewport(*UILayers.Find(NewUIWidget->GetUIType()));
}
