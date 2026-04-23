// Copyright (c) 2026 장윤제. All rights reserved.


#include "RulesHorrorGameInstance.h"
#include "RulesHorrorUtils.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#if WITH_EDITOR
#include "UI/Office/UI_Monitor.h"
#endif

void URulesHorrorGameInstance::Shutdown()
{
#if WITH_EDITOR
	UUI_Monitor::ResetLastActiveWidgetindex();
#endif

	Super::Shutdown();
}

void URulesHorrorGameInstance::QuitGame()
{
	auto pc = URulesHorrorUtils::GetLocalPlayerController(this);
	if (IsInvalid(pc))
		return;

	UKismetSystemLibrary::QuitGame(this, pc, EQuitPreference::Quit, false);
}

void URulesHorrorGameInstance::PauseGame(bool _is_pause)
{
	auto pc = URulesHorrorUtils::GetLocalPlayerController(this);
	if (IsInvalid(pc))
		return;	

	pc->SetPause(_is_pause);

	// UGameplayStatics::SetGlobalTimeDilation(GetWorld(), _is_pause ? 0.0f : 1.0f);
}
