// Copyright (c) 2026 장윤제. All rights reserved.


#include "StoryFlow/Shots/Shot_Fade.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "CommonUtils.h"

namespace
{
	// 레벨을 막 연 직후처럼 카메라 매니저가 아직 없으면 이 시간까지 기다린다.
	static const float FadeCameraWaitLimitSeconds = 3.0f;
}

void UShot_Fade::OnEnterShot_Implementation()
{
	Super::OnEnterShot_Implementation();

	_IsFadeStarted = false;
	if (TryStartFade() && (_WaitForFinish == false || _Duration <= 0.0f))
	{
		FinishShot();
	}
}

void UShot_Fade::OnTickShot_Implementation(float _delta_time)
{
	Super::OnTickShot_Implementation(_delta_time);

	if (_IsFadeStarted == false)
	{
		if (TryStartFade() == false)
		{
			if (GetElapsedTime() > FadeCameraWaitLimitSeconds)
			{
				TRACE_WARNING(TEXT("No camera manager for fade. Skipping."));
				FinishShot();
			}
			return;
		}

		if (_WaitForFinish == false || _Duration <= 0.0f)
		{
			FinishShot();
			return;
		}
	}

	if (GetElapsedTime() - _FadeStartTime >= _Duration)
	{
		FinishShot();
	}
}

bool UShot_Fade::TryStartFade()
{
	APlayerCameraManager* camera_manager = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (IsInvalid(camera_manager))
	{
		return false;
	}

	const float from_alpha = _FromAlpha < 0.0f ? (camera_manager->bEnableFading ? camera_manager->FadeAmount : 0.0f) : _FromAlpha;
	if (_Duration <= 0.0f)
	{
		camera_manager->SetManualCameraFade(_ToAlpha, _Color, false);
	}
	else
	{
		camera_manager->StartCameraFade(from_alpha, _ToAlpha, _Duration, _Color, false, true);
	}

	_IsFadeStarted = true;
	_FadeStartTime = GetElapsedTime();
	return true;
}
