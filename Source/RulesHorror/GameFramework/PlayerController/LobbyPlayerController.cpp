// Copyright (c) 2026 장윤제. All rights reserved.


#include "LobbyPlayerController.h"
#include "RulesHorrorUtils.h"
#if WITH_EDITOR
#include "UI/Office/UI_Monitor.h"
#endif

ALobbyPlayerController::ALobbyPlayerController()
{
}

void ALobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();

#if WITH_EDITOR
	// 에디터에서 static 변수 리셋용
	UUI_Monitor::ResetLastActiveWidgetindex();
#endif
}

void ALobbyPlayerController::Tick(float _delta_time)
{
	Super::Tick(_delta_time);
}
