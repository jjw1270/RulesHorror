// Copyright (c) 2026 장윤제. All rights reserved.


#include "LoadingPlayerController.h"
#include "RulesHorrorUtils.h"
#include "UI/Loading/UI_Loading.h"

void ALoadingPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsValid(_LoadingWidgetClass))
	{
		auto loading_widget = CreateWidget<UUI_Loading>(this, _LoadingWidgetClass);
		if (IsValid(loading_widget))
		{
			loading_widget->AddToViewport();
		}
	}
	else
	{
		TRACE_ERROR(TEXT("_LoadingWidgetClass Is Invalid."));
	}
}
