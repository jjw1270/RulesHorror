// Copyright (c) 2026 장윤제. All rights reserved.


#include "LobbyPlayerController.h"
#include "RulesHorrorUtils.h"
#include "StoryFlowSubsystem.h"
#include "RulesHorrorDeveloperSettings.h"

ALobbyPlayerController::ALobbyPlayerController()
{
}

void ALobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	auto story_flow_subsystem = URulesHorrorUtils::GetGameInstanceSubsystem<UStoryFlowSubsystem>(this);
	if (IsValid(story_flow_subsystem))
	{
		const auto dev_setting = GetDefault<URulesHorrorDeveloperSettings>();
		if (IsValid(dev_setting))
		{
			if (dev_setting->_IntroSceneID.IsValid() == false)
			{
				TRACE_ERROR(TEXT("Rules Horror dev setting _IntroSceneID is empty."));
				return;
			}

			story_flow_subsystem->StartFromScene(dev_setting->_IntroSceneID);
		}
	}
}

void ALobbyPlayerController::Tick(float _delta_time)
{
	Super::Tick(_delta_time);
}
