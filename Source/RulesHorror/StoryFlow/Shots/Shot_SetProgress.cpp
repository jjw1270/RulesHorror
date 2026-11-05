// Copyright (c) 2026 장윤제. All rights reserved.


#include "StoryFlow/Shots/Shot_SetProgress.h"
#include "SaveGame/RulesHorrorProgressSubsystem.h"
#include "CommonUtils.h"

void UShot_SetProgress::OnEnterShot_Implementation()
{
	Super::OnEnterShot_Implementation();

	URulesHorrorProgressSubsystem* progress_subsystem = URulesHorrorProgressSubsystem::FindFromSubsystemOuter(this);
	if (IsValid(progress_subsystem))
	{
		progress_subsystem->SetProgressFlags(_FlagsToSet, true);
	}
	else
	{
		TRACE_ERROR(TEXT("progress subsystem invalid."));
	}

	FinishShot();
}
