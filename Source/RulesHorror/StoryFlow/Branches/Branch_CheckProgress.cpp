// Copyright (c) 2026 장윤제. All rights reserved.


#include "StoryFlow/Branches/Branch_CheckProgress.h"
#include "SaveGame/RulesHorrorProgressSubsystem.h"
#include "CommonUtils.h"

UBranch_CheckProgress::UBranch_CheckProgress()
{
	FStoryBranchOutput not_met_output;
	not_met_output.DisplayName = FText::FromString(TEXT("미충족"));

	FStoryBranchOutput met_output;
	met_output.DisplayName = FText::FromString(TEXT("충족"));

	_BranchOutputs = { not_met_output, met_output };
}

int32 UBranch_CheckProgress::SelectNextIndex_Implementation(int32 _next_count) const
{
	const URulesHorrorProgressSubsystem* progress_subsystem = URulesHorrorProgressSubsystem::FindFromSubsystemOuter(this);
	if (IsInvalid(progress_subsystem))
	{
		TRACE_ERROR(TEXT("progress subsystem invalid."));
		return 0;
	}

	return progress_subsystem->AreAllProgressFlagsSet(_RequiredFlags) ? 1 : 0;
}
