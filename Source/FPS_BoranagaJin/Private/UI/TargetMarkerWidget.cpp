

#include "UI/TargetMarkerWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"

void UTargetMarkerWidget::StartLockOnProcess(float InReadyDuration, float InSuccessDuration)
{

    TargetingReadyDuration = InReadyDuration;
    TargetingSuccessDuration = InSuccessDuration;

    CurrentLockOnState = ETargetLockOnState::TargetingReady;
    CurrentTimerDelay = TargetingReadyDuration;
    bIsLockedOn = false;

    // 애니메이션 초기화
    AnimationElapsedTime = 0.f;

    // UI에 초기 상태(흰색)를 반영하는 로직 추가
    if (MarkerImage)
    {
        // 초기 크기와 회전 상태로 설정
        FWidgetTransform InitialTransform;
        InitialTransform.Scale = FVector2D(1.0f, 1.0f);
        InitialTransform.Angle = 0.0f;
        MarkerImage->SetRenderTransform(InitialTransform);
        MarkerImage->SetColorAndOpacity(FLinearColor::White);
    }
}

void UTargetMarkerWidget::UpdateLockOnTimer(float RealTimeDeltaSeconds)
{
    // 락온이 아직 완료되지 않았을 때만 타이머를 업데이트함
    if (!bIsLockedOn)
    {
        CurrentTimerDelay -= RealTimeDeltaSeconds;

        // 애니메이션 경과 시간 업데이트
        AnimationElapsedTime += RealTimeDeltaSeconds;

        //  현재 상태에 맞는 애니메이션 지속 시간으로 Alpha 값 계산
        float CurrentAnimationDuration = (CurrentLockOnState == ETargetLockOnState::TargetingReady) ? TargetingReadyDuration : TargetingSuccessDuration;
        float Alpha = FMath::Clamp(AnimationElapsedTime / CurrentAnimationDuration, 0.f, 1.f);

        if (MarkerImage)
        {
            FWidgetTransform NewTransform;
            FLinearColor InterpColor;

            // 현재 상태에 따라 애니메이션 보간을 결정
            if (CurrentLockOnState == ETargetLockOnState::TargetingReady)
            {
                // 타겟팅 완료 상태: 흰색, 100% -> 75% 크기, 0도 -> 45도 회전
                float InterpAngle = FMath::Lerp(0.0f, 45.0f, Alpha);
                FVector2D InterpScale = FMath::Lerp(FVector2D(1.0f, 1.0f), FVector2D(0.75f, 0.75f), Alpha);

                NewTransform.Scale = InterpScale;
                NewTransform.Angle = InterpAngle;
                InterpColor = FLinearColor::White; // 색상은 흰색 유지
            }
            else if (CurrentLockOnState == ETargetLockOnState::TargetingSuccess)
            {
                //  타겟팅 성공 상태: 크기, 회전 애니메이션 제거
                //  색상만 흰색 -> 주황색으로 보간
                NewTransform.Scale = FVector2D(0.75f, 0.75f); // 75% 크기 고정
                NewTransform.Angle = 45.0f; // 45도 회전 고정
                InterpColor = FMath::Lerp(FLinearColor::White, DamageColor, Alpha);
            }

            MarkerImage->SetRenderTransform(NewTransform);
            MarkerImage->SetColorAndOpacity(InterpColor);
        }

        // 상태 전환 확인
        if (CurrentTimerDelay <= 0.f)
        {
            if (CurrentLockOnState == ETargetLockOnState::TargetingReady)
            {
                // TargetingReady -> TargetingSuccess로 전환
                CurrentLockOnState = ETargetLockOnState::TargetingSuccess;
                CurrentTimerDelay = TargetingSuccessDuration;
                AnimationElapsedTime = 0.f; // 새로운 애니메이션을 위해 경과 시간 초기화
            }
            else if (CurrentLockOnState == ETargetLockOnState::TargetingSuccess)
            {
                // TargetingSuccess 단계 완료. 최종 락온 상태로 전환.
                bIsLockedOn = true;
                CurrentTimerDelay = 0.f;

                // 최종 UI 상태 설정 (주황색, 고정 크기/회전)
                FWidgetTransform FinalTransform;
                FinalTransform.Scale = FVector2D(0.75f, 0.75f);
                FinalTransform.Angle = 45.0f;
                MarkerImage->SetRenderTransform(FinalTransform);
                MarkerImage->SetColorAndOpacity(DamageColor);
            }
        }
    }
}

void UTargetMarkerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (BoolTest)
    {
        FString StateText;

        switch (CurrentLockOnState)
        {
        case ETargetLockOnState::TargetingReady:
            StateText = TEXT("Ready");
            break;
        case ETargetLockOnState::TargetingSuccess:
            StateText = TEXT("Success");
            break;
        case ETargetLockOnState::None:
            StateText = bIsLockedOn ? TEXT("Locked On") : TEXT("None");
            break;
        }

        BoolTest->SetText(FText::FromString(StateText));
    }
}