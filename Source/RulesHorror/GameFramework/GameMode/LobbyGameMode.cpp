// Copyright (c) 2026 장윤제. All rights reserved.

#include "LobbyGameMode.h"
#include "RulesHorrorUtils.h"
#include "StoryFlowSubsystem.h"
#include "RulesHorrorDeveloperSettings.h"

ALobbyGameMode::ALobbyGameMode()
{
}

void ALobbyGameMode::StartPlay()
{
	Super::StartPlay();

#if WITH_EDITOR
	if (UStoryFlowSubsystem::IsEditorPlayFromShotSession())
	{
		return;
	}
#endif

	auto story_flow_subsystem = URulesHorrorUtils::GetGameInstanceSubsystem<UStoryFlowSubsystem>(this);
	if (IsInvalid(story_flow_subsystem))
	{
		return;
	}

	const auto dev_setting = GetDefault<URulesHorrorDeveloperSettings>();
	if (IsInvalid(dev_setting))
	{
		return;
	}

	if (dev_setting->_IntroSceneID.IsValid() == false)
	{
		TRACE_ERROR(TEXT("Rules Horror dev setting _IntroSceneID is empty."));
		return;
	}

	story_flow_subsystem->StartFromScene(dev_setting->_IntroSceneID);
}
