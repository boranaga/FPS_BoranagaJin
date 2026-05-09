


#include "UI/UIManagerComponent.h"
//#include "Characters/Player/CharacterPlayer.h"

#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"


UUIManagerComponent::UUIManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

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

}

void UUIManagerComponent::SetupInput()
{
	if (APlayerController* PC = Cast<APlayerController>(GetOwner()->GetInstigatorController()))
	{
		if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PC->InputComponent))
		{
			//EnhancedInput->BindAction(OpenPauseMenuAction, ETriggerEvent::Started, this, &UUIManagerComponent::TogglePauseMenu);
			//EnhancedInput->BindAction(ShowTabMenuAction, ETriggerEvent::Started, this, &UUIManagerComponent::OnShowTabMenuStarted);
			//EnhancedInput->BindAction(ShowTabMenuAction, ETriggerEvent::Completed, this, &UUIManagerComponent::OnShowTabMenuCompleted);
		}
	}
}

void UUIManagerComponent::OpenUI(EUIType UIType)
{
	//UBaseUIWidget* TargetWidget = GetWidget(UIType);
	//if (!TargetWidget) return; // 위젯 생성에 실패한 경우 처리

	//if (!TargetWidget->IsInViewport())
	//{
	//	TargetWidget->OpenUI();
	//}
}

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
	//// 위젯 클래스가 유효하고, 아직 위젯이 생성되지 않았다면
	//if (TabMenuWidgetClass && !TabMenuWidgetInstance)
	//{
	//	// 1. 이 컴포넌트의 소유자(Owner)를 가져옵니다.
	//	//    (이 컴포넌트가 플레이어 캐릭터에 붙어있다고 가정)
	//	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	//	if (!OwnerPawn)
	//	{
	//		// 소유자가 Pawn이 아니면 컨트롤러를 가져올 수 없습니다.
	//		return;
	//	}

	//	// 2. 소유자인 Pawn에서 컨트롤러를 가져옵니다.
	//	APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController());
	//	if (!PC)
	//	{
	//		// 컨트롤러가 유효하지 않으면(예: AI에 의해 조종되는 Pawn) 중단합니다.
	//		return;
	//	}

	//	// 이제 PC 변수가 유효하므로 위젯을 생성할 수 있습니다.
	//	TabMenuWidgetInstance = CreateWidget<UUserWidget>(PC, TabMenuWidgetClass);

	//	if (TabMenuWidgetInstance)
	//	{
	//		// 뷰포트에 추가
	//		TabMenuWidgetInstance->AddToViewport();
	//	}
	//}
}

void UUIManagerComponent::OnShowTabMenuCompleted(const FInputActionValue& Value)
{
	//// 위젯 인스턴스가 유효하다면 (즉, 화면에 떠 있다면)
	//if (TabMenuWidgetInstance)
	//{
	//	TabMenuWidgetInstance->RemoveFromParent();
	//	TabMenuWidgetInstance = nullptr; // 참조 제거

	//	// *** 중요 ***
	//	// 여기서도 입력 모드를 건드리지 않습니다.
	//}
}