// Copyright (c) 2026 장윤제. All rights reserved.


#include "StoryFlow/Shots/Shot_SetInputLock.h"
#include "StoryFlow/RulesHorrorStorySubsystem.h"
#include "CommonUtils.h"

void UShot_SetInputLock::OnEnterShot_Implementation()
{
	Super::OnEnterShot_Implementation();

	if (URulesHorrorStorySubsystem* story_subsystem = URulesHorrorStorySubsystem::Get(this))
	{
		story_subsystem->SetPlayerInputLocked(_Locked);
	}
	else
	{
		TRACE_ERROR(TEXT("story subsystem invalid."));
	}

	FinishShot();
}
